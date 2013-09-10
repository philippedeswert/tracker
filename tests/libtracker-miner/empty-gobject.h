/* empty-gobject.h generated by valac, the Vala compiler, do not modify */


#ifndef __EMPTY_GOBJECT_H__
#define __EMPTY_GOBJECT_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS


#define TYPE_EMPTY_OBJECT (empty_object_get_type ())
#define EMPTY_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_EMPTY_OBJECT, EmptyObject))
#define EMPTY_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_EMPTY_OBJECT, EmptyObjectClass))
#define IS_EMPTY_OBJECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_EMPTY_OBJECT))
#define IS_EMPTY_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_EMPTY_OBJECT))
#define EMPTY_OBJECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_EMPTY_OBJECT, EmptyObjectClass))

typedef struct _EmptyObject EmptyObject;
typedef struct _EmptyObjectClass EmptyObjectClass;
typedef struct _EmptyObjectPrivate EmptyObjectPrivate;

struct _EmptyObject {
	GObject parent_instance;
	EmptyObjectPrivate * priv;
};

struct _EmptyObjectClass {
	GObjectClass parent_class;
};


GType empty_object_get_type (void);
EmptyObject* empty_object_new (void);
EmptyObject* empty_object_construct (GType object_type);
gint empty_object_get_id (EmptyObject* self);
void empty_object_set_id (EmptyObject* self, gint value);


G_END_DECLS

#endif