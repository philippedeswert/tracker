/*
 * Copyright (C) 2009, Nokia <ivan.frade@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include <string.h>
#include <locale.h>

#include <glib.h>
#include <gio/gio.h>

#include <libtracker-common/tracker-common.h>

#include <libtracker-data/tracker-data-manager.h>
#include <libtracker-data/tracker-data-query.h>
#include <libtracker-data/tracker-data-update.h>
#include <libtracker-data/tracker-data.h>
#include <libtracker-data/tracker-sparql-query.h>

static gchar *tests_data_dir = NULL;
static gchar *xdg_location = NULL;

typedef struct _TestInfo TestInfo;

struct _TestInfo {
	const gchar *test_name;
	const gchar *data;
	gboolean expect_query_error;
	gboolean expect_update_error;
};

const TestInfo tests[] = {
	{ "aggregates/aggregate-1", "aggregates/data-1", FALSE },
	{ "aggregates/aggregate-distinct-1", "aggregates/data-1", FALSE },
	{ "aggregates/aggregate-group-1", "aggregates/data-1", FALSE },
	{ "algebra/two-nested-opt", "algebra/two-nested-opt", FALSE },
	{ "algebra/two-nested-opt-alt", "algebra/two-nested-opt", FALSE },
	{ "algebra/opt-filter-3", "algebra/opt-filter-3", FALSE },
	{ "algebra/filter-placement-1", "algebra/data-2", FALSE },
	{ "algebra/filter-placement-2", "algebra/data-2", FALSE },
	{ "algebra/filter-placement-3", "algebra/data-2", FALSE },
	{ "algebra/filter-placement-3a", "algebra/data-2", FALSE },
	{ "algebra/filter-nested-1", "algebra/data-1", FALSE },
	{ "algebra/filter-nested-2", "algebra/data-1", FALSE },
	{ "algebra/filter-scope-1", "algebra/data-2", FALSE },
	{ "algebra/filter-in-1", "algebra/data-2", FALSE },
	{ "algebra/filter-in-2", "algebra/data-2", FALSE },
	{ "algebra/filter-in-3", "algebra/data-2", FALSE },
	{ "algebra/filter-in-4", "algebra/data-2", FALSE },
	{ "algebra/filter-in-5", "algebra/data-2", FALSE },
	{ "algebra/var-scope-join-1", "algebra/var-scope-join-1", FALSE },
	{ "anon/query", "anon/data", FALSE },
	{ "anon/query-2", "anon/data", FALSE },
	{ "ask/ask-1", "ask/data", FALSE },
	{ "basic/base-prefix-3", "basic/data-1", FALSE },
	{ "basic/compare-cast", "basic/data-1", FALSE },
	{ "basic/predicate-variable", "basic/data-1", FALSE },
	{ "basic/predicate-variable-2", "basic/data-1", FALSE },
	{ "basic/predicate-variable-3", "basic/data-1", FALSE },
	{ "basic/predicate-variable-4", "basic/data-1", FALSE },
	{ "bnode-coreference/query", "bnode-coreference/data", FALSE },
	{ "bound/bound1", "bound/data", FALSE },
	{ "datetime/delete-1", "datetime/data-3", FALSE },
	{ "datetime/functions-localtime-1", "datetime/data-1", FALSE },
	{ "datetime/functions-timezone-1", "datetime/data-2", FALSE },
	{ "datetime/functions-timezone-2", "datetime/data-2", FALSE },
	{ "expr-ops/query-ge-1", "expr-ops/data", FALSE },
	{ "expr-ops/query-le-1", "expr-ops/data", FALSE },
	{ "expr-ops/query-minus-1", "expr-ops/data", FALSE },
	{ "expr-ops/query-mul-1", "expr-ops/data", FALSE },
	{ "expr-ops/query-plus-1", "expr-ops/data", FALSE },
	{ "expr-ops/query-unminus-1", "expr-ops/data", FALSE },
	{ "expr-ops/query-unplus-1", "expr-ops/data", FALSE },
	{ "expr-ops/query-res-1", "expr-ops/data", FALSE },
	{ "functions/functions-property-1", "functions/data-1", FALSE },
	{ "functions/functions-tracker-1", "functions/data-1", FALSE },
	{ "functions/functions-tracker-2", "functions/data-2", FALSE },
	{ "functions/functions-tracker-loc-1", "functions/data-3", FALSE },
	{ "functions/functions-xpath-1", "functions/data-1", FALSE },
	{ "functions/functions-xpath-2", "functions/data-1", FALSE },
	{ "functions/functions-xpath-3", "functions/data-1", FALSE },
	{ "functions/functions-xpath-4", "functions/data-1", FALSE },
	{ "functions/functions-xpath-5", "functions/data-1", FALSE },
	{ "functions/functions-xpath-6", "functions/data-1", FALSE },
	{ "functions/functions-xpath-7", "functions/data-1", FALSE },
	{ "functions/functions-xpath-8", "functions/data-1", FALSE },
	{ "functions/functions-xpath-9", "functions/data-1", FALSE },
	{ "graph/graph-1", "graph/data-1", FALSE },
	{ "graph/graph-2", "graph/data-2", FALSE },
	{ "graph/graph-3", "graph/data-3", FALSE },
	{ "graph/graph-4", "graph/data-3", FALSE },
	{ "graph/graph-4", "graph/data-4", FALSE },
	{ "optional/q-opt-complex-1", "optional/complex-data-1", FALSE },
	{ "optional/simple-optional-triple", "optional/simple-optional-triple", FALSE },
	{ "regex/regex-query-001", "regex/regex-data-01", FALSE },
	{ "regex/regex-query-002", "regex/regex-data-01", FALSE },
	{ "sort/query-sort-1", "sort/data-sort-1", FALSE },
	{ "sort/query-sort-2", "sort/data-sort-1", FALSE },
	{ "sort/query-sort-3", "sort/data-sort-3", FALSE },
	{ "sort/query-sort-4", "sort/data-sort-4", FALSE },
	{ "sort/query-sort-5", "sort/data-sort-4", FALSE },
	{ "sort/query-sort-6", "sort/data-sort-4", FALSE },
	{ "sort/query-sort-7", "sort/data-sort-1", FALSE },
	{ "sort/query-sort-8", "sort/data-sort-5", FALSE },
	{ "subqueries/subqueries-1", "subqueries/data-1", FALSE },
	{ "subqueries/subqueries-union-1", "subqueries/data-1", FALSE },
	{ "subqueries/subqueries-union-2", "subqueries/data-1", FALSE },
	/* Bracket error after WHERE */
	{ "error/query-error-1", "error/query-error-1", TRUE, FALSE },
	/* Unknown property */
	{ "error/query-error-2", "error/query-error-2", TRUE, FALSE },
	{ "error/update-error-query-1", "error/update-error-1", FALSE, TRUE },

	{ "turtle/turtle-query-001", "turtle/turtle-data-001", FALSE },
	{ "turtle/turtle-query-002", "turtle/turtle-data-002", FALSE },
	{ NULL }
};

