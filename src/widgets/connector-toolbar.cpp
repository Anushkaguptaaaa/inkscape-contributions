/**
 * @file
 * Connector aux toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtkmm.h>
#include <glibmm.h>
#include <glibmm/i18n.h>

#include "connector-toolbar.h"
#include "conn-avoid-ref.h"

#include "desktop.h"
#include "document-undo.h"
#include "enums.h"
#include "graphlayout.h"
#include "ink-toggle-action.h"
#include "inkscape.h"
#include "toolbox.h"
#include "verbs.h"

#include "object/sp-namedview.h"
#include "object/sp-path.h"

#include "ui/icon-names.h"
#include "ui/tools/connector-tool.h"
#include "ui/uxmanager.h"
#include "ui/widget/spin-button-tool-item.h"

#include "widgets/ege-adjustment-action.h"
#include "widgets/spinbutton-events.h"

#include "xml/node-event-vector.h"

using Inkscape::UI::UXManager;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;

//#########################
//##      Connector      ##
//#########################

static void connector_spacing_changed(GtkAdjustment *adj, GObject* tbl)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data( tbl, "desktop" ));
    SPDocument *doc = desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }

    Inkscape::XML::Node *repr = desktop->namedview->getRepr();

    if ( !repr->attribute("inkscape:connector-spacing") &&
            ( gtk_adjustment_get_value(adj) == defaultConnSpacing )) {
        // Don't need to update the repr if the attribute doesn't
        // exist and it is being set to the default value -- as will
        // happen at startup.
        return;
    }

    // quit if run by the attr_changed listener
    if (g_object_get_data( tbl, "freeze" )) {
        return;
    }

    // in turn, prevent listener from responding
    g_object_set_data( tbl, "freeze", GINT_TO_POINTER(TRUE));

    sp_repr_set_css_double(repr, "inkscape:connector-spacing", gtk_adjustment_get_value(adj));
    desktop->namedview->updateRepr();
    bool modmade = false;

    std::vector<SPItem *> items;
    items = get_avoided_items(items, desktop->currentRoot(), desktop);
    for (std::vector<SPItem *>::const_iterator iter = items.begin(); iter != items.end(); ++iter ) {
        SPItem *item = *iter;
        Geom::Affine m = Geom::identity();
        avoid_item_move(&m, item);
        modmade = true;
    }

    if(modmade) {
        DocumentUndo::done(doc, SP_VERB_CONTEXT_CONNECTOR,
                       _("Change connector spacing"));
    }
    g_object_set_data( tbl, "freeze", GINT_TO_POINTER(FALSE) );
}

static void sp_connector_graph_layout(void)
{
    if (!SP_ACTIVE_DESKTOP) {
        return;
    }
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // hack for clones, see comment in align-and-distribute.cpp
    int saved_compensation = prefs->getInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);
    prefs->setInt("/options/clonecompensation/value", SP_CLONE_COMPENSATION_UNMOVED);

    auto tmp = SP_ACTIVE_DESKTOP->getSelection()->items();
    std::vector<SPItem *> vec(tmp.begin(), tmp.end());
    graphlayout(vec);

    prefs->setInt("/options/clonecompensation/value", saved_compensation);

    DocumentUndo::done(SP_ACTIVE_DESKTOP->getDocument(), SP_VERB_DIALOG_ALIGN_DISTRIBUTE, _("Arrange connector network"));
}

static void sp_directed_graph_layout_toggled( GtkToggleAction* act, GObject * /*tbl*/ )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/connector/directedlayout",
                gtk_toggle_action_get_active( act ));
}

static void sp_nooverlaps_graph_layout_toggled( GtkToggleAction* act, GObject * /*tbl*/ )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setBool("/tools/connector/avoidoverlaplayout",
                gtk_toggle_action_get_active( act ));
}


static void connector_length_changed(GtkAdjustment *adj, GObject* /*tbl*/)
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setDouble("/tools/connector/length", gtk_adjustment_get_value(adj));
}

