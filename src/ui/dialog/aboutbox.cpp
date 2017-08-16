/**
 * @file
 * Inkscape About box - implementation.
 */
/* Authors:
 *   Derek P. Moore <derekm@hackunix.org>
 *   MenTaLguY <mental@rydia.net>
 *   Kees Cook <kees@outflux.net>
 *   Jon Phillips <jon@rejon.org>
 *   Abhishek Sharma
 *
 * Copyright (C) 2004 Derek P. Moore
 * Copyright 2004 Kees Cook
 * Copyright 2004 Jon Phillips
 * Copyright 2005 MenTaLguY
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ui/dialog/aboutbox.h"

#include <fstream>

#include <glibmm/fileutils.h>
#include <glibmm/i18n.h>
#include <glibmm/miscutils.h>

#include <gtkmm/aspectframe.h>
#include <gtkmm/textview.h>

#include "path-prefix.h"
#include "document.h"
#include "svg-view-widget.h"
#include "sp-text.h"
#include "text-editing.h"
#include "ui/icon-names.h"
#include "util/units.h"

#include "inkscape-version.h"


namespace Inkscape {
namespace UI {
namespace Dialog {



static Gtk::Widget *build_splash_widget();

static AboutBox *window=NULL;

void AboutBox::show_about() {
    if (!window)
        window = new AboutBox();
    window->show();
}

void AboutBox::hide_about() {
    if (window)
        window->hide();
}

/**
 * Constructor
 */ 
AboutBox::AboutBox() {
    // call this first
    initStrings();

    // Insert the Splash widget.  This is placed directly into the
    // content area of the dialog, whereas everything else is placed
    // automatically by the Gtk::AboutDialog parent class
    Gtk::Widget *splash=build_splash_widget();
    if (splash) {
        get_content_area()->pack_end(*manage(splash), true, true);
        splash->show_all();
    }

    // Set Application metadata, which will be automatically
    // inserted into text widgets by the Gtk::AboutDialog parent class
    set_program_name("Inkscape");
    set_version(Inkscape::version_string);
    set_logo_icon_name(INKSCAPE_ICON("inkscape"));
    set_website("https://www.inkscape.org");
    set_website_label(_("Inkscape website"));
    set_license_type(Gtk::LICENSE_GPL_3_0);
    set_comments(_("Open Source Scalable Vector Graphics Editor\n"
                   "Draw Freely."));
}

Gtk::Widget *build_splash_widget() {
    /* TRANSLATORS: This is the filename of the `About Inkscape' picture in
       the `screens' directory.  Thus the translation of "about.svg" should be
       the filename of its translated version, e.g. about.zh.svg for Chinese.

       Please don't translate the filename unless the translated picture exists. */

    // Try to get the translated version of the 'About Inkscape' file first.  If the
    // translation fails, or if the file does not exist, then fall-back to the
    // default untranslated "about.svg" file
    //
    // FIXME? INKSCAPE_SCREENSDIR and "about.svg" are in UTF-8, not the
    // native filename encoding... and the filename passed to sp_document_new
    // should be in UTF-*8..
    auto about = Glib::build_filename(INKSCAPE_SCREENSDIR, _("about.svg"));
    if (!Glib::file_test (about, Glib::FILE_TEST_EXISTS)) {
        about = Glib::build_filename(INKSCAPE_SCREENSDIR, "about.svg");
    }

    // Create an Inkscape document from the 'About Inkscape' picture
    SPDocument *doc=SPDocument::createNewDoc (about.c_str(), TRUE);
    g_return_val_if_fail(doc != NULL, NULL);

    SPObject *version = doc->getObjectById("version");
    if ( version && SP_IS_TEXT(version) ) {
        sp_te_set_repr_text_multiline (SP_TEXT (version), Inkscape::version_string);
    }
    doc->ensureUpToDate();

    // TODO: Return a Gdk::Pixbuf instead of a widget, for better integration
    // with the parent Gtk::AboutDialog class
    GtkWidget *v=sp_svg_view_widget_new(doc);

    // temporary hack: halve the dimensions so the dialog will fit
    double width=doc->getWidth().value("px") / 2;
    double height=doc->getHeight().value("px") / 2;
    
    doc->doUnref();

    SP_SVG_VIEW_WIDGET(v)->setResize(false, static_cast<int>(width), static_cast<int>(height));

    Gtk::AspectFrame *frame=new Gtk::AspectFrame();
    frame->unset_label();
    frame->set_shadow_type(Gtk::SHADOW_NONE);
    frame->property_ratio() = width / height;
    frame->add(*manage(Glib::wrap(v)));

    return frame;
}

/**
 * This method must be called before any of the texts are
 * used for making widgets
 */  
void AboutBox::initStrings() {
    //##############################
    //# A U T H O R S
    //##############################

    // Create an empty vector to store the list of authors
    std::vector<Glib::ustring> authors;

    // Try to copy the list of authors from the "AUTHORS" file, which
    // should have been installed into the share/doc directory
    auto authors_filename = Glib::build_filename(INKSCAPE_DOCDIR, "AUTHORS");
    std::ifstream authors_filestream(authors_filename);
    if(authors_filestream) {
        std::string author_line;

        while (std::getline(authors_filestream, author_line)) {
            authors.push_back(author_line);
        }
    }

    // Set the author credits in this dialog, using the author list
    set_authors(authors);

    //##############################
    //# T R A N S L A T O R S
    //##############################

    Glib::ustring translators_text;

    // TRANSLATORS: Put here your name (and other national contributors')      
    // one per line in the form of: name surname (email). Use \n for newline.
    Glib::ustring thisTranslation = _("translator-credits");

    /**
     * See if the translators for the current language
     * made an entry for "translator-credits".  If it exists,
     * put it at the top of the window,  add some space between
     * it and the list of all translators.
     *      
     * NOTE:  Do we need 2 more .po entries for titles:
     *  "translators for this language"
     *  "all translators"  ??     
     */                          
    if (thisTranslation != "translator-credits") {
        translators_text.append(thisTranslation);
        translators_text.append("\n\n\n");
    }

    auto translators_filename = Glib::build_filename(INKSCAPE_DOCDIR, "TRANSLATORS");

    if (Glib::file_test (translators_filename, Glib::FILE_TEST_EXISTS)) {
        auto all_translators = Glib::file_get_contents(translators_filename);
        translators_text.append(all_translators);
    }

    set_translator_credits(translators_text);
}

void AboutBox::on_response(int response_id) {
    hide();
}
} // namespace Dialog
} // namespace UI
} // namespace Inkscape

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
