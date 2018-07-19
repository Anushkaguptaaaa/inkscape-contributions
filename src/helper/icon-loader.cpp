/*
 * Icon Loader
 *
 * Icon Loader management code
 *
 * Authors:
 *  Jabiertxo Arraiza <jabier.arraiza@marker.es>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "icon-loader.h"
#include "inkscape.h"
#include "io/resource.h"
#include "preferences.h"
#include "svg/svg-color.h"
#include "widgets/toolbox.h"
#include <gtkmm/iconinfo.h>
#include <gtkmm/icontheme.h>

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gint size)
{
    using namespace Inkscape::IO::Resource;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    if (prefs->getString("/theme/themeName") == "") {
        prefs->setString("/theme/themeName", "hicolor");
    }
    //TODO: remove this fixed value by a theme setting in prefs or wereeber might to be
    prefs->setBool("/theme/symbolicIcons", false);
    prefs->setString("/theme/symbolicIcons", "hicolor");
    prefs->setInt("/theme/symbolicColor", 0x000000ff);
    //end of remove
    auto iconTheme = Gtk::IconTheme::create();
    iconTheme->set_custom_theme(prefs->getString("/theme/themeName"));
    iconTheme->append_search_path(get_path_ustring(SYSTEM, ICONS));
    iconTheme->append_search_path(get_path_ustring(USER, ICONS));
#ifdef INKSCAPE_THEMEDIR
    iconTheme->append_search_path(get_path_ustring(SYSTEM, THEMES));
    iconTheme->append_search_path(get_path_ustring(USER, THEMES));
#endif
    Glib::RefPtr<Gdk::Pixbuf> _icon_pixbuf;
    try {
//        if (prefs->getBool("/theme/symbolicIcons", false)) {
//            gchar colornamed[64];
//            sp_svg_write_color(colornamed, sizeof(colornamed), prefs->getInt("/theme/symbolicColor", 0x000000ff));
//            Gdk::RGBA color;
//            color.set(colornamed);
//            Gtk::IconInfo iconinfo =
//                iconTheme->lookup_icon(icon_name + Glib::ustring("-symbolic"), size, Gtk::ICON_LOOKUP_FORCE_SIZE);
//            if (bool(iconinfo)) {
//                // TODO: view if we need parametrice other colors
//                bool was_symbolic = false;
//                _icon_pixbuf = iconinfo.load_symbolic(color, color, color, color, was_symbolic);
//            }
//            else {
//                _icon_pixbuf = iconTheme->load_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
//            }
//        }
//        else {
            _icon_pixbuf = iconTheme->load_icon(icon_name, size, Gtk::ICON_LOOKUP_FORCE_SIZE);
//        }
    }
    catch (const Gtk::IconThemeError &e) {
        std::cout << "Icon Loader: " << e.what() << std::endl;
    }
    return _icon_pixbuf;
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size)
{
    int width, height;
    Gtk::IconSize::lookup(Gtk::IconSize(icon_size), width, height);
    return sp_get_icon_pixbuf(icon_name, width);
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, GtkIconSize icon_size)
{
    gint width, height;
    gtk_icon_size_lookup(icon_size, &width, &height);
    return sp_get_icon_pixbuf(icon_name, width);
}

Glib::RefPtr<Gdk::Pixbuf> sp_get_icon_pixbuf(Glib::ustring icon_name, gchar const *prefs_size)
{
    // Load icon based in preference size defined allowed values are:
    //"/toolbox/tools/small" Toolbox icon size
    //"/toolbox/small" Control bar icon size
    //"/toolbox/secondary" Secondary toolbar icon size
    GtkIconSize icon_size = Inkscape::UI::ToolboxFactory::prefToSize(prefs_size);
    return sp_get_icon_pixbuf(icon_name, icon_size);
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gint size)
{
    auto icon = sp_get_icon_pixbuf(icon_name, size);
    Gtk::Image *image = new Gtk::Image(icon);
    return image;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, Gtk::BuiltinIconSize icon_size)
{
    auto icon = sp_get_icon_pixbuf(icon_name, icon_size);
    Gtk::Image *image = new Gtk::Image(icon);
    return image;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, GtkIconSize icon_size)
{
    auto icon = sp_get_icon_pixbuf(icon_name, icon_size);
    Gtk::Image *image = new Gtk::Image(icon);
    return image;
}

Gtk::Image *sp_get_icon_image(Glib::ustring icon_name, gchar const *prefs_size)
{
    auto icon = sp_get_icon_pixbuf(icon_name, prefs_size);
    Gtk::Image *image = new Gtk::Image(icon);
    return image;
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
