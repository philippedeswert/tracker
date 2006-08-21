/* Tracker
 * Copyright (C) 2005, Mr Jamie McCracken
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */



#include "tracker-indexer.h"
#include "tracker-utils.h"


#define INDEXBNUM       262144            /* initial bucket number of inverted index */
#define INDEXALIGN      -2                /* alignment of inverted index */
#define INDEXFBP        32                /* size of free block pool of inverted index */




Indexer *
tracker_indexer_open (const char *name)
{

	char *base_dir = g_build_filename (g_get_home_dir(), ".Tracker", "Indexes", NULL);
	char *word_index_name = g_strconcat (base_dir, "/words/", name, NULL);
	char *blob_index_name = g_strconcat (base_dir, "/blobs/", name, NULL);

	tracker_log ("Word index is %s and blob index is %s", word_index_name, blob_index_name);


	if (!tracker_file_is_valid (word_index_name)) {
		g_mkdir_with_parents (word_index_name, 00755);
	}

	if (!tracker_file_is_valid (blob_index_name)) {
		g_mkdir_with_parents (blob_index_name, 00755);
	}

  	CURIA *word_index = cropen (word_index_name, CR_OWRITER | CR_OCREAT | CR_ONOLCK, INDEXBNUM, 2);

	if (!word_index) {
		tracker_log ("word index was not closed properly - attempting repair");
		if (crrepair (word_index_name)) {
			word_index = cropen (word_index_name, CR_OWRITER | CR_OCREAT | CR_ONOLCK, INDEXBNUM, 2);
		} else {
			g_assert ("indexer is dead");
		}
	}


	VILLA *blob_index = vlopen (blob_index_name, VL_OWRITER  | VL_OCREAT | VL_ONOLCK, VL_CMPINT);

	g_free (base_dir);
	g_free (word_index_name);
	g_free (blob_index_name);

	Indexer *result = g_new (Indexer, 1);

	result->word_index  = word_index;
	result->blob_index = blob_index;

	result->mutex = g_mutex_new ();
	result->search_waiting_mutex = g_mutex_new ();

	crsetalign (word_index , INDEXALIGN);
	//crsetfbpsiz (word_index, OD_INDEXFBP);

	return result;
}

void
tracker_indexer_close (Indexer *indexer)
{
	g_return_if_fail (indexer);

	g_mutex_lock (indexer->mutex);

	vlclose (indexer->blob_index);
	crclose (indexer->word_index);

	g_mutex_unlock (indexer->mutex);
	g_mutex_free (indexer->mutex);
	g_mutex_free (indexer->search_waiting_mutex);

	g_free (indexer);
	
}


static gboolean
WAITING (Indexer *indexer)
{
	gboolean result;

	g_return_val_if_fail (indexer, FALSE);


	result = g_mutex_trylock (indexer->search_waiting_mutex);

	if (result) {
		g_mutex_unlock (indexer->search_waiting_mutex);
	}	

	return !result;
}

static void
LOCK (Indexer *indexer)
{
	g_return_if_fail (indexer);

	g_mutex_lock (indexer->mutex);
}

static void	 	
UNLOCK (Indexer *indexer)
{
	g_return_if_fail (indexer);

	g_mutex_unlock (indexer->mutex);
}


static void 
RELOCK (Indexer *indexer)
{
	g_return_if_fail (indexer);

	/* give priority to other threads waiting  */
	g_mutex_lock (indexer->search_waiting_mutex); 
	g_mutex_unlock (indexer->search_waiting_mutex);			

	LOCK (indexer);	


}

/* removes dud hits */
void
tracker_indexer_sweep (Indexer *indexer)
{
	WordPair *pairs;
	char *kbuf, *vbuf;
	int i, rnum, tnum, ksiz, vsiz, pnum, wi;
	
	g_return_if_fail (indexer && indexer->db_con);

	LOCK (indexer);

  	if ((rnum = crrnum (indexer->word_index)) < 1) {
		return;
	}

	if (!criterinit (indexer->word_index)) {
		return;
	}

	UNLOCK (indexer);
	tnum = 0;

	while (TRUE){

		/* give priority to other threads waiting  */
		RELOCK (indexer);

		if (!(kbuf = criternext(indexer->word_index, &ksiz))){
			UNLOCK (indexer);
			return;
		}

		if (!(vbuf = crget (indexer->word_index, kbuf, ksiz, 0, -1, &vsiz))){
        		g_free (kbuf);
			UNLOCK (indexer);
	      		return;
	    	}

		UNLOCK (indexer);

		pairs = (WordPair *) vbuf;
    		pnum = vsiz / sizeof (WordPair);
   
		wi = 0;

    		for (i = 0; i < pnum; i++){

      			if (tracker_db_index_id_exists (indexer->db_con, pairs[i].id) ) {
		        	pairs[wi++] = pairs[i];
      			}
    		}


		/* give priority to other threads waiting  */
		RELOCK (indexer);


    		if (wi > 0) {
      			if (!crput (indexer->word_index, kbuf, ksiz, vbuf, wi * sizeof (WordPair), CR_DOVER)){
        			g_free (vbuf);
		     	   	g_free (kbuf);
				UNLOCK (indexer);
			        return;
			}
		} else {
			if (!crout (indexer->word_index, kbuf, ksiz)){
        			g_free (vbuf);
		     	   	g_free (kbuf);
				UNLOCK (indexer);
			        return;
      			}
    		}
		UNLOCK (indexer);

		free(vbuf);
		free(kbuf);
    		tnum++;

		/* send thread to sleep for a short while so we are not using too much cpu or hogging this resource */
		g_usleep (1000) ;
  	}
 
}