static void connector_tb_event_attr_changed(Inkscape::XML::Node *repr,
                                            gchar const *name, gchar const * /*old_value*/, gchar const * /*new_value*/,
                                            bool /*is_interactive*/, gpointer data)
{
    GtkWidget *tbl = GTK_WIDGET(data);

    if ( !g_object_get_data(G_OBJECT(tbl), "freeze")
         && (strcmp(name, "inkscape:connector-spacing") == 0) ) {
        GtkAdjustment *adj = static_cast<GtkAdjustment*>(g_object_get_data(G_OBJECT(tbl), "spacing"));
        gdouble spacing = defaultConnSpacing;
        sp_repr_get_double(repr, "inkscape:connector-spacing", &spacing);

        gtk_adjustment_set_value(adj, spacing);

#if !GTK_CHECK_VERSION(3,18,0)
        gtk_adjustment_value_changed(adj);
#endif

        spinbutton_defocus(tbl);
    }
}

static Inkscape::XML::NodeEventVector connector_tb_repr_events = {
    NULL, /* child_added */
    NULL, /* child_removed */
    connector_tb_event_attr_changed,
    NULL, /* content_changed */
    NULL  /* order_changed */
};

static void sp_connector_toolbox_selection_changed(Inkscape::Selection *selection, GObject *tbl)
{
    GtkAdjustment *adj = GTK_ADJUSTMENT( g_object_get_data( tbl, "curvature" ) );
    GtkToggleAction *act = GTK_TOGGLE_ACTION( g_object_get_data( tbl, "orthogonal" ) );
    SPItem *item = selection->singleItem();
    if (SP_IS_PATH(item))
    {
        gdouble curvature = SP_PATH(item)->connEndPair.getCurvature();
        bool is_orthog = SP_PATH(item)->connEndPair.isOrthogonal();
        gtk_toggle_action_set_active(act, is_orthog);
        gtk_adjustment_set_value(adj, curvature);
    }

}

void sp_connector_toolbox_prep( SPDesktop *desktop, GtkActionGroup* mainActions, GObject* holder )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    GtkIconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);
    auto toolbar = GTK_TOOLBAR(holder);

    EgeAdjustmentAction* eact = 0;
    // Curvature spinbox
#if 0
    eact = create_adjustment_action( "ConnectorCurvatureAction",
                                    _("Connector Curvature"), _("Curvature:"),
                                    _("The amount of connectors curvature"),
                                    "/tools/connector/curvature", defaultConnCurvature,
                                    GTK_WIDGET(desktop->canvas), holder, TRUE, "inkscape:connector-curvature",
                                    0, 100, 1.0, 10.0,
                                    0, 0, 0,
                                    connector_curvature_changed, NULL /*unit tracker*/, 1, 0 );
    gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
#endif

    // Spacing spinbox
    eact = create_adjustment_action( "ConnectorSpacingAction",
                                    _("Connector Spacing"), _("Spacing:"),
                                    _("The amount of space left around objects by auto-routing connectors"),
                                    "/tools/connector/spacing", defaultConnSpacing,
                                    GTK_WIDGET(desktop->canvas), holder, TRUE, "inkscape:connector-spacing",
                                    0, 100, 1.0, 10.0,
                                    0, 0, 0,
                                    connector_spacing_changed, NULL /*unit tracker*/, 1, 0 );
    gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );

    // Graph (connector network) layout
    {
        GtkAction* inky = gtk_action_new( "ConnectorGraphAction",
                                          _("Graph"),
                                          _("Nicely arrange selected connector network"),
                                          NULL);
        gtk_action_set_icon_name(inky, INKSCAPE_ICON("distribute-graph"));
        g_signal_connect_after( G_OBJECT(inky), "activate", G_CALLBACK(sp_connector_graph_layout), holder );
        gtk_action_group_add_action( mainActions, GTK_ACTION(inky) );
    }

    // Default connector length spinbox
    eact = create_adjustment_action( "ConnectorLengthAction",
                                     _("Connector Length"), _("Length:"),
                                     _("Ideal length for connectors when layout is applied"),
                                     "/tools/connector/length", 100,
                                     GTK_WIDGET(desktop->canvas), holder, TRUE, "inkscape:connector-length",
                                     10, 1000, 10.0, 100.0,
                                     0, 0, 0,
                                     connector_length_changed, NULL /*unit tracker*/, 1, 0 );
    gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );


    // Directed edges toggle button
    {
        InkToggleAction* act = ink_toggle_action_new( "ConnectorDirectedAction",
                                                      _("Downwards"),
                                                      _("Make connectors with end-markers (arrows) point downwards"),
                                                      INKSCAPE_ICON("distribute-graph-directed"),
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );

        bool tbuttonstate = prefs->getBool("/tools/connector/directedlayout");
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), ( tbuttonstate ? TRUE : FALSE ));

        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(sp_directed_graph_layout_toggled), holder );
        desktop->getSelection()->connectChanged(sigc::bind(sigc::ptr_fun(sp_connector_toolbox_selection_changed), holder));
    }

    // Avoid overlaps toggle button
    {
        InkToggleAction* act = ink_toggle_action_new( "ConnectorOverlapAction",
                                                      _("Remove overlaps"),
                                                      _("Do not allow overlapping shapes"),
                                                      INKSCAPE_ICON("distribute-remove-overlaps"),
                                                      GTK_ICON_SIZE_MENU );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );

        bool tbuttonstate = prefs->getBool("/tools/connector/avoidoverlaplayout");
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), (tbuttonstate ? TRUE : FALSE ));

        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(sp_nooverlaps_graph_layout_toggled), holder );
    }


    // Code to watch for changes to the connector-spacing attribute in
    // the XML.
    Inkscape::XML::Node *repr = desktop->namedview->getRepr();
    g_assert(repr != NULL);

    purge_repr_listener( holder, holder );

    if (repr) {
        g_object_set_data( holder, "repr", repr );
        Inkscape::GC::anchor(repr);
        sp_repr_add_listener( repr, &connector_tb_repr_events, holder );
        sp_repr_synthesize_events( repr, &connector_tb_repr_events, holder );
    }
} // end of sp_connector_toolbox_prep()