static int
strstr_i (const char *a, const char *b)
{
	return strstr (a, b) != NULL ? 1 : 0;
}

static void
check_result (TrackerDBCursor *cursor,
              const TestInfo *test_info,
              const gchar *results_filename,
              GError *error)
{
	int (*comparer) (const char *a, const char *b);
	GString *test_results;
	gchar *results;
	GError *nerror = NULL;

	if (test_info->expect_query_error) {
		comparer = strstr_i;
		g_assert (error != NULL);
	} else {
		comparer = strcmp;
		g_assert_no_error (error);
	}

	g_file_get_contents (results_filename, &results, NULL, &nerror);
	g_assert_no_error (nerror);
	g_clear_error (&nerror);

	/* compare results with reference output */

	test_results = g_string_new ("");

	if (cursor) {
		gint col;

		while (tracker_db_cursor_iter_next (cursor, NULL, &error)) {
			for (col = 0; col < tracker_db_cursor_get_n_columns (cursor); col++) {
				const gchar *str;

				if (col > 0) {
					g_string_append (test_results, "\t");
				}

				str = tracker_db_cursor_get_string (cursor, col, NULL);
				if (str != NULL) {
					/* bound variable */
					g_string_append_printf (test_results, "\"%s\"", str);
				}
			}

			g_string_append (test_results, "\n");
		}
	} else if (test_info->expect_query_error) {
		g_string_append (test_results, error->message);
		g_clear_error (&error);
	}

	if (comparer (results, test_results->str)) {
		/* print result difference */
		gchar *quoted_results;
		gchar *command_line;
		gchar *quoted_command_line;
		gchar *shell;
		gchar *diff;

		quoted_results = g_shell_quote (test_results->str);
		command_line = g_strdup_printf ("echo -n %s | diff -u %s -", quoted_results, results_filename);
		quoted_command_line = g_shell_quote (command_line);
		shell = g_strdup_printf ("sh -c %s", quoted_command_line);
		g_spawn_command_line_sync (shell, &diff, NULL, NULL, &error);
		g_assert_no_error (error);

		g_error ("%s", diff);

		g_free (quoted_results);
		g_free (command_line);
		g_free (quoted_command_line);
		g_free (shell);
		g_free (diff);
	}

	g_string_free (test_results, TRUE);
	g_free (results);
}

