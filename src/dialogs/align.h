#ifndef __SP_QUICK_ALIGN_H__
#define __SP_QUICK_ALIGN_H__

/**
 * \brief  Object align dialog
 *
 * Authors:
 *   Frank Felfe <innerspace@iname.com>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */



#include <list>
#include <gtkmm/frame.h>
#include <gtkmm/tooltips.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/table.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/label.h>
#include "libnr/nr-dim2.h"
#include "libnr/nr-rect.h"

#include "dialogs/dockable.h"
#include "forward.h"

void sp_quick_align_dialog (void);

class Action;
class SPItem;

class DialogAlign : public Dockable{
public :
    static DialogAlign & get();
    virtual Gtk::Container & get_main_widget() {return _widget;}
    Gtk::Table &align_table(){return _alignTable;}
    Gtk::Table &distribute_table(){return _distributeTable;}
    Gtk::Table &nodes_table(){return _nodesTable;}
    Gtk::Tooltips &tooltips(){return _tooltips;}
    enum 
    AlignTarget{
        LAST = 0, FIRST , BIGGEST, SMALLEST, PAGE, DRAWING, SELECTION
    };
    AlignTarget getAlignTarget();
    std::list<SPItem *>::iterator find_master(std::list <SPItem *> &list, bool horizontal);
    void setMode(bool nodeEdit);

    NR::Rect randomize_bbox;
    bool randomize_bbox_set;

private :
    DialogAlign();    
    void on_ref_change();
    void addDistributeButton(const Glib::ustring &id, const Glib::ustring tiptext, 
                                      guint row, guint col, bool onInterSpace, 
                                      NR::Dim2 orientation, float kBegin, float kEnd);
    void addAlignButton(const Glib::ustring &id, const Glib::ustring tiptext, 
                        guint row, guint col);
    void addNodeButton(const Glib::ustring &id, const Glib::ustring tiptext, 
                        guint col, NR::Dim2 orientation, bool distribute);
    void addUnclumpButton(const Glib::ustring &id, const Glib::ustring tiptext, 
                        guint row, guint col);
    void addRandomizeButton(const Glib::ustring &id, const Glib::ustring tiptext, 
                        guint row, guint col);
    void addBaselineButton(const Glib::ustring &id, const Glib::ustring tiptext,
                           guint row, guint col, Gtk::Table &table, NR::Dim2 orientation, bool distribute);
    
    virtual ~DialogAlign();
   
    std::list<Action *> _actionList;
     
    Gtk::VBox _widget;
    Gtk::Frame _alignFrame, _distributeFrame, _nodesFrame;
    Gtk::Table _alignTable, _distributeTable, _nodesTable;
    Gtk::HBox _anchorBox;
    Gtk::VBox _alignBox;
    Gtk::Label _anchorLabel;
    Gtk::ComboBoxText _combo;
    Gtk::Tooltips _tooltips;
};




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