gboolean	
tracker_indexer_optimize (Indexer *indexer)
{
	if (!croptimize (indexer->word_index, INDEXBNUM)) {
		return FALSE;
  	}

	return TRUE;
}


/*  indexing api */
gboolean	
tracker_indexer_insert_word (Indexer *indexer, unsigned int id, const char *word,  int score)
{

	
	WordPair *current_pairs;
	WordPair *pairs;
	
	char *tmp;
	int tsiz, i;

	g_return_val_if_fail ((indexer && word), FALSE);

	tracker_log ("inserting word %s with score %d into ID %d", word, score, id);

	/* give priority to other threads waiting  */
	RELOCK (indexer);

	/* check if existing record is there and append and sort word/score pairs if necessary */
	if ((tmp = crget (indexer->word_index, word, -1, 0, -1, &tsiz)) != NULL) {
		
		UNLOCK (indexer);

      		if (tsiz >= (int) sizeof (WordPair)){
	       		
			current_pairs = (WordPair *) tmp;
		
			int pnum = tsiz / sizeof (WordPair);

			pairs = g_new (WordPair, pnum +1);

		
			if ((pnum > 0) && (score > current_pairs[pnum -1].score)) {

				/* need to insert into right place - use a binary sort to do it quickly by starting in the middle */
				div_t t = div (pnum, 2);

				i = t.quot;

				gboolean unsorted = TRUE;
				
				while (unsorted) {

					if (score == current_pairs[i].score) {

						unsorted = FALSE;

					} else if (score > current_pairs[i].score) {
						
						if ((i == 0) || ((i > 0) && (score < current_pairs[i-1].score))) {
							unsorted = FALSE;							
						} 
						
						i--;
						
					} else {
						
						if ((i == pnum-1) || ((i < pnum-1) && (score > current_pairs[i+1].score))) {
							unsorted = FALSE;							
						} else {
						
							i++;
						}
					}

				}


				int j;

				for (j = pnum-1; j > i; j--) {
					pairs[j+1] = current_pairs[j];
				}
				pairs[i+1].id = id;
				pairs[i+1].score = score;
				for (j = 0; j < i+1; j++) {
					pairs[j] = current_pairs[j];
				}
					

			} else {
				pairs[pnum].id = id;
				pairs[pnum].score = score;
			}


			RELOCK (indexer);

		        if (!crput (indexer->word_index, word, -1, (char *) pairs, (tsiz + sizeof (WordPair)), CR_DOVER)) {
				g_free (tmp);
				g_free (pairs);
				UNLOCK (indexer);
				return FALSE;
			}

			g_free (tmp);
			g_free (pairs);

		}
			
	} else {
		
		WordPair pair;

		pair.id = id;
		pair.score =score;

		if (!crput (indexer->word_index, word, -1, (char *) &pair, sizeof (pair), CR_DOVER)) {
			UNLOCK (indexer);
			return FALSE;
		}
	}
	
	UNLOCK (indexer);	        
	return TRUE;
    

}



WordPair * 
tracker_indexer_get_hits (Indexer *indexer, const char *word, int offset, int limit,  int *count)
{
	char *tmp;
	int tsiz;

	g_return_val_if_fail ((indexer && word), NULL);

	if (!g_mutex_trylock (indexer->mutex)) {
		g_mutex_lock (indexer->search_waiting_mutex);
		g_mutex_lock (indexer->mutex);
		g_mutex_unlock (indexer->search_waiting_mutex);
	}


	if(!(tmp = crget (indexer->word_index, word, -1, (offset * sizeof (WordPair)), limit, &tsiz))){
		UNLOCK (indexer);
		return NULL;
	}

	UNLOCK (indexer);

	*count = tsiz / sizeof (WordPair);

	return (WordPair *)tmp;	


}