static void
test_sparql_query (TestInfo      *test_info,
                   gconstpointer  context)
{
	TrackerDBCursor *cursor;
	GError *error = NULL;
	gchar *data_filename;
	gchar *query, *query_filename;
	gchar *results_filename;
	gchar *prefix, *data_prefix, *test_prefix;
	const gchar *test_schemas[2] = { NULL, NULL };

	/* initialization */
	prefix = g_build_path (G_DIR_SEPARATOR_S, TOP_SRCDIR, "tests", "libtracker-data", NULL);
	data_prefix = g_build_filename (prefix, test_info->data, NULL);
	test_prefix = g_build_filename (prefix, test_info->test_name, NULL);
	g_free (prefix);

	test_schemas[0] = data_prefix;

	tracker_db_journal_set_rotating (FALSE, G_MAXSIZE, NULL);

	tracker_data_manager_init (TRACKER_DB_MANAGER_FORCE_REINDEX,
	                           test_schemas,
	                           NULL, FALSE, FALSE,
	                           100, 100, NULL, NULL, NULL, &error);

	g_assert_no_error (error);

	/* data_path = g_build_path (G_DIR_SEPARATOR_S, TOP_SRCDIR, "tests", "libtracker-data", NULL); */

	/* load data set */
	data_filename = g_strconcat (data_prefix, ".ttl", NULL);
	if (g_file_test (data_filename, G_FILE_TEST_IS_REGULAR)) {
		tracker_turtle_reader_load (data_filename, &error);
		g_assert_no_error (error);
	} else {
		/* no .ttl available, assume .rq with SPARQL Update */
		gchar *data;

		g_free (data_filename);

		data_filename = g_strconcat (data_prefix, ".rq", NULL);
		g_file_get_contents (data_filename, &data, NULL, &error);
		g_assert_no_error (error);

		tracker_data_update_sparql (data, &error);
		if (test_info->expect_update_error) {
			g_assert (error != NULL);
			g_clear_error (&error);
		} else {
			g_assert_no_error (error);
		}

		g_free (data);
	}

	query_filename = g_strconcat (test_prefix, ".rq", NULL);
	g_file_get_contents (query_filename, &query, NULL, &error);
	g_assert_no_error (error);

	results_filename = g_strconcat (test_prefix, ".out", NULL);

	/* perform actual query */

	cursor = tracker_data_query_sparql_cursor (query, &error);

	check_result (cursor, test_info, results_filename, error);

	g_free (query_filename);
	g_free (query);

	query_filename = g_strconcat (test_prefix, ".extra.rq", NULL);
	if (g_file_get_contents (query_filename, &query, NULL, NULL)) {
		g_object_unref (cursor);
		cursor = tracker_data_query_sparql_cursor (query, &error);
		g_assert_no_error (error);
		g_free (results_filename);
		results_filename = g_strconcat (test_prefix, ".extra.out", NULL);
		check_result (cursor, test_info, results_filename, error);
	}

	g_free (data_prefix);
	g_free (test_prefix);

	if (cursor) {
		g_object_unref (cursor);
	}

	/* cleanup */

	g_free (data_filename);
	g_free (query_filename);
	g_free (query);
	g_free (results_filename);

	tracker_data_manager_shutdown ();
}

