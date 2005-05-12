#define __SP_ANCHOR_C__

/*
 * SVG <a> element implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#define noSP_ANCHOR_VERBOSE

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <string.h>

#include <glibmm/i18n.h>
#include "display/sp-canvas.h"
#include "dialogs/object-attributes.h"
#include "xml/repr.h"
#include "attributes.h"
#include "sp-anchor.h"

/* fixme: This is insane, and should be removed */
#include "svg-view.h"

static void sp_anchor_class_init(SPAnchorClass *ac);
static void sp_anchor_init(SPAnchor *anchor);

static void sp_anchor_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr);
static void sp_anchor_release(SPObject *object);
static void sp_anchor_set(SPObject *object, unsigned int key, const gchar *value);
static Inkscape::XML::Node *sp_anchor_write(SPObject *object, Inkscape::XML::Node *repr, guint flags);

static gchar *sp_anchor_description(SPItem *item);
static gint sp_anchor_event(SPItem *item, SPEvent *event);

static SPGroupClass *parent_class;

GType sp_anchor_get_type(void)
{
    static GType type = 0;
    
    if (!type) {
        GTypeInfo info = {
            sizeof(SPAnchorClass),
            NULL,	/* base_init */
            NULL,	/* base_finalize */
            (GClassInitFunc) sp_anchor_class_init,
            NULL,	/* class_finalize */
            NULL,	/* class_data */
            sizeof(SPAnchor),
            16,	/* n_preallocs */
            (GInstanceInitFunc) sp_anchor_init,
            NULL,	/* value_table */
        };
        type = g_type_register_static(SP_TYPE_GROUP, "SPAnchor", &info, (GTypeFlags) 0);
    }
    
    return type;
}

static void sp_anchor_class_init(SPAnchorClass *ac)
{
    SPObjectClass *sp_object_class = (SPObjectClass *) ac; 
    SPItemClass *item_class = (SPItemClass *) ac;
    
    parent_class = (SPGroupClass *) g_type_class_ref(SP_TYPE_GROUP);
    
    sp_object_class->build = sp_anchor_build;
    sp_object_class->release = sp_anchor_release;
    sp_object_class->set = sp_anchor_set;
    sp_object_class->write = sp_anchor_write;

    item_class->description = sp_anchor_description;
    item_class->event = sp_anchor_event;
}

static void sp_anchor_init(SPAnchor *anchor)
{
    anchor->href = NULL;
}

static void sp_anchor_build(SPObject *object, SPDocument *document, Inkscape::XML::Node *repr)
{
    if (((SPObjectClass *) (parent_class))->build) {
        ((SPObjectClass *) (parent_class))->build(object, document, repr);
    }

    sp_object_read_attr(object, "xlink:type");
    sp_object_read_attr(object, "xlink:role");
    sp_object_read_attr(object, "xlink:arcrole");
    sp_object_read_attr(object, "xlink:title");
    sp_object_read_attr(object, "xlink:show");
    sp_object_read_attr(object, "xlink:actuate");
    sp_object_read_attr(object, "xlink:href");
    sp_object_read_attr(object, "target");
}

static void sp_anchor_release(SPObject *object)
{
    SPAnchor *anchor = SP_ANCHOR(object);

    if (anchor->href) {
        g_free(anchor->href);
        anchor->href = NULL;
    }

    if (((SPObjectClass *) parent_class)->release) {
        ((SPObjectClass *) parent_class)->release(object);
    }
}

static void sp_anchor_set(SPObject *object, unsigned int key, const gchar *value)
{
    SPAnchor *anchor = SP_ANCHOR(object);

    switch (key) {
	case SP_ATTR_XLINK_HREF:
            g_free(anchor->href);
            anchor->href = g_strdup(value);
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	case SP_ATTR_XLINK_TYPE:
	case SP_ATTR_XLINK_ROLE:
	case SP_ATTR_XLINK_ARCROLE:
	case SP_ATTR_XLINK_TITLE:
	case SP_ATTR_XLINK_SHOW:
	case SP_ATTR_XLINK_ACTUATE:
	case SP_ATTR_TARGET:
            object->requestModified(SP_OBJECT_MODIFIED_FLAG);
            break;
	default:
            if (((SPObjectClass *) (parent_class))->set) {
                ((SPObjectClass *) (parent_class))->set(object, key, value);
            }
            break;
    }
}


#define COPY_ATTR(rd,rs,key) sp_repr_set_attr((rd), (key), rs->attribute(key));

static Inkscape::XML::Node *sp_anchor_write(SPObject *object, Inkscape::XML::Node *repr, guint flags)
{
    SPAnchor *anchor = SP_ANCHOR(object);

    if ((flags & SP_OBJECT_WRITE_BUILD) && !repr) {
        repr = sp_repr_new("svg:a");
    }

    sp_repr_set_attr(repr, "xlink:href", anchor->href);

    if (repr != SP_OBJECT_REPR(object)) {
        COPY_ATTR(repr, object->repr, "xlink:type");
        COPY_ATTR(repr, object->repr, "xlink:role");
        COPY_ATTR(repr, object->repr, "xlink:arcrole");
        COPY_ATTR(repr, object->repr, "xlink:title");
        COPY_ATTR(repr, object->repr, "xlink:show");
        COPY_ATTR(repr, object->repr, "xlink:actuate");
        COPY_ATTR(repr, object->repr, "target");
    }

    if (((SPObjectClass *) (parent_class))->write) {
        ((SPObjectClass *) (parent_class))->write(object, repr, flags);
    }

    return repr;
}

static gchar *sp_anchor_description(SPItem *item)
{
    SPAnchor *anchor = SP_ANCHOR(item);
    if (anchor->href) 
        return g_strdup_printf (_("<b>Link</b> to %s"), anchor->href);
    else 
        return g_strdup (_("<b>Link</b> without URI"));
}

/* fixme: We should forward event to appropriate container/view */

static gint sp_anchor_event(SPItem *item, SPEvent *event)
{
    SPAnchor *anchor = SP_ANCHOR(item);
    
    switch (event->type) {
	case SP_EVENT_ACTIVATE:
            if (anchor->href) {
                g_print("Activated xlink:href=\"%s\"\n", anchor->href);
                return TRUE;
            }
            break;
	case SP_EVENT_MOUSEOVER:
            /* fixme: */
            if (SP_IS_SVG_VIEW(event->data)) {
                SPSVGView *svgview = (SPSVGView *) event->data;
                GdkCursor *cursor = gdk_cursor_new(GDK_HAND2);
                gdk_window_set_cursor(GTK_WIDGET(SP_CANVAS_ITEM(svgview->drawing)->canvas)->window, cursor);
                gdk_cursor_destroy(cursor);
            }
            break;
	case SP_EVENT_MOUSEOUT:
            /* fixme: */
            if (SP_IS_SVG_VIEW(event->data)) {
                SPSVGView *svgview = (SPSVGView *) event->data;
                gdk_window_set_cursor(GTK_WIDGET(SP_CANVAS_ITEM(svgview->drawing)->canvas)->window, NULL);
            }
            break;
	default:
            break;
    }
    
    return FALSE;
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
