#ifndef __SP_EVENT_CONTEXT_H__
#define __SP_EVENT_CONTEXT_H__

/*
 * Base class for event processors
 *
 * This is per desktop object, which (its derivatives) implements
 * different actions bound to mouse events.
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <glib-object.h>
#include <gtk/gtkwidget.h>
#include "selcue.h"
#include "gradient-drag.h"

#include "forward.h"

class SPRepr;

namespace Inkscape { class MessageContext; }

struct SPEventContext : public GObject {
	void enableSelectionCue(bool enable=true);
	void enableGrDrag(bool enable=true);

	/* Desktop eventcontext stack */
	SPEventContext *next;
	unsigned int key;
	SPDesktop *desktop;
	SPRepr *repr;
	gchar **cursor_shape;
	gint hot_x, hot_y;
	GdkCursor *cursor;

	gint xp , yp; // where drag started
	gint tolerance;
	bool within_tolerance; // are we still within tolerance of origin

	SPItem *item_to_select; // the item where mouse_press occurred, to be selected if this is a click not drag

	Inkscape::MessageContext *defaultMessageContext() {
		return _message_context;
	}

	Inkscape::MessageContext *_message_context;

	SPSelCue *_selcue;

	GrDrag *_grdrag;
};

struct SPEventContextClass : public GObjectClass {
	void (* setup) (SPEventContext *ec);
	void (* finish) (SPEventContext *ec);
	void (* set) (SPEventContext *ec, const gchar *key, const gchar *val);
	void (* activate) (SPEventContext *ec);
	void (* deactivate) (SPEventContext *ec);
	gint (* root_handler) (SPEventContext *ec, GdkEvent *event);
	gint (* item_handler) (SPEventContext *ec, SPItem *item, GdkEvent *event);
};

#define SP_EVENT_CONTEXT_DESKTOP(e) (SP_EVENT_CONTEXT (e)->desktop)
#define SP_EVENT_CONTEXT_REPR(e) (SP_EVENT_CONTEXT (e)->repr)

#define SP_EVENT_CONTEXT_STATIC 0

SPEventContext *sp_event_context_new (GType type, SPDesktop *desktop, SPRepr *repr, unsigned int key);
void sp_event_context_finish (SPEventContext *ec);
void sp_event_context_read (SPEventContext *ec, const gchar *key);
void sp_event_context_activate (SPEventContext *ec);
void sp_event_context_deactivate (SPEventContext *ec);

gint sp_event_context_root_handler (SPEventContext *ec, GdkEvent *event);
gint sp_event_context_item_handler (SPEventContext *ec, SPItem *item, GdkEvent *event);

void sp_event_root_menu_popup (SPDesktop *desktop, SPItem *item, GdkEvent *event);

gint gobble_key_events (guint keyval, gint mask);
gint gobble_motion_events (gint mask);

void sp_event_context_update_cursor (SPEventContext *ec);

void sp_event_show_modifier_tip (Inkscape::MessageContext *message_context, GdkEvent *event, const gchar *ctrl_tip, const gchar *shift_tip, const gchar *alt_tip);

guint get_group0_keyval (GdkEventKey *event);

#endif
