// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * @brief A dialog for the start screen
 */
/*
 * Copyright (C) Martin Owens 2020 <doctormo@gmail.com>
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef STARTSCREEN_H
#define STARTSCREEN_H

#include <string>             // for string
#include <gdk/gdk.h>          // for GdkModifierType
#include <glibmm/refptr.h>    // for RefPtr
#include <glibmm/timer.h>     // for Timer
#include <glibmm/ustring.h>   // for ustring
#include <gtk/gtk.h>          // for GtkEventControllerKey
#include <gtkmm/dialog.h>     // for Dialog
#include <gtkmm/treemodel.h>  // for TreeModel
#include <sigc++/scoped_connection.h>

#include "ui/widget/template-list.h"

namespace Gtk {
class Builder;
class Button;
class ComboBox;
class Label;
class Notebook;
class Overlay;
class TreeView;
class Widget;
} // namespace Gtk

class SPDocument;

namespace Inkscape::UI::Dialog {

class StartScreen : public Gtk::Dialog {
public:
    StartScreen();
    ~StartScreen() override;

    SPDocument* get_document() { return _document; }

    void show_now();
    void show_welcome();

    static int get_start_mode();
protected:
    void on_response(int response_id) override;

private:
    SPDocument* get_template_document();

    void notebook_next(Gtk::Widget *button);
    bool on_key_pressed(unsigned keyval, unsigned keycode, Gdk::ModifierType state);
    Gtk::TreeModel::Row active_combo(std::string widget_name);
    void set_active_combo(std::string widget_name, std::string unique_id);
    void show_toggle();
    void refresh_keys_warning();
    void enlist_recent_files();
    void enlist_keys();
    void filter_themes(Gtk::ComboBox *themes);
    void keyboard_changed();
    void banner_switch(unsigned page_num);

    void theme_changed();
    void canvas_changed();
    void refresh_theme(Glib::ustring theme_name);
    void refresh_dark_switch();

    void new_document();
    void load_document();
    void on_recent_changed();
    void on_kind_changed(const Glib::ustring& name);

    const std::string opt_shown;
    Glib::Timer _timer;

    Glib::RefPtr<Gtk::Builder> build_splash;
    Gtk::Overlay &banners;
    Gtk::Button &close_btn;
    Gtk::Label &messages;
    Inkscape::UI::Widget::TemplateList templates;

    Glib::RefPtr<Gtk::Builder> build_welcome;
    Gtk::TreeView *recentfiles = nullptr;

    SPDocument* _document = nullptr;
    bool _welcome = false;

    sigc::scoped_connection _tabs_switch_page_conn;
    sigc::scoped_connection _templates_switch_page_conn;
};

} // namespace Inkscape::UI::Dialog

#endif // STARTSCREEN_H

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