namespace Inkscape {
namespace UI {
namespace Widget {

ConnectorToolbar::ConnectorToolbar(SPDesktop *desktop)
    : _desktop(desktop),
      _orthogonal_button(Gtk::manage(new Gtk::ToggleToolButton())),
      _freeze_flag(false)
{
    // Create a new action group to describe all the actions that relate to tools
    // in this toolbar.  All actions will have the "connector-actions" prefix
    auto action_group = Gio::SimpleActionGroup::create();
    insert_action_group("connector-actions", action_group);

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    auto secondarySize = static_cast<Gtk::IconSize>(ToolboxFactory::prefToSize("/toolbox/secondary", 1));

    action_group->add_action("avoid",      sigc::mem_fun(*this, &ConnectorToolbar::on_avoid_activated));
    action_group->add_action("ignore",     sigc::mem_fun(*this, &ConnectorToolbar::on_ignore_activated));
    action_group->add_action("orthogonal", sigc::mem_fun(*this, &ConnectorToolbar::on_orthogonal_activated));

    /*******************************************/
    /**** Toolbutton for the "avoid" action ****/
    /*******************************************/
    auto avoid_icon   = Gtk::manage(new Gtk::Image());
    avoid_icon->set_from_icon_name(INKSCAPE_ICON("connector-avoid"), secondarySize);
    auto avoid_button = Gtk::manage(new Gtk::ToolButton(*avoid_icon, _("Avoid")));
    avoid_button->set_tooltip_text(_("Make connectors avoid selected objects"));
    gtk_actionable_set_action_name(GTK_ACTIONABLE(avoid_button->gobj()), "connector-actions.avoid");

    /********************************************/
    /**** Toolbutton for the "ignore" action ****/
    /********************************************/
    auto ignore_icon   = Gtk::manage(new Gtk::Image());
    ignore_icon->set_from_icon_name(INKSCAPE_ICON("connector-ignore"), secondarySize);
    auto ignore_button = Gtk::manage(new Gtk::ToolButton(*ignore_icon, _("Ignore")));
    ignore_button->set_tooltip_text(_("Make connectors ignore selected objects"));
    gtk_actionable_set_action_name(GTK_ACTIONABLE(ignore_button->gobj()),
                                   "connector-actions.ignore");

    /*******************************************************/
    /**** Toggle toolbutton for the "orthogonal" action ****/
    /*******************************************************/
    auto orthogonal_icon   = Gtk::manage(new Gtk::Image());
    orthogonal_icon->set_from_icon_name(INKSCAPE_ICON("connector-orthogonal"), Gtk::ICON_SIZE_MENU);
    _orthogonal_button->set_label(_("Orthogonal"));
    _orthogonal_button->set_icon_widget(*orthogonal_icon);
    _orthogonal_button->set_tooltip_text(_("Make connector orthogonal or polyline"));
    bool tbuttonstate = prefs->getBool("/tools/connector/orthogonal");
    _orthogonal_button->set_active(tbuttonstate);
    gtk_actionable_set_action_name(GTK_ACTIONABLE(_orthogonal_button->gobj()),
                                   "connector-actions.orthogonal");

    /*******************/
    /**** Separator ****/
    /*******************/
    auto separator = Gtk::manage(new Gtk::SeparatorToolItem());

    /*******************************/
    /**** Curvature spin-button ****/
    /*******************************/
    auto curvature_value = prefs->getDouble("/tools/connector/curvature", defaultConnCurvature);
    _curvature_adj = Gtk::Adjustment::create(curvature_value, 0, 100);
    auto curvature_adj_value_changed_cb = sigc::mem_fun(*this, &ConnectorToolbar::on_curvature_adj_value_changed);
    _curvature_adj->signal_value_changed().connect(curvature_adj_value_changed_cb);

    auto curvature_sb = Gtk::manage(new Inkscape::UI::Widget::SpinButtonToolItem(_("Curvature"),
                                                                                 _curvature_adj));
    curvature_sb->set_all_tooltip_text(_("The amount of connectors curvature"));

    // Append all widgets to toolbar
    append(*avoid_button);
    append(*ignore_button);
    append(*_orthogonal_button);
    append(*separator);
    append(*curvature_sb);
    show_all();
}

GtkWidget *
ConnectorToolbar::create(SPDesktop *desktop)
{
    auto connector_toolbar = Gtk::manage(new ConnectorToolbar(desktop));
    return GTK_WIDGET(connector_toolbar->gobj());
}

void
ConnectorToolbar::on_avoid_activated()
{
    Inkscape::UI::Tools::cc_selection_set_avoid(true);
}

void
ConnectorToolbar::on_ignore_activated()
{
    Inkscape::UI::Tools::cc_selection_set_avoid(false);
}

void
ConnectorToolbar::on_orthogonal_activated()
{
    SPDocument *doc = _desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }

