#define __SP_HELP_C__

/*
 * Help/About window
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 authors
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "path-prefix.h"

#include <gtk/gtk.h>
#include "inkscape.h"
#include "document.h"
#include "sp-text.h"
#include "svg-view.h"
#include "help.h"
#include "file.h"
#include <glibmm/i18n.h>
#include "libnr/nr-macros.h"
#include "inkscape_version.h"

static GtkWidget *w = NULL;

static gint
sp_help_about_delete (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    w = NULL;
    return FALSE;
}

#define WINDOW_MIN 20
#define WINDOW_MAX INT_MAX

void
sp_help_about (void)
{
    SPDocument *doc;
    SPObject *version;
    GtkWidget *v;
    gint width, height;


    /* REJON: INKSCAPE_PIXMAPSDIR was changed to INKSCAPE_SCREENSDIR to 
     * coordinate with the directory reorganization.
     */

    if (!w) {
        /* TRANSLATORS: This is the filename of the `About Inkscape' picture in
           the `screens' directory.  Thus the translation of "about.svg" should be
           the filename of its translated version, e.g. about.zh.svg for Chinese.

           N.B. about.svg changes once per release.  (We should probably rename
           the original to about-0.40.svg etc. as soon as we have a translation.
           If we do so, then add an item to release-checklist saying that the
           string here should be changed.) */
	char *about = g_build_filename(INKSCAPE_SCREENSDIR, _("about.svg"), NULL);
        doc = sp_document_new (about, FALSE, TRUE);
        g_free(about);
        g_return_if_fail (doc != NULL);
        version = doc->getObjectById("version");
        
        if (version && SP_IS_TEXT (version)) {
            sp_text_set_repr_text_multiline ( SP_TEXT (version), 
                                              INKSCAPE_VERSION);
        }

        sp_document_ensure_up_to_date (doc);


        w = gtk_window_new (GTK_WINDOW_TOPLEVEL);

        gtk_window_set_title (GTK_WINDOW (w), _("About Inkscape"));

        width  = static_cast< gint > ( CLAMP( sp_document_width(doc), 
                                              WINDOW_MIN, WINDOW_MAX ));
        
        height = static_cast< gint > ( CLAMP( sp_document_height(doc), 
                                              WINDOW_MIN, WINDOW_MAX ));

        gtk_window_set_default_size ( GTK_WINDOW (w), width, height );

        gtk_window_set_position( GTK_WINDOW(w), GTK_WIN_POS_CENTER);

        gtk_window_set_policy ( GTK_WINDOW (w), TRUE, TRUE, FALSE);

        gtk_signal_connect ( GTK_OBJECT (w), "delete_event", 
                             GTK_SIGNAL_FUNC (sp_help_about_delete), NULL);

        v = sp_svg_view_widget_new (doc);

        sp_svg_view_widget_set_resize ( SP_SVG_VIEW_WIDGET (v), FALSE, 
                                        sp_document_width (doc), 
                                        sp_document_height (doc));

        sp_document_unref (doc);
        gtk_widget_show (v);
        gtk_container_add (GTK_CONTAINER (w), v);

    } // close if (!w)

    gtk_window_present (GTK_WINDOW (w));

} // close sp_help_about()

void
sp_help_open_tutorial (GtkMenuItem *menuitem, gpointer data)
{
    gchar const *name = static_cast<gchar const *>(data);
    gchar *c = g_build_filename(INKSCAPE_TUTORIALSDIR, name, NULL);
    sp_file_open(c, NULL, false);
    g_free(c);
}

void
sp_help_open_screen(gchar const *name)
{
    gchar *c = g_build_filename(INKSCAPE_SCREENSDIR, name, NULL);
    sp_file_open(c, NULL, false);
    g_free(c);
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