static void
setup (TestInfo      *info,
       gconstpointer  context)
{
	gint i;

	i = GPOINTER_TO_INT (context);
	*info = tests[i];

	/* Sadly, we can't use ONE location per test because GLib
	 * caches XDG env vars, so g_get_*dir() will not change if we
	 * update the environment, this sucks majorly.
	 */
	if (!xdg_location) {
		gchar *basename;

		/* NOTE: g_test_build_filename() doesn't work env vars G_TEST_* are not defined?? */
		basename = g_strdup_printf ("%d", g_test_rand_int_range (0, G_MAXINT));
		xdg_location = g_build_path (G_DIR_SEPARATOR_S, tests_data_dir, basename, NULL);
		g_free (basename);

		g_assert_true (g_setenv ("XDG_DATA_HOME", xdg_location, TRUE));
		g_assert_true (g_setenv ("XDG_CACHE_HOME", xdg_location, TRUE));
		g_assert_true (g_setenv ("TRACKER_DB_ONTOLOGIES_DIR", TOP_SRCDIR "/data/ontologies/", TRUE));
	}
}

static void
teardown (TestInfo      *info,
          gconstpointer  context)
{
	gchar *cleanup_command;

	/* clean up */
	g_print ("Removing temporary data (%s)\n", xdg_location);

	cleanup_command = g_strdup_printf ("rm -Rf %s/", xdg_location);
	g_spawn_command_line_sync (cleanup_command, NULL, NULL, NULL, NULL);
	g_free (cleanup_command);

	g_free (xdg_location);
	xdg_location = NULL;
}

int
main (int argc, char **argv)
{
	gchar *current_dir;
	gint result;
	gint i;

	setlocale (LC_COLLATE, "en_US.utf8");

	current_dir = g_get_current_dir ();
	tests_data_dir = g_build_path (G_DIR_SEPARATOR_S, current_dir, "test-data", NULL);
	g_free (current_dir);

	g_test_init (&argc, &argv, NULL);

	/* add test cases */
	for (i = 0; tests[i].test_name; i++) {
		gchar *testpath;

#ifndef HAVE_LIBICU
		/* Skip tests which fail collation tests and are known
		 * to do so. For more details see:
		 *
		 * https://bugzilla.gnome.org/show_bug.cgi?id=636074
		 */
		if (strcmp (tests[i].test_name, "functions/functions-xpath-2") == 0) {
			continue;
		}
#endif

		testpath = g_strconcat ("/libtracker-data/sparql/", tests[i].test_name, NULL);
		g_test_add (testpath, TestInfo, GINT_TO_POINTER(i), setup, test_sparql_query, teardown);
		g_free (testpath);
	}

	/* run tests */
	result = g_test_run ();

	g_remove (tests_data_dir);
	g_free (tests_data_dir);

	return result;
}