    // quit if run by the _changed callbacks
    if (_freeze_flag) {
        return;
    }

    // in turn, prevent callbacks from responding
    _freeze_flag = true;

    // Get the toolbutton state
    bool is_orthog = _orthogonal_button->get_active();
    gchar orthog_str[] = "orthogonal";
    gchar polyline_str[] = "polyline";
    gchar *value = is_orthog ? orthog_str : polyline_str ;

    bool modmade = false;
    auto itemlist= _desktop->getSelection()->items();
    for(auto i=itemlist.begin();i!=itemlist.end();++i){
        SPItem *item = *i;

        if (Inkscape::UI::Tools::cc_item_is_connector(item)) {
            item->setAttribute( "inkscape:connector-type",
                    value, NULL);
            item->avoidRef->handleSettingChange();
            modmade = true;
        }
    }

    if (!modmade) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setBool("/tools/connector/orthogonal", is_orthog);
    } else {

        DocumentUndo::done(doc, SP_VERB_CONTEXT_CONNECTOR,
                       is_orthog ? _("Set connector type: orthogonal"): _("Set connector type: polyline"));
    }

    _freeze_flag = false;
}

void
ConnectorToolbar::on_curvature_adj_value_changed()
{
    SPDocument *doc = _desktop->getDocument();

    if (!DocumentUndo::getUndoSensitive(doc)) {
        return;
    }


    // quit if run by the _changed callbacks
    if (_freeze_flag) {
        return;
    }

    // in turn, prevent callbacks from responding
    _freeze_flag = true;

    gdouble newValue = _curvature_adj->get_value();
    gchar value[G_ASCII_DTOSTR_BUF_SIZE];
    g_ascii_dtostr(value, G_ASCII_DTOSTR_BUF_SIZE, newValue);

    bool modmade = false;
    auto itemlist= _desktop->getSelection()->items();
    for(auto item : itemlist){
        if (Inkscape::UI::Tools::cc_item_is_connector(item)) {
            item->setAttribute( "inkscape:connector-curvature",
                    value, NULL);
            item->avoidRef->handleSettingChange();
            modmade = true;
        }
    }

    if (!modmade) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setDouble(Glib::ustring("/tools/connector/curvature"), newValue);
    }
    else {
        DocumentUndo::done(doc, SP_VERB_CONTEXT_CONNECTOR,
                       _("Change connector curvature"));
    }

    _freeze_flag = false;
}


}
}
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :
