#ifndef __INKSCAPE_H__
#define __INKSCAPE_H__

/*
 * Interface to main application
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "forward.h"

class SPRepr;

#define INKSCAPE inkscape_get_instance()

void inkscape_application_init (const gchar *argv0, gboolean use_gui);

/* Preference management */
gboolean inkscape_load_preferences (Inkscape::Application * inkscape);
gboolean inkscape_save_preferences (Inkscape::Application * inkscape);
SPRepr *inkscape_get_repr (Inkscape::Application *inkscape, const gchar *key);

Inkscape::Application *inkscape_get_instance();

#define SP_ACTIVE_EVENTCONTEXT inkscape_active_event_context ()
SPEventContext * inkscape_active_event_context (void);

#define SP_ACTIVE_DOCUMENT inkscape_active_document ()
SPDocument * inkscape_active_document (void);

#define SP_ACTIVE_DESKTOP inkscape_active_desktop ()
SPDesktop * inkscape_active_desktop (void);

bool inkscape_is_sole_desktop_for_document(SPDesktop const &desktop);

gchar *homedir_path(const char *filename);
gchar *profile_path(const char *filename);

void inkscape_switch_desktops_next ();
void inkscape_switch_desktops_prev ();

void inkscape_dialogs_hide ();
void inkscape_dialogs_unhide ();
void inkscape_dialogs_toggle ();

/*
 * fixme: This has to be rethought
 */

void inkscape_refresh_display (Inkscape::Application *inkscape);

/*
 * fixme: This also
 */

void inkscape_exit (Inkscape::Application *inkscape);

#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=c++:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
