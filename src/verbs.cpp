#define __SP_VERBS_C__
/**
 * \file verbs.cpp
 *
 * \brief Actions for inkscape
 *
 * This file implements routines necessary to deal with verbs.  A verb
 * is a numeric identifier used to retrieve standard SPActions for particular
 * views.
 */

/*
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Ted Gould <ted@gould.cx>
 *   MenTaLguY <mental@rydia.net>
 *   David Turner <novalis@gnu.org>
 *
 * This code is in public domain.
 */


#include <cstddef>

#include <assert.h>

#include <gtk/gtkstock.h>

#include <config.h>

#include "helper/action.h"
#include <glibmm/i18n.h>

#include "dialogs/text-edit.h"
#include "dialogs/export.h"
#include "dialogs/xml-tree.h"
#include "dialogs/align.h"
#include "dialogs/transformation.h"
#include "dialogs/object-properties.h"
#include "dialogs/desktop-properties.h"
#include "dialogs/display-settings.h"
#include "dialogs/item-properties.h"
#include "dialogs/find.h"
#include "dialogs/debugdialog.h"
#include "dialogs/scriptdialog.h"
#include "dialogs/tracedialog.h"
#include "dialogs/layer-properties.h"
#include "dialogs/clonetiler.h"

#include "extension/effect.h"

#include "tools-switch.h"
#include "inkscape-private.h"
#include "file.h"
#include "help.h"
#include "document.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "selection.h"
#include "selection-chemistry.h"
#include "path-chemistry.h"
#include "text-chemistry.h"
#include "shortcuts.h"
#include "toolbox.h"
#include "view.h"
#include "interface.h"
#include "prefs-utils.h"
#include "splivarot.h"
#include "sp-namedview.h"
#include "sp-flowtext.h"
#include "layer-fns.h"
#include "node-context.h"
#include "verbs.h"


/**
 * \brief Return the name without underscores and ellipsis, for use in dialog
 * titles, etc. Allocated memory must be freed by caller.
 */
gchar *
sp_action_get_title (const SPAction *action)
{
    char const *src = action->name;
    gchar *ret = g_new (gchar, strlen(src) + 1);
    unsigned ri = 0;

    for (unsigned si = 0 ; ; si++)  {
        int const c = src[si];
        if ( c != '_' && c != '.' ) {
            ret[ri] = c;
            ri++;
            if (c == '\0') {
                return ret;
            }
        }
    }

} // end of sp_action_get_title()


namespace Inkscape {

/** \brief A class to encompass all of the verbs which deal with
           file operations. */
class FileVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    FileVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* FileVerb class */

/** \brief A class to encompass all of the verbs which deal with
           edit operations. */
class EditVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    EditVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* EditVerb class */

/** \brief A class to encompass all of the verbs which deal with
           selection operations. */
class SelectionVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    SelectionVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* SelectionVerb class */

/** \brief A class to encompass all of the verbs which deal with
           layer operations. */
class LayerVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    LayerVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* LayerVerb class */

/** \brief A class to encompass all of the verbs which deal with
           operations related to objects. */
class ObjectVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    ObjectVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* ObjectVerb class */

/** \brief A class to encompass all of the verbs which deal with
           operations relative to context. */
class ContextVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    ContextVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* ContextVerb class */

/** \brief A class to encompass all of the verbs which deal with
           zoom operations. */
class ZoomVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    ZoomVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* ZoomVerb class */

/** \brief A class to encompass all of the verbs which deal with
           dialog operations. */
class DialogVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    DialogVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* DialogVerb class */

/** \brief A class to encompass all of the verbs which deal with
           help operations. */
class HelpVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    HelpVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* HelpVerb class */

/** \brief A class to encompass all of the verbs which deal with
           tutorial operations. */
class TutorialVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    TutorialVerb(const unsigned int code,
             gchar const * id,
             gchar const * name,
             gchar const * tip,
             gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* TutorialVerb class */


Verb::VerbTable Verb::_verbs;

/** \brief  Create a verb without a code.

    This function calls the other constructor for all of the parameters,
    but generates the code.  It is important to READ THE OTHER DOCUMENTATION
    it has important details in it.  To generate the code a static is
    used which starts at the last static value: \c SP_VERB_LAST.  For
    each call it is incremented.  The list of allocated verbs is kept
    in the \c _verbs hashtable which is indexed by the \c code.
*/
Verb::Verb(gchar const * id, gchar const * name, gchar const * tip, gchar const * image) :
        _actions(NULL), _id(id), _name(name), _tip(tip), _image(image)
{
    static int count = SP_VERB_LAST;

    count++;
    _code = count;
    _verbs.insert(VerbTable::value_type(count, this));

    return;
}

/** \brief  Destroy a verb.

      The only allocated variable is the _actions variable.  If it has
    been allocated it is deleted.
*/
Verb::~Verb (void)
{
    /** \todo all the actions need to be cleaned up first */
    if (_actions != NULL) {
        delete _actions;
    }

    return;
}

/** \brief  Verbs are no good without actions.  This is a place holder
            for a function that every subclass should write.  Most
            can be written using \c make_action_helper.
    \param  view  Which view the action should be created for.
    \return NULL to represent error (this function shouldn't ever be called)
*/
SPAction *
Verb::make_action (SPView * view)
{
//    std::cout << "make_action" << std::endl;
    return NULL;
}

/** \brief  Create an action for a \c FileVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
FileVerb::make_action (SPView * view)
{
//    std::cout << "fileverb: make_action: " << &vector << std::endl;
    return make_action_helper(view, &vector);
}

/** \brief  Create an action for a \c EditVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
EditVerb::make_action (SPView * view)
{
//    std::cout << "editverb: make_action: " << &vector << std::endl;
    return make_action_helper(view, &vector);
}

/** \brief  Create an action for a \c SelectionVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
SelectionVerb::make_action (SPView * view)
{
    return make_action_helper(view, &vector);
}

/** \brief  Create an action for a \c LayerVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
LayerVerb::make_action (SPView * view)
{
    return make_action_helper(view, &vector);
}

/** \brief  Create an action for a \c ObjectVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
ObjectVerb::make_action (SPView * view)
{
    return make_action_helper(view, &vector);
}

/** \brief  Create an action for a \c ContextVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
ContextVerb::make_action (SPView * view)
{
    return make_action_helper(view, &vector);
}

/** \brief  Create an action for a \c ZoomVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
ZoomVerb::make_action (SPView * view)
{
    return make_action_helper(view, &vector);
}

/** \brief  Create an action for a \c DialogVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
DialogVerb::make_action (SPView * view)
{
    return make_action_helper(view, &vector);
}

/** \brief  Create an action for a \c HelpVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
HelpVerb::make_action (SPView * view)
{
    return make_action_helper(view, &vector);
}

/** \brief  Create an action for a \c TutorialVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
TutorialVerb::make_action (SPView * view)
{
    return make_action_helper(view, &vector);
}

/** \brief A quick little convience function to make building actions
           a little bit easier.
    \param  view    Which view the action should be created for.
    \param  vector  The function vector for the verb.
    \return The created action.

    This function does a couple of things.  The most obvious is that
    it allocates and creates the action.  When it does this it
    translates the \c _name and \c _tip variables.  This allows them
    to be staticly allocated easily, and get translated in the end.  Then,
    if the action gets crated, a listener is added to the action with
    the vector that is passed in.
*/
SPAction *
Verb::make_action_helper (SPView * view, SPActionEventVector * vector, void * in_pntr)
{
    SPAction *action;
    
    //std::cout << "Adding action: " << _code << std::endl;
    action = sp_action_new(view, _id, _(_name),
                           _(_tip), _image, this);

    if (action != NULL) {
        if (in_pntr == NULL) {
            nr_active_object_add_listener (
                (NRActiveObject *) action,
                (NRObjectEventVector *) vector,
                sizeof (SPActionEventVector),
                reinterpret_cast<void *>(_code)
            );
        } else {
            nr_active_object_add_listener (
                (NRActiveObject *) action,
                (NRObjectEventVector *) vector,
                sizeof (SPActionEventVector),
                in_pntr
            );
        }
    }

    return action;
}

/** \brief  A function to get an action if it exists, or otherwise to
            build it.
    \param  view  The view which this action would relate to
    \return The action, or NULL if there is an error.

    This function will get the action for a given view for this verb.  It
    will create the verb if it can't be found in the ActionTable.  Also,
    if the \c ActionTable has not been created, it gets created by this
    function.
*/
SPAction *
Verb::get_action (SPView * view)
{
    SPAction * action = NULL;

    if (_actions == NULL) {
        _actions = new ActionTable;
    }
    ActionTable::iterator action_found = _actions->find(view);

    if (action_found != _actions->end()) {
        action = action_found->second;
    } else {
        action = this->make_action(view);
        // if (action == NULL) printf("Hmm, NULL in %s\n", _name);
        _actions->insert(ActionTable::value_type(view, action));
    }

    return action;
}

/** \brief  A function to remove the action associated with a view.
    \param  view  Which view's actions should be removed.
    \return None

    This function looks for the action in \c _actions.  If it is
    found then it is unreferenced and the entry in the action
    table is erased.
*/
void
Verb::delete_view (SPView * view)
{
    if (_actions == NULL) return;
    if (_actions->empty()) return;

#if 0
    static int count = 0;
    std::cout << count++ << std::endl;
#endif

    ActionTable::iterator action_found = _actions->find(view);

    if (action_found != _actions->end()) {
        SPAction * action = action_found->second;
        nr_object_unref(NR_OBJECT(action));
        _actions->erase(action_found);
    }

    return;
}

/** \brief  A function to delete a view from all verbs
    \param  view  Which view's actions should be removed.
    \return None

    This function first looks through _base_verbs and deteles
    the view from all of those views.  If \c _verbs is not empty
    then all of the entries in that table have all of the views
    deleted also.
*/
void
Verb::delete_all_view (SPView * view)
{
    for (int i = 0; i <= SP_VERB_LAST; i++) {
        _base_verbs[i]->delete_view(view);
    }

    if (!_verbs.empty()) {
        for (VerbTable::iterator thisverb = _verbs.begin();
             thisverb != _verbs.end(); thisverb++) {
            Inkscape::Verb * verbpntr = thisverb->second;
            // std::cout << "Delete In Verb: " << verbpntr->_name << std::endl;
            verbpntr->delete_view(view);
        }
    }

    return;
}

/** \brief  A function to turn a \c code into a Verb for dynamically
            created Verbs.
    \param  code  What code is being looked for
    \return The found Verb of NULL if none is found.

    This function basically just looks through the \c _verbs hash
    table.  STL does all the work.
*/
Verb *
Verb::get_search (unsigned int code)
{
    Verb * verb = NULL;
    VerbTable::iterator verb_found = _verbs.find(code);

    if (verb_found != _verbs.end()) {
        verb = verb_found->second;
    }

    return verb;
}

/** \brief  Decode the verb code and take appropriate action */
void
FileVerb::perform (SPAction *action, void * data, void *pdata)
{
#if 0
    /* These aren't used, but are here to remind people not to use
       the CURRENT_DOCUMENT macros unless they really have to. */
    SPView * current_view = sp_action_get_view(action);
    SPDocument * current_document = SP_VIEW_DOCUMENT(current_view);
#endif
    switch ((long) data) {
        case SP_VERB_FILE_NEW:
            sp_file_new_default ();
            break;
        case SP_VERB_FILE_OPEN:
            sp_file_open_dialog (NULL, NULL);
            break;
        case SP_VERB_FILE_REVERT:
            sp_file_revert_dialog ();
            break;
        case SP_VERB_FILE_SAVE:
            sp_file_save (NULL, NULL);
            break;
        case SP_VERB_FILE_SAVE_AS:
            sp_file_save_as (NULL, NULL);
            break;
        case SP_VERB_FILE_PRINT:
            sp_file_print ();
            break;
        case SP_VERB_FILE_VACUUM:
            sp_file_vacuum ();
            break;
        case SP_VERB_FILE_PRINT_DIRECT:
            sp_file_print_direct ();
            break;
        case SP_VERB_FILE_PRINT_PREVIEW:
            sp_file_print_preview (NULL, NULL);
            break;
        case SP_VERB_FILE_IMPORT:
            sp_file_import (NULL);
            break;
        case SP_VERB_FILE_EXPORT:
            sp_file_export_dialog (NULL);
            break;
        case SP_VERB_FILE_NEXT_DESKTOP:
            inkscape_switch_desktops_next();
            break;
        case SP_VERB_FILE_PREV_DESKTOP:
            inkscape_switch_desktops_prev();
            break;
        case SP_VERB_FILE_CLOSE_VIEW:
            sp_ui_close_view (NULL);
            break;
        case SP_VERB_FILE_QUIT:
            sp_file_exit ();
            break;
        default:
            break;
    }

} // end of sp_verb_action_file_perform()

/** \brief  Decode the verb code and take appropriate action */
void
EditVerb::perform (SPAction *action, void * data, void * pdata)
{
    SPDesktop *dt;
    SPEventContext *ec;

    dt = SP_DESKTOP (sp_action_get_view (action));
    if (!dt)
        return;

    ec = dt->event_context;

    switch (reinterpret_cast<std::size_t>(data)) {
        case SP_VERB_EDIT_UNDO:
            sp_undo (dt, SP_DT_DOCUMENT (dt));
            break;
        case SP_VERB_EDIT_REDO:
            sp_redo (dt, SP_DT_DOCUMENT (dt));
            break;
        case SP_VERB_EDIT_CUT:
            sp_selection_cut();
            break;
        case SP_VERB_EDIT_COPY:
            sp_selection_copy();
            break;
        case SP_VERB_EDIT_PASTE:
            sp_selection_paste(false);
            break;
        case SP_VERB_EDIT_PASTE_STYLE:
            sp_selection_paste_style();
            break;
        case SP_VERB_EDIT_PASTE_IN_PLACE:
            sp_selection_paste(true);
            break;
        case SP_VERB_EDIT_DELETE:
            sp_selection_delete();
            break;
        case SP_VERB_EDIT_DUPLICATE:
        sp_selection_duplicate();
            break;
        case SP_VERB_EDIT_CLONE:
            sp_selection_clone();
            break;
        case SP_VERB_EDIT_UNLINK_CLONE:
            sp_selection_unlink();
            break;
        case SP_VERB_EDIT_CLONE_ORIGINAL:
            sp_select_clone_original();
            break;
        case SP_VERB_EDIT_TILE:
        sp_selection_tile();
            break;
        case SP_VERB_EDIT_UNTILE:
        sp_selection_untile();
            break;
        case SP_VERB_EDIT_CLEAR_ALL:
            sp_edit_clear_all();
            break;
        case SP_VERB_EDIT_SELECT_ALL:
            if (tools_isactive (dt, TOOLS_NODES)) {
                sp_nodepath_select_all (SP_NODE_CONTEXT(ec)->nodepath);
            } else {
                sp_edit_select_all();
            }
            break;
        case SP_VERB_EDIT_INVERT:
            if (tools_isactive (dt, TOOLS_NODES)) {
                //FIXME: implement invert for nodes
            } else {
                sp_edit_invert();
            }
            break;
        case SP_VERB_EDIT_SELECT_ALL_IN_ALL_LAYERS:
            sp_edit_select_all_in_all_layers();
            break;
        case SP_VERB_EDIT_INVERT_IN_ALL_LAYERS:
            sp_edit_invert_in_all_layers();
            break;
        case SP_VERB_EDIT_DESELECT:
            if (tools_isactive (dt, TOOLS_NODES)) {
                sp_nodepath_deselect (SP_NODE_CONTEXT(ec)->nodepath);
            } else {
                SP_DT_SELECTION(dt)->clear();
            }
            break;
        default:
            break;
    }

} // end of sp_verb_action_edit_perform()

/** \brief  Decode the verb code and take appropriate action */
void
SelectionVerb::perform (SPAction *action, void * data, void * pdata)
{
    SPDesktop *dt;

    dt = SP_DESKTOP (sp_action_get_view (action));

    if (!dt)
        return;

    switch (reinterpret_cast<std::size_t>(data)) {
        case SP_VERB_SELECTION_TO_FRONT:
            sp_selection_raise_to_top();
            break;
        case SP_VERB_SELECTION_TO_BACK:
            sp_selection_lower_to_bottom();
            break;
        case SP_VERB_SELECTION_RAISE:
            sp_selection_raise();
            break;
        case SP_VERB_SELECTION_LOWER:
            sp_selection_lower();
            break;
        case SP_VERB_SELECTION_GROUP:
            sp_selection_group();
            break;
        case SP_VERB_SELECTION_UNGROUP:
            sp_selection_ungroup();
            break;

        case SP_VERB_SELECTION_TEXTTOPATH:
            text_put_on_path ();
            break;
        case SP_VERB_SELECTION_TEXTFROMPATH:
            text_remove_from_path ();
            break;
        case SP_VERB_SELECTION_REMOVE_KERNS:
            text_remove_all_kerns ();
            break;

        case SP_VERB_SELECTION_UNION:
            sp_selected_path_union ();
            break;
        case SP_VERB_SELECTION_INTERSECT:
            sp_selected_path_intersect ();
            break;
        case SP_VERB_SELECTION_DIFF:
            sp_selected_path_diff ();
            break;
        case SP_VERB_SELECTION_SYMDIFF:
            sp_selected_path_symdiff ();
            break;

        case SP_VERB_SELECTION_CUT:
            sp_selected_path_cut ();
            break;
        case SP_VERB_SELECTION_SLICE:
            sp_selected_path_slice ();
            break;

        case SP_VERB_SELECTION_OFFSET:
            sp_selected_path_offset ();
            break;
        case SP_VERB_SELECTION_OFFSET_SCREEN:
            sp_selected_path_offset_screen (1);
            break;
        case SP_VERB_SELECTION_OFFSET_SCREEN_10:
            sp_selected_path_offset_screen (10);
            break;
        case SP_VERB_SELECTION_INSET:
            sp_selected_path_inset ();
            break;
        case SP_VERB_SELECTION_INSET_SCREEN:
            sp_selected_path_inset_screen (1);
            break;
        case SP_VERB_SELECTION_INSET_SCREEN_10:
            sp_selected_path_inset_screen (10);
            break;
        case SP_VERB_SELECTION_DYNAMIC_OFFSET:
            sp_selected_path_create_offset_object_zero ();
            break;
        case SP_VERB_SELECTION_LINKED_OFFSET:
            sp_selected_path_create_updating_offset_object_zero ();
            break;

        case SP_VERB_SELECTION_OUTLINE:
            sp_selected_path_outline ();
            break;
        case SP_VERB_SELECTION_SIMPLIFY:
            sp_selected_path_simplify ();
            break;
        case SP_VERB_SELECTION_CLEANUP:
            sp_selection_cleanup ();
            break;
        case SP_VERB_SELECTION_REVERSE:
            sp_selected_path_reverse ();
            break;
        case SP_VERB_SELECTION_POTRACE:
            Inkscape::UI::Dialogs::TraceDialog::showInstance();
            break;
        case SP_VERB_SELECTION_CREATE_BITMAP:
            sp_selection_create_bitmap_copy ();
            break;

        case SP_VERB_SELECTION_COMBINE:
            sp_selected_path_combine ();
            break;
        case SP_VERB_SELECTION_BREAK_APART:
            sp_selected_path_break_apart ();
            break;
        default:
            break;
    }

} // end of sp_verb_action_selection_perform()

/** \brief  Decode the verb code and take appropriate action */
void
LayerVerb::perform (SPAction *action, void *data, void *pdata)
{
    SPDesktop *dt = SP_DESKTOP(sp_action_get_view(action));
    unsigned int verb=reinterpret_cast<std::size_t>(data);

    if ( !dt || !dt->currentLayer() ) {
        return;
    }

    switch (verb) {
        case SP_VERB_LAYER_NEW: {
            Inkscape::UI::Dialogs::LayerPropertiesDialog::showCreate(dt, dt->currentLayer());
            break;
        }
        case SP_VERB_LAYER_RENAME: {
            Inkscape::UI::Dialogs::LayerPropertiesDialog::showRename(dt, dt->currentLayer());
            break;
        }
        case SP_VERB_LAYER_NEXT: {
            SPObject *next=Inkscape::next_layer(dt->currentRoot(), dt->currentLayer());
            if (next) {
                dt->setCurrentLayer(next);
                // TODO move selected objects to top of next
                sp_document_done(SP_DT_DOCUMENT(dt));
                dt->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Moved to next layer."));
            } else {
                dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Can't move past last layer."));
            }
            break;
        }
        case SP_VERB_LAYER_PREV: {
            SPObject *prev=Inkscape::previous_layer(dt->currentRoot(), dt->currentLayer());
            if (prev) {
                dt->setCurrentLayer(prev);
                // TODO move selected objects to top of prev
                sp_document_done(SP_DT_DOCUMENT(dt));
                dt->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Moved to previous layer."));
            } else {
                dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Can't move past first layer."));
            }
            break;
        }
        case SP_VERB_LAYER_TO_TOP:
        case SP_VERB_LAYER_TO_BOTTOM:
        case SP_VERB_LAYER_RAISE:
        case SP_VERB_LAYER_LOWER: {
            if ( dt->currentLayer() == dt->currentRoot() ) {
                dt->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("No current layer."));
                return;
            }

            SPItem *layer=SP_ITEM(dt->currentLayer());
            g_return_if_fail(layer != NULL);

            SPObject *old_pos=SP_OBJECT_NEXT(layer);

            switch (verb) {
                case SP_VERB_LAYER_TO_TOP:
                    layer->raiseToTop();
                    break;
                case SP_VERB_LAYER_TO_BOTTOM:
                    layer->lowerToBottom();
                    break;
                case SP_VERB_LAYER_RAISE:
                    layer->raiseOne();
                    break;
                case SP_VERB_LAYER_LOWER:
                    layer->lowerOne();
                    break;
            }

            if ( SP_OBJECT_NEXT(layer) != old_pos ) {
                char const *message = NULL;
                switch (verb) {
                    case SP_VERB_LAYER_TO_TOP:
                    case SP_VERB_LAYER_RAISE:
                        message = g_strdup_printf (_("Raised layer <b>%s</b>."), layer->label());
                        break;
                    case SP_VERB_LAYER_TO_BOTTOM:
                    case SP_VERB_LAYER_LOWER:
                        message = g_strdup_printf (_("Lowered layer <b>%s</b>."), layer->label());
                        break;
                };
                sp_document_done(SP_DT_DOCUMENT(dt));
                if (message) {
                    dt->messageStack()->flash(Inkscape::NORMAL_MESSAGE, message);
                    g_free ((void *) message);
                }
            } else {
                dt->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Can't move layer any further."));
            }

            break;
        }
        case SP_VERB_LAYER_DELETE: {
            if ( dt->currentLayer() != dt->currentRoot() ) {
                SP_DT_SELECTION(dt)->clear();
                SPObject *old_layer=dt->currentLayer();

                sp_object_ref(old_layer, NULL);
                SPObject *survivor=Inkscape::next_layer(dt->currentRoot(), old_layer);
                if (!survivor) {
                    survivor = Inkscape::previous_layer(dt->currentRoot(), old_layer);
                }
                if (survivor) {
                    dt->setCurrentLayer(survivor);
                }
                old_layer->deleteObject();
                sp_object_unref(old_layer, NULL);

                sp_document_done(SP_DT_DOCUMENT(dt));

                // TRANSLATORS: this means "The layer has been deleted."
                dt->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Deleted layer."));
            } else {
                dt->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("No current layer."));
            }
            break;
        }
    }

    return;
} // end of sp_verb_action_layer_perform()

/** \brief  Decode the verb code and take appropriate action */
void
ObjectVerb::perform ( SPAction *action, void *data, void *pdata )
{
    SPDesktop *dt = SP_DESKTOP(sp_action_get_view(action));

    if (!dt)
        return;

    SPSelection *sel = SP_DT_SELECTION(dt);

    if (sel->isEmpty())
        return;

    NR::Point const center(sel->bounds().midpoint());

    switch (reinterpret_cast<std::size_t>(data)) {
        case SP_VERB_OBJECT_ROTATE_90_CW:
            sp_selection_rotate_90_cw ();
            break;
        case SP_VERB_OBJECT_ROTATE_90_CCW:
            sp_selection_rotate_90_ccw ();
            break;
        case SP_VERB_OBJECT_FLATTEN:
            sp_selection_remove_transform ();
            break;
        case SP_VERB_OBJECT_TO_CURVE:
            sp_selected_path_to_curves ();
            break;
        case SP_VERB_OBJECT_FLOWTEXT_TO_TEXT:
            convert_to_text ();
            break;
        case SP_VERB_OBJECT_FLIP_HORIZONTAL:
            // TODO: make tool-sensitive, in node edit flip selected node(s)
            sp_selection_scale_relative(sel, center, NR::scale(-1.0, 1.0));
            sp_document_done (SP_DT_DOCUMENT (dt));
            break;
        case SP_VERB_OBJECT_FLIP_VERTICAL:
            // TODO: make tool-sensitive, in node edit flip selected node(s)
            sp_selection_scale_relative(sel, center, NR::scale(1.0, -1.0));
            sp_document_done (SP_DT_DOCUMENT (dt));
            break;
        default:
            break;
    }

} // end of sp_verb_action_object_perform()

/** \brief  Decode the verb code and take appropriate action */
void
ContextVerb::perform (SPAction *action, void * data, void * pdata)
{
    SPDesktop *dt;
    sp_verb_t verb;
    int vidx;

    dt = SP_DESKTOP (sp_action_get_view (action));

    if (!dt)
        return;

    verb = (sp_verb_t)GPOINTER_TO_INT((gpointer)data);

    /* TODO !!! hopefully this can go away soon and actions can look after
     * themselves
     */
    for (vidx = SP_VERB_CONTEXT_SELECT; vidx <= SP_VERB_CONTEXT_DROPPER_PREFS; vidx++)
    {
        SPAction *tool_action= get((sp_verb_t)vidx)->get_action(SP_VIEW (dt));
        if (tool_action) {
            sp_action_set_active (tool_action, vidx == (int)verb);
        }
    }

    switch (verb) {
        case SP_VERB_CONTEXT_SELECT:
            tools_switch_current (TOOLS_SELECT);
            break;
        case SP_VERB_CONTEXT_NODE:
            tools_switch_current (TOOLS_NODES);
            break;
        case SP_VERB_CONTEXT_RECT:
            tools_switch_current (TOOLS_SHAPES_RECT);
            break;
        case SP_VERB_CONTEXT_ARC:
            tools_switch_current (TOOLS_SHAPES_ARC);
            break;
        case SP_VERB_CONTEXT_STAR:
            tools_switch_current (TOOLS_SHAPES_STAR);
            break;
        case SP_VERB_CONTEXT_SPIRAL:
            tools_switch_current (TOOLS_SHAPES_SPIRAL);
            break;
        case SP_VERB_CONTEXT_PENCIL:
            tools_switch_current (TOOLS_FREEHAND_PENCIL);
            break;
        case SP_VERB_CONTEXT_PEN:
            tools_switch_current (TOOLS_FREEHAND_PEN);
            break;
        case SP_VERB_CONTEXT_CALLIGRAPHIC:
            tools_switch_current (TOOLS_CALLIGRAPHIC);
            break;
        case SP_VERB_CONTEXT_TEXT:
            tools_switch_current (TOOLS_TEXT);
            break;
        case SP_VERB_CONTEXT_ZOOM:
            tools_switch_current (TOOLS_ZOOM);
            break;
        case SP_VERB_CONTEXT_DROPPER:
            tools_switch_current (TOOLS_DROPPER);
            break;

        case SP_VERB_CONTEXT_SELECT_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_SELECTOR);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_NODE_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_NODE);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_RECT_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_SHAPES);
            prefs_set_int_attribute ("dialogs.preferences", "page_shapes", PREFS_PAGE_TOOLS_SHAPES_RECT);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_ARC_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_SHAPES);
            prefs_set_int_attribute ("dialogs.preferences", "page_shapes", PREFS_PAGE_TOOLS_SHAPES_ELLIPSE);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_STAR_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_SHAPES);
            prefs_set_int_attribute ("dialogs.preferences", "page_shapes", PREFS_PAGE_TOOLS_SHAPES_STAR);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_SPIRAL_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_SHAPES);
            prefs_set_int_attribute ("dialogs.preferences", "page_shapes", PREFS_PAGE_TOOLS_SHAPES_SPIRAL);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_PENCIL_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_PENCIL);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_PEN_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_PEN);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_CALLIGRAPHIC_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_CALLIGRAPHY);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_TEXT_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_TEXT);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_ZOOM_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_ZOOM);
            sp_display_dialog ();
            break;
        case SP_VERB_CONTEXT_DROPPER_PREFS:
            prefs_set_int_attribute ("dialogs.preferences", "page_top", PREFS_PAGE_TOOLS);
            prefs_set_int_attribute ("dialogs.preferences", "page_tools", PREFS_PAGE_TOOLS_DROPPER);
            sp_display_dialog ();
            break;

        default:
            break;
    }

} // end of sp_verb_action_ctx_perform()

/** \brief  Decode the verb code and take appropriate action */
void
ZoomVerb::perform (SPAction *action, void * data, void * pdata)
{
    NRRect d;

    SPDesktop *dt = SP_DESKTOP (sp_action_get_view (action));
    if (!dt)
        return;

    SPDocument *doc = SP_DT_DOCUMENT (dt);

    SPRepr *repr = SP_OBJECT_REPR (dt->namedview);

    gdouble zoom_inc =
        prefs_get_double_attribute_limited ( "options.zoomincrement",
                                             "value", 1.414213562, 1.01, 10 );

    switch (reinterpret_cast<std::size_t>(data)) {
        case SP_VERB_ZOOM_IN:
            sp_desktop_get_display_area (dt, &d);
            sp_desktop_zoom_relative ( dt, (d.x0 + d.x1) / 2,
                                       (d.y0 + d.y1) / 2, zoom_inc );
            break;
        case SP_VERB_ZOOM_OUT:
            sp_desktop_get_display_area (dt, &d);
            sp_desktop_zoom_relative ( dt, (d.x0 + d.x1) / 2,
                                       (d.y0 + d.y1) / 2, 1 / zoom_inc );
            break;
        case SP_VERB_ZOOM_1_1:
            sp_desktop_get_display_area (dt, &d);
            sp_desktop_zoom_absolute ( dt, (d.x0 + d.x1) / 2,
                                       (d.y0 + d.y1) / 2, 1.0 );
            break;
        case SP_VERB_ZOOM_1_2:
            sp_desktop_get_display_area (dt, &d);
            sp_desktop_zoom_absolute ( dt, (d.x0 + d.x1) / 2,
                                       (d.y0 + d.y1) / 2, 0.5);
            break;
        case SP_VERB_ZOOM_2_1:
            sp_desktop_get_display_area (dt, &d);
            sp_desktop_zoom_absolute ( dt, (d.x0 + d.x1) / 2,
                                       (d.y0 + d.y1) / 2, 2.0 );
            break;
        case SP_VERB_ZOOM_PAGE:
            sp_desktop_zoom_page (dt);
            break;
        case SP_VERB_ZOOM_PAGE_WIDTH:
            sp_desktop_zoom_page_width (dt);
            break;
        case SP_VERB_ZOOM_DRAWING:
            sp_desktop_zoom_drawing (dt);
            break;
        case SP_VERB_ZOOM_SELECTION:
            sp_desktop_zoom_selection (dt);
            break;
        case SP_VERB_ZOOM_NEXT:
            sp_desktop_next_zoom (dt);
            break;
        case SP_VERB_ZOOM_PREV:
            sp_desktop_prev_zoom (dt);
            break;
        case SP_VERB_TOGGLE_RULERS:
            sp_desktop_toggle_rulers (dt);
            break;
        case SP_VERB_TOGGLE_SCROLLBARS:
            sp_desktop_toggle_scrollbars (dt);
            break;
        case SP_VERB_TOGGLE_GUIDES:
            sp_namedview_toggle_guides (doc, repr);
            break;
        case SP_VERB_TOGGLE_GRID:
            sp_namedview_toggle_grid (doc, repr);
            break;
#ifdef HAVE_GTK_WINDOW_FULLSCREEN
        case SP_VERB_FULLSCREEN:
            fullscreen (dt);
            break;
#endif /* HAVE_GTK_WINDOW_FULLSCREEN */
        case SP_VERB_VIEW_NEW:
            sp_ui_new_view ();
            break;
        case SP_VERB_VIEW_NEW_PREVIEW:
            sp_ui_new_view_preview ();
            break;
        default:
            break;
    }

} // end of sp_verb_action_zoom_perform()

/** \brief  Decode the verb code and take appropriate action */
void
DialogVerb::perform (SPAction *action, void * data, void * pdata)
{
    if ((int) data != SP_VERB_DIALOG_TOGGLE) {
        // unhide all when opening a new dialog
        inkscape_dialogs_unhide ();
    }

    switch (reinterpret_cast<std::size_t>(data)) {
        case SP_VERB_DIALOG_DISPLAY:
            sp_display_dialog ();
            break;
        case SP_VERB_DIALOG_NAMEDVIEW:
            sp_desktop_dialog ();
            break;
        case SP_VERB_DIALOG_FILL_STROKE:
            sp_object_properties_dialog ();
            break;
        case SP_VERB_DIALOG_TRANSFORM:
            sp_transformation_dialog_move ();
            break;
        case SP_VERB_DIALOG_ALIGN_DISTRIBUTE:
            sp_quick_align_dialog ();
            break;
        case SP_VERB_DIALOG_TEXT:
            sp_text_edit_dialog ();
            break;
        case SP_VERB_DIALOG_XML_EDITOR:
            sp_xml_tree_dialog ();
            break;
        case SP_VERB_DIALOG_FIND:
            sp_find_dialog ();
            break;
        case SP_VERB_DIALOG_DEBUG:
            Inkscape::UI::Dialogs::DebugDialog::showInstance();
            break;
        case SP_VERB_DIALOG_SCRIPT:
            Inkscape::UI::Dialogs::ScriptDialog::showInstance();
            break;
        case SP_VERB_DIALOG_TOGGLE:
            inkscape_dialogs_toggle ();
            break;
        case SP_VERB_DIALOG_CLONETILER:
            clonetiler_dialog ();
            break;
        case SP_VERB_DIALOG_ITEM:
            sp_item_dialog ();
            break;
        default:
            break;
    }

} // end of sp_verb_action_dialog_perform()

/** \brief  Decode the verb code and take appropriate action */
void
HelpVerb::perform (SPAction *action, void * data, void * pdata)
{
    switch (reinterpret_cast<std::size_t>(data)) {
        case SP_VERB_HELP_KEYS:
            sp_help_open_screen (_("keys.svg"));
            break;
        case SP_VERB_HELP_ABOUT:
            sp_help_about ();
            break;
    default:
        break;
    }
} // end of sp_verb_action_help_perform()

/** \brief  Decode the verb code and take appropriate action */
void
TutorialVerb::perform (SPAction *action, void * data, void * pdata)
{
    switch (reinterpret_cast<std::size_t>(data)) {
        case SP_VERB_TUTORIAL_BASIC:
            sp_help_open_tutorial (NULL, (gpointer)_("tutorial-basic.svg"));
            break;
        case SP_VERB_TUTORIAL_SHAPES:
            sp_help_open_tutorial (NULL, (gpointer)_("tutorial-shapes.svg"));
            break;
        case SP_VERB_TUTORIAL_ADVANCED:
            sp_help_open_tutorial (NULL, (gpointer)_("tutorial-advanced.svg"));
            break;
        case SP_VERB_TUTORIAL_TRACING:
            sp_help_open_tutorial (NULL, (gpointer)_("tutorial-tracing.svg"));
            break;
        case SP_VERB_TUTORIAL_CALLIGRAPHY:
            sp_help_open_tutorial (NULL, (gpointer)_("tutorial-calligraphy.svg"));
            break;
        case SP_VERB_TUTORIAL_DESIGN:
            sp_help_open_tutorial (NULL, (gpointer)_("elementsofdesign.svg"));
            break;
        case SP_VERB_TUTORIAL_TIPS:
            sp_help_open_tutorial (NULL, (gpointer)_("tipsandtricks.svg"));
            break;
    default:
        break;
    }
} // end of sp_verb_action_tutorial_perform()


/**
 * Action vector to define functions called if a staticly defined file verb
 * is called.
 */
SPActionEventVector FileVerb::vector =
            {{NULL},FileVerb::perform, NULL, NULL, NULL};
/**
 * Action vector to define functions called if a staticly defined edit verb is
 * called.
 */
SPActionEventVector EditVerb::vector =
            {{NULL},EditVerb::perform, NULL, NULL, NULL};

/**
 * Action vector to define functions called if a staticly defined selection
 * verb is called
 */
SPActionEventVector SelectionVerb::vector =
            {{NULL},SelectionVerb::perform, NULL, NULL, NULL};

/**
 * Action vector to define functions called if a staticly defined layer
 * verb is called
 */
SPActionEventVector LayerVerb::vector =
            {{NULL}, LayerVerb::perform, NULL, NULL, NULL};

/**
 * Action vector to define functions called if a staticly defined object
 * editing verb is called
 */
SPActionEventVector ObjectVerb::vector =
            {{NULL},ObjectVerb::perform, NULL, NULL, NULL};

/**
 * Action vector to define functions called if a staticly defined context
 * verb is called
 */
SPActionEventVector ContextVerb::vector =
            {{NULL},ContextVerb::perform, NULL, NULL, NULL};

/**
 * Action vector to define functions called if a staticly defined zoom verb
 * is called
 */
SPActionEventVector ZoomVerb::vector =
            {{NULL},ZoomVerb::perform, NULL, NULL, NULL};

/**
 * Action vector to define functions called if a staticly defined dialog verb
 * is called
 */
SPActionEventVector DialogVerb::vector =
            {{NULL},DialogVerb::perform, NULL, NULL, NULL};

/**
 * Action vector to define functions called if a staticly defined help verb
 * is called
 */
SPActionEventVector HelpVerb::vector =
            {{NULL},HelpVerb::perform, NULL, NULL, NULL};


/* *********** Effect Last ********** */
/**
 * Action vector to define functions called if a staticly defined tutorial verb
 * is called
 */
SPActionEventVector TutorialVerb::vector =
            {{NULL},TutorialVerb::perform, NULL, NULL, NULL};

/** \brief A class to represent the last effect issued */
class EffectLastVerb : public Verb {
private:
    static void perform (SPAction * action, void * mydata, void * otherdata);
    static SPActionEventVector vector;
protected:
    virtual SPAction * make_action (SPView * view);
public:
    /** \brief Use the Verb initializer with the same parameters. */
    EffectLastVerb(const unsigned int code,
                   gchar const * id,
                   gchar const * name,
                   gchar const * tip,
                   gchar const * image) :
            Verb(code, id, name, tip, image) {
    }
}; /* EffectLastVerb class */

/**
 * The vector to attach in the last effect verb.
 */
SPActionEventVector EffectLastVerb::vector =
            {{NULL},EffectLastVerb::perform, NULL, NULL, NULL};

/** \brief  Create an action for a \c EffectLastVerb
    \param  view  Which view the action should be created for
    \return The built action.

    Calls \c make_action_helper with the \c vector.
*/
SPAction *
EffectLastVerb::make_action (SPView * view)
{
    return make_action_helper(view, &vector);
}

/** \brief  Decode the verb code and take appropriate action */
void
EffectLastVerb::perform (SPAction *action, void * data, void *pdata)
{
    /* These aren't used, but are here to remind people not to use
       the CURRENT_DOCUMENT macros unless they really have to. */
    SPView * current_view = sp_action_get_view(action);
    // SPDocument * current_document = SP_VIEW_DOCUMENT(current_view);
    Inkscape::Extension::Effect * effect = Inkscape::Extension::Effect::get_last_effect();

    if (effect == NULL) return;
    if (current_view == NULL) return;

    switch ((long) data) {
        case SP_VERB_EFFECT_LAST_PREF:
            if (!effect->prefs(current_view))
                return;
        case SP_VERB_EFFECT_LAST:
            effect->effect(current_view);
            break;
        default:
            return;
    }

    return;
}
/* *********** End Effect Last ********** */

/* these must be in the same order as the SP_VERB_* enum in "verbs.h" */
Verb * Verb::_base_verbs[] = {
    /* Header */
    new Verb(SP_VERB_INVALID, NULL, NULL, NULL, NULL),
    new Verb(SP_VERB_NONE, "None", N_("None"), N_("Does nothing"), NULL),

    /* File */
    new FileVerb(SP_VERB_FILE_NEW, "FileNew", N_("Default"), N_("Create new document from default template"),
        GTK_STOCK_NEW ),
    new FileVerb(SP_VERB_FILE_OPEN, "FileOpen", N_("_Open..."),
    N_("Open existing document"), GTK_STOCK_OPEN ),
    new FileVerb(SP_VERB_FILE_REVERT, "FileRevert", N_("Re_vert"),
    N_("Revert to the last saved version of document (changes will be lost)"), GTK_STOCK_REVERT_TO_SAVED ),
    new FileVerb(SP_VERB_FILE_SAVE, "FileSave", N_("_Save"), N_("Save document"),
        GTK_STOCK_SAVE ),
    new FileVerb(SP_VERB_FILE_SAVE_AS, "FileSaveAs", N_("Save _As..."),
        N_("Save document under new name"), GTK_STOCK_SAVE_AS ),
    new FileVerb(SP_VERB_FILE_PRINT, "FilePrint", N_("_Print..."), N_("Print document"),
        GTK_STOCK_PRINT ),
    // TRANSLATORS: "Vacuum Defs" means "Clean up defs" (so as to remove unused definitions)
    new FileVerb(SP_VERB_FILE_VACUUM, "FileVacuum", N_("Vac_uum Defs"), N_("Remove unused stuff from the &lt;defs&gt; of the document"),
     "file_vacuum" ),
    new FileVerb(SP_VERB_FILE_PRINT_DIRECT, "FilePrintDirect", N_("Print _Direct"),
        N_("Print directly to file or pipe"), NULL ),
    new FileVerb(SP_VERB_FILE_PRINT_PREVIEW, "FilePrintPreview", N_("Print Previe_w"),
        N_("Preview document printout"), GTK_STOCK_PRINT_PREVIEW ),
    new FileVerb(SP_VERB_FILE_IMPORT, "FileImport", N_("_Import..."),
        N_("Import bitmap or SVG image into document"), "file_import"),
    new FileVerb(SP_VERB_FILE_EXPORT, "FileExport", N_("_Export Bitmap..."),
        N_("Export document or selection as PNG bitmap"), "file_export"),
    new FileVerb(SP_VERB_FILE_NEXT_DESKTOP, "FileNextDesktop", N_("N_ext Window"),
        N_("Switch to the next document window"), "window_next"),
    new FileVerb(SP_VERB_FILE_PREV_DESKTOP, "FilePrevDesktop", N_("P_revious Window"),
        N_("Switch to the previous document window"), "window_previous"),
    new FileVerb(SP_VERB_FILE_CLOSE_VIEW, "FileCloseView", N_("_Close"),
    N_("Close window"), GTK_STOCK_CLOSE),
    new FileVerb(SP_VERB_FILE_QUIT, "FileQuit", N_("_Quit"), N_("Quit Inkscape"), GTK_STOCK_QUIT),

    /* Edit */
    new EditVerb(SP_VERB_EDIT_UNDO, "EditUndo", N_("_Undo"), N_("Undo last action"),
        GTK_STOCK_UNDO),
    new EditVerb(SP_VERB_EDIT_REDO, "EditRedo", N_("_Redo"),
        N_("Do again last undone action"), GTK_STOCK_REDO),
    new EditVerb(SP_VERB_EDIT_CUT, "EditCut", N_("Cu_t"),
        N_("Cut selection to clipboard"), GTK_STOCK_CUT),
    new EditVerb(SP_VERB_EDIT_COPY, "EditCopy", N_("_Copy"),
        N_("Copy selection to clipboard"), GTK_STOCK_COPY),
    new EditVerb(SP_VERB_EDIT_PASTE, "EditPaste", N_("_Paste"),
        N_("Paste object(s) from clipboard to mouse point"), GTK_STOCK_PASTE),
    new EditVerb(SP_VERB_EDIT_PASTE_STYLE, "EditPasteStyle", N_("Paste _Style"),
        N_("Apply style of the copied object to selection"), "selection_paste_style"),
    new EditVerb(SP_VERB_EDIT_PASTE_IN_PLACE, "EditPasteInPlace", N_("Paste _In Place"),
        N_("Paste object(s) from clipboard to the original location"), "selection_paste_in_place"),
    new EditVerb(SP_VERB_EDIT_DELETE, "EditDelete", N_("_Delete"),
        N_("Delete selection"), GTK_STOCK_DELETE),
    new EditVerb(SP_VERB_EDIT_DUPLICATE, "EditDuplicate", N_("Duplic_ate"),
        N_("Duplicate selected object(s)"), "edit_duplicate"),
    new EditVerb(SP_VERB_EDIT_CLONE, "EditClone", N_("Clo_ne"),
        N_("Create a clone of selected object (a copy linked to the original)"), "edit_clone"),
    new EditVerb(SP_VERB_EDIT_UNLINK_CLONE, "EditUnlinkClone", N_("Unlin_k Clone"),
        N_("Cut the clone's link to its original"), "edit_unlink_clone"),
    new EditVerb(SP_VERB_EDIT_CLONE_ORIGINAL, "EditCloneOriginal", N_("Select _Original"),
        N_("Select the object to which the clone is linked"), NULL),
    // TRANSLATORS: Convert selection to a rectangle with tiled pattern fill
    new EditVerb(SP_VERB_EDIT_TILE, "EditTile", N_("O_bject(s) to Pattern"),
        N_("Convert selection to a rectangle with tiled pattern fill"), NULL),
    // TRANSLATORS: Extract objects from a tiled pattern fill
    new EditVerb(SP_VERB_EDIT_UNTILE, "EditUnTile", N_("Pattern to Ob_ject(s)"),
        N_("Extract objects from a tiled pattern fill"), NULL),
    new EditVerb(SP_VERB_EDIT_CLEAR_ALL, "EditClearAll", N_("Clea_r All"),
        N_("Delete all objects from document"), NULL),
    new EditVerb(SP_VERB_EDIT_SELECT_ALL, "EditSelectAll", N_("Select Al_l"),
        N_("Select all objects or all nodes"), "selection_select_all"),
    new EditVerb(SP_VERB_EDIT_SELECT_ALL_IN_ALL_LAYERS, "EditSelectAllInAllLayers", N_("Select All in All Layers"),
        N_("Select all objects in all visible and unlocked layers"), NULL),
    new EditVerb(SP_VERB_EDIT_INVERT, "EditInvert", N_("Invert Selection"),
        N_("Invert selection (unselect what is selected and select everything else)"), NULL),
    new EditVerb(SP_VERB_EDIT_INVERT_IN_ALL_LAYERS, "EditInvertInAllLayers", N_("Invert in All Layers"),
        N_("Invert selection in all visible and unlocked layers"), NULL),
    new EditVerb(SP_VERB_EDIT_DESELECT, "EditDeselect", N_("D_eselect"),
        N_("Deselect any selected objects or nodes"), NULL),

    /* Selection */
    new SelectionVerb(SP_VERB_SELECTION_TO_FRONT, "SelectionToFront", N_("Raise to _Top"),
        N_("Raise selection to top"), "selection_top"),
    new SelectionVerb(SP_VERB_SELECTION_TO_BACK, "SelectionToBack", N_("Lower to _Bottom"),
        N_("Lower selection to bottom"), "selection_bot"),
    new SelectionVerb(SP_VERB_SELECTION_RAISE, "SelectionRaise", N_("_Raise"),
        N_("Raise selection one step"), "selection_up"),
    new SelectionVerb(SP_VERB_SELECTION_LOWER, "SelectionLower", N_("_Lower"),
        N_("Lower selection one step"), "selection_down"),
    new SelectionVerb(SP_VERB_SELECTION_GROUP, "SelectionGroup", N_("_Group"),
        N_("Group selected objects"), "selection_group"),
    new SelectionVerb(SP_VERB_SELECTION_UNGROUP, "SelectionUnGroup", N_("_Ungroup"),
        N_("Ungroup selected group(s)"), "selection_ungroup"),

    new SelectionVerb(SP_VERB_SELECTION_TEXTTOPATH, "SelectionTextToPath", N_("_Put on Path"),
        N_("Put text on path"), NULL),
    new SelectionVerb(SP_VERB_SELECTION_TEXTFROMPATH, "SelectionTextFromPath", N_("_Remove from Path"),
        N_("Remove text from path"), NULL),
    new SelectionVerb(SP_VERB_SELECTION_REMOVE_KERNS, "SelectionTextRemoveKerns", N_("Remove Manual _Kerns"),
        N_("Remove all manual kerns from a text object"), NULL),

    new SelectionVerb(SP_VERB_SELECTION_UNION, "SelectionUnion", N_("_Union"),
        N_("Union of selected objects"), "union"),
    new SelectionVerb(SP_VERB_SELECTION_INTERSECT, "SelectionIntersect", N_("_Intersection"),
        N_("Intersection of selected objects"), "intersection"),
    new SelectionVerb(SP_VERB_SELECTION_DIFF, "SelectionDiff", N_("_Difference"),
        N_("Difference of selected objects (bottom minus top)"), "difference"),
    new SelectionVerb(SP_VERB_SELECTION_SYMDIFF, "SelectionSymDiff", N_("E_xclusion"),
        N_("Exclusive OR of selected objects"), "exclusion"),
    new SelectionVerb(SP_VERB_SELECTION_CUT, "SelectionDivide", N_("Di_vision"),
        N_("Cut the bottom object into pieces"), "division"),
    // TRANSLATORS: "to cut a path" is not the same as "to break a path apart" - see the
    // Advanced tutorial for more info
    new SelectionVerb(SP_VERB_SELECTION_SLICE, "SelectionCutPath", N_("Cut _Path"),
        N_("Cut the bottom object's stroke into pieces, removing fill"), "cut_path"),
    // TRANSLATORS: "outset": expand a shape by offsetting the object's path,
    // i.e. by displacing it perpendicular to the path in each point.
    // See also the Advanced Tutorial for explanation.
    new SelectionVerb(SP_VERB_SELECTION_OFFSET, "SelectionOffset", N_("Ou_tset"),
        N_("Outset selected path(s)"), "outset_path"),
    new SelectionVerb(SP_VERB_SELECTION_OFFSET_SCREEN, "SelectionOffsetScreen", 
        N_("O_utset Path by 1px"),
        N_("Outset selected path(s) by 1px"), NULL),
    new SelectionVerb(SP_VERB_SELECTION_OFFSET_SCREEN_10, "SelectionOffsetScreen10", 
        N_("O_utset Path by 10px"),
        N_("Outset selected path(s) by 10px"), NULL),
    // TRANSLATORS: "inset": contract a shape by offsetting the object's path,
    // i.e. by displacing it perpendicular to the path in each point.
    // See also the Advanced Tutorial for explanation.
    new SelectionVerb(SP_VERB_SELECTION_INSET, "SelectionInset", N_("I_nset"),
        N_("Inset selected path(s)"), "inset_path"),
    new SelectionVerb(SP_VERB_SELECTION_INSET_SCREEN, "SelectionInsetScreen", 
        N_("I_nset Path by 1px"),
        N_("Inset selected path(s) by 1px"), NULL),
    new SelectionVerb(SP_VERB_SELECTION_INSET_SCREEN_10, "SelectionInsetScreen", 
        N_("I_nset Path by 10px"),
        N_("Inset selected path(s) by 10px"), NULL),
    new SelectionVerb(SP_VERB_SELECTION_DYNAMIC_OFFSET, "SelectionDynOffset",
        N_("D_ynamic Offset"), N_("Create a dynamic offset object"), "dynamic_offset"),
    new SelectionVerb(SP_VERB_SELECTION_LINKED_OFFSET, "SelectionLinkedOffset",
        N_("_Linked Offset"),
        N_("Create a dynamic offset object linked to the original path"),
        "linked_offset"),
    new SelectionVerb(SP_VERB_SELECTION_OUTLINE, "SelectionOutline", N_("_Stroke to Path"),
        N_("Convert selected stroke(s) to path(s)"), "stroke_tocurve"),
    new SelectionVerb(SP_VERB_SELECTION_SIMPLIFY, "SelectionSimplify", N_("Si_mplify"),
        N_("Simplify selected path(s) by removing extra nodes"), "simplify"),
    new SelectionVerb(SP_VERB_SELECTION_CLEANUP, "SelectionCleanup", N_("Cl_eanup"),
        N_("Clean up selected path(s)"), "selection_cleanup"),
    new SelectionVerb(SP_VERB_SELECTION_REVERSE, "SelectionReverse", N_("_Reverse"),
        N_("Reverses the direction of selected path(s); useful for flipping markers"), "selection_reverse"),
    // TRANSLATORS: "to trace" means "to convert a bitmap to vector graphics" (to vectorize)
    new SelectionVerb(SP_VERB_SELECTION_POTRACE, "SelectionPotrace", N_("_Trace Bitmap"),
        N_("Convert bitmap object to paths"), "selection_trace"),
    new SelectionVerb(SP_VERB_SELECTION_CREATE_BITMAP, "SelectionCreateBitmap", N_("_Make a Bitmap Copy"),
        N_("Export selection to a bitmap and insert it into document"), "selection_bitmap" ),
    new SelectionVerb(SP_VERB_SELECTION_COMBINE, "SelectionCombine", N_("_Combine"),
        N_("Combine several paths into one"), "selection_combine"),
    // TRANSLATORS: "to cut a path" is not the same as "to break a path apart" - see the
    // Advanced tutorial for more info
    new SelectionVerb(SP_VERB_SELECTION_BREAK_APART, "SelectionBreakApart", N_("Break _Apart"),
        N_("Break selected path(s) into subpaths"), "selection_break"),

    /* Layer */
    new LayerVerb(SP_VERB_LAYER_NEW, "LayerNew", N_("_New Layer..."),
        N_("Create a new layer"), NULL),
    new LayerVerb(SP_VERB_LAYER_RENAME, "LayerRename", N_("Ren_ame Layer..."),
        N_("Rename the current layer"), NULL),
    new LayerVerb(SP_VERB_LAYER_NEXT, "LayerNext", N_("Switch to Next Layer"),
        N_("Switch to the next layer in the document"), NULL),
    new LayerVerb(SP_VERB_LAYER_PREV, "LayerPrev", N_("Switch to Previous Layer"),
        N_("Switch to the previous layer in the document"), NULL),
    new LayerVerb(SP_VERB_LAYER_TO_TOP, "LayerToTop", N_("Layer to _Top"),
        N_("Raise the current layer to the top"), NULL),
    new LayerVerb(SP_VERB_LAYER_TO_BOTTOM, "LayerToBottom", N_("Layer to _Bottom"),
        N_("Lower the current layer to the bottom"), NULL),
    new LayerVerb(SP_VERB_LAYER_RAISE, "LayerRaise", N_("_Raise Layer"),
        N_("Raise the current layer"), NULL),
    new LayerVerb(SP_VERB_LAYER_LOWER, "LayerLower", N_("_Lower Layer"),
        N_("Lower the current layer"), NULL),
    new LayerVerb(SP_VERB_LAYER_DELETE, "LayerDelete", N_("_Delete Current Layer"),
        N_("Delete the current layer"), NULL),

    /* Object */
    new ObjectVerb(SP_VERB_OBJECT_ROTATE_90_CW, "ObjectRotate90", N_("Rotate _90 deg CW"),
        N_("Rotate selection 90 degrees clockwise"), "object_rotate_90_CW"),
    new ObjectVerb(SP_VERB_OBJECT_ROTATE_90_CCW, "ObjectRotate90CCW", N_("Rotate 9_0 deg CCW"),
        N_("Rotate selection 90 degrees counter-clockwise"), "object_rotate_90_CCW"),
    new ObjectVerb(SP_VERB_OBJECT_FLATTEN, "ObjectFlatten", N_("Remove _Transformations"),
        N_("Remove transformations from object"), "object_reset"),
    new ObjectVerb(SP_VERB_OBJECT_TO_CURVE, "ObjectToCurve", N_("_Object to Path"),
        N_("Convert selected object(s) to path(s)"), "object_tocurve"),
    new ObjectVerb(SP_VERB_OBJECT_FLOWTEXT_TO_TEXT, "ObjectFlowtextToText", N_("_Unflow Text"),
        N_("Convert selected flowed text(s) to regular text objects"), NULL),
    new ObjectVerb(SP_VERB_OBJECT_FLIP_HORIZONTAL, "ObjectFlipHorizontally",
        N_("Flip _Horizontally"), N_("Flip selection horizontally"),
        "object_flip_hor"),
    new ObjectVerb(SP_VERB_OBJECT_FLIP_VERTICAL, "ObjectFlipVertically",
        N_("Flip _Vertically"), N_("Flip selection vertically"),
        "object_flip_ver"),

    /* Tools */
    new ContextVerb(SP_VERB_CONTEXT_SELECT, "DrawSelect", N_("Select"),
        N_("Select and transform objects"), "draw_select"),
    new ContextVerb(SP_VERB_CONTEXT_NODE, "DrawNode", N_("Node Edit"),
        N_("Edit path nodes or control handles"), "draw_node"),
    new ContextVerb(SP_VERB_CONTEXT_RECT, "DrawRect", N_("Rectangle"),
        N_("Create rectangles and squares"), "draw_rect"),
    new ContextVerb(SP_VERB_CONTEXT_ARC, "DrawArc", N_("Ellipse"),
        N_("Create circles, ellipses, and arcs"), "draw_arc"),
    new ContextVerb(SP_VERB_CONTEXT_STAR, "DrawStar", N_("Star"),
        N_("Create stars and polygons"), "draw_star"),
    new ContextVerb(SP_VERB_CONTEXT_SPIRAL, "DrawSpiral", N_("Spiral"),
        N_("Create spirals"), "draw_spiral"),
    new ContextVerb(SP_VERB_CONTEXT_PENCIL, "DrawPencil", N_("Pencil"),
        N_("Draw freehand lines"), "draw_freehand"),
    new ContextVerb(SP_VERB_CONTEXT_PEN, "DrawPen", N_("Pen"),
        N_("Draw Bezier curves and straight lines"), "draw_pen"),
    new ContextVerb(SP_VERB_CONTEXT_CALLIGRAPHIC, "DrawCalligrphic", N_("Calligraphy"),
        N_("Draw calligraphic lines"), "draw_calligraphic"),
    new ContextVerb(SP_VERB_CONTEXT_TEXT, "DrawText", N_("Text"),
        N_("Create and edit text objects"), "draw_text"),
    new ContextVerb(SP_VERB_CONTEXT_ZOOM, "DrawZoom", N_("Zoom"),
        N_("Zoom in or out"), "draw_zoom"),
    new ContextVerb(SP_VERB_CONTEXT_DROPPER, "DrawDropper", N_("Dropper"),
        N_("Pick averaged colors from image"), "draw_dropper"),

    /* Tool prefs */
    new ContextVerb(SP_VERB_CONTEXT_SELECT_PREFS, "SelectPrefs", N_("Selector Preferences"),
        N_("Open Inkscape Preferences for the Selector tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_NODE_PREFS, "NodePrefs", N_("Node Tool Preferences"),
        N_("Open Inkscape Preferences for the Node tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_RECT_PREFS, "RectPrefs", N_("Rectangle Preferences"),
        N_("Open Inkscape Preferences for the Rectangle tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_ARC_PREFS, "ArcPrefs", N_("Ellipse Preferences"),
        N_("Open Inkscape Preferences for the Ellipse tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_STAR_PREFS, "StarPrefs", N_("Star Preferences"),
        N_("Open Inkscape Preferences for the Star tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_SPIRAL_PREFS, "SpiralPrefs", N_("Spiral Preferences"),
        N_("Open Inkscape Preferences for the Spiral tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_PENCIL_PREFS, "PencilPrefs", N_("Pencil Preferences"),
        N_("Open Inkscape Preferences for the Pencil tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_PEN_PREFS, "PenPrefs", N_("Pen Preferences"),
        N_("Open Inkscape Preferences for the Pen tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_CALLIGRAPHIC_PREFS, "CalligraphicPrefs", N_("Calligraphic Preferences"),
        N_("Open Inkscape Preferences for the Calligraphy tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_TEXT_PREFS, "TextPrefs", N_("Text Preferences"),
        N_("Open Inkscape Preferences for the Text tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_ZOOM_PREFS, "ZoomPrefs", N_("Zoom Preferences"),
        N_("Open Inkscape Preferences for the Zoom tool"), NULL),
    new ContextVerb(SP_VERB_CONTEXT_DROPPER_PREFS, "DropperPrefs", N_("Dropper Preferences"),
        N_("Open Inkscape Preferences for the Dropper tool"), NULL),

    /* Zooming */
    new ZoomVerb(SP_VERB_ZOOM_IN, "ZoomIn", N_("Zoom In"), N_("Zoom in"), "zoom_in"),
    new ZoomVerb(SP_VERB_ZOOM_OUT, "ZoomOut", N_("Zoom Out"), N_("Zoom out"), "zoom_out"),
    new ZoomVerb(SP_VERB_TOGGLE_RULERS, "ToggleRulers", N_("_Rulers"), N_("Show or hide the canvas rulers"), "rulers"),
    new ZoomVerb(SP_VERB_TOGGLE_SCROLLBARS, "ToggleScrollbars", N_("Scroll_bars"), N_("Show or hide the canvas scrollbars"), "scrollbars"),
    new ZoomVerb(SP_VERB_TOGGLE_GRID, "ToggleGrid", N_("_Grid"), N_("Show or hide grid"), "grid"),
    new ZoomVerb(SP_VERB_TOGGLE_GUIDES, "ToggleGuides", N_("G_uides"), N_("Show or hide guides"), "guides"),
    new ZoomVerb(SP_VERB_ZOOM_NEXT, "ZoomNext", N_("Nex_t Zoom"), N_("Next zoom (from the history of zooms)"),
        "zoom_next"),
    new ZoomVerb(SP_VERB_ZOOM_PREV, "ZoomPrev", N_("Pre_vious Zoom"), N_("Previous zoom (from the history of zooms)"),
        "zoom_previous"),
    new ZoomVerb(SP_VERB_ZOOM_1_1, "Zoom1:0", N_("Zoom 1:_1"), N_("Zoom to 1:1"),
        "zoom_1_to_1"),
    new ZoomVerb(SP_VERB_ZOOM_1_2, "Zoom1:2", N_("Zoom 1:_2"), N_("Zoom to 1:2"),
        "zoom_1_to_2"),
    new ZoomVerb(SP_VERB_ZOOM_2_1, "Zoom2:1", N_("_Zoom 2:1"), N_("Zoom to 2:1"),
        "zoom_2_to_1"),
#ifdef HAVE_GTK_WINDOW_FULLSCREEN
    new ZoomVerb(SP_VERB_FULLSCREEN, "FullScreen", N_("_Fullscreen"), N_("Stretch this document window to full screen"),
        "fullscreen"),
#endif /* HAVE_GTK_WINDOW_FULLSCREEN */
    new ZoomVerb(SP_VERB_VIEW_NEW, "ViewNew", N_("D_uplicate Window"), N_("Open a new window with the same document"),
        "view_new"),
    new ZoomVerb(SP_VERB_VIEW_NEW_PREVIEW, "ViewNewPreview", N_("_New View Preview"),
         N_("New View Preview"), NULL/*"view_new_preview"*/),
    new ZoomVerb(SP_VERB_ZOOM_PAGE, "ZoomPage", N_("_Page"), 
       N_("Zoom to fit page in window"), "zoom_page"),
    new ZoomVerb(SP_VERB_ZOOM_PAGE_WIDTH, "ZoomPageWidth", N_("Page _Width"),
        N_("Zoom to fit page width in window"), "zoom_pagewidth"),
    new ZoomVerb(SP_VERB_ZOOM_DRAWING, "ZoomDrawing", N_("_Drawing"),
        N_("Zoom to fit drawing in window"), "zoom_draw"),
    new ZoomVerb(SP_VERB_ZOOM_SELECTION, "ZoomSelection", N_("_Selection"),
        N_("Zoom to fit selection in window"), "zoom_select"),

    /* Dialogs */
    new DialogVerb(SP_VERB_DIALOG_DISPLAY, "DialogDisplay", N_("In_kscape Preferences..."),
        N_("Global Inkscape preferences"), GTK_STOCK_PREFERENCES ),
    new DialogVerb(SP_VERB_DIALOG_NAMEDVIEW, "DialogNamedview", N_("_Document Preferences..."),
        N_("Preferences saved with the document"), GTK_STOCK_PROPERTIES ),
    new DialogVerb(SP_VERB_DIALOG_FILL_STROKE, "DialogFillStroke", N_("_Fill and Stroke..."),
        N_("Fill and Stroke dialog"), "fill_and_stroke"),
    new DialogVerb(SP_VERB_DIALOG_TRANSFORM, "DialogTransform", N_("Transfor_m..."),
        N_("Transform dialog"), "object_trans"),
    new DialogVerb(SP_VERB_DIALOG_ALIGN_DISTRIBUTE, "DialogAlignDistribute", N_("_Align and Distribute..."), 
        N_("Align and Distribute dialog"), "object_align"),
    new DialogVerb(SP_VERB_DIALOG_TEXT, "Dialogtext", N_("_Text and Font..."),
        N_("Text and Font dialog"), "object_font"),
    new DialogVerb(SP_VERB_DIALOG_XML_EDITOR, "DialogXMLEditor", N_("_XML Editor..."),
        N_("XML Editor"), "xml_editor"),
    new DialogVerb(SP_VERB_DIALOG_FIND, "DialogFind", N_("_Find..."),
        N_("Find objects in document"), GTK_STOCK_FIND ),
    new DialogVerb(SP_VERB_DIALOG_DEBUG, "DialogDebug", N_("_Messages..."),
        N_("View debug messages"), NULL),
    new DialogVerb(SP_VERB_DIALOG_SCRIPT, "DialogScript", N_("_Scripts..."),
        N_("Run scripts"), NULL),
    new DialogVerb(SP_VERB_DIALOG_TOGGLE, "DialogsToggle", N_("Show/_Hide Dialogs"),
        N_("Show or hide all active dialogs"), "dialog_toggle"),
    // TRANSLATORS: "Tile clones" means: "Create tiled clones"
    new DialogVerb(SP_VERB_DIALOG_CLONETILER, "DialogsClonetiler", N_("Tile clones..."),
        N_("Create and arrange multiple clones of selection"), NULL),
    new DialogVerb(SP_VERB_DIALOG_ITEM, "DialogItem", N_("_Object Properties..."),
        N_("Object Properties dialog"), "dialog_item_properties"),

    /* Help */
    new HelpVerb(SP_VERB_HELP_KEYS, "HelpKeys", N_("_Keys and Mouse"),
        N_("Key and mouse shortcuts reference"), "help_keys"),
    new HelpVerb(SP_VERB_HELP_ABOUT, "HelpAbout", N_("_About Inkscape"),
        N_("About Inkscape"), /*"help_about"*/"inkscape_options"),

    /* Tutorials */
    new TutorialVerb(SP_VERB_TUTORIAL_BASIC, "TutorialsBasic", N_("Inkscape: _Basic"),
        N_("Getting started with Inkscape"), NULL/*"tutorial_basic"*/),
    new TutorialVerb(SP_VERB_TUTORIAL_SHAPES, "TutorialsShapes", N_("Inkscape: _Shapes"),
        N_("Using shape tools to create and edit shapes"), NULL),
    new TutorialVerb(SP_VERB_TUTORIAL_ADVANCED, "TutorialsAdvanced", N_("Inkscape: _Advanced"),
        N_("Advanced Inkscape topics"), NULL/*"tutorial_advanced"*/),
    // TRANSLATORS: "to trace" means "to convert a bitmap to vector graphics" (to vectorize)
    new TutorialVerb(SP_VERB_TUTORIAL_TRACING, "TutorialsTracing", N_("Inkscape: T_racing"),
        N_("Using bitmap tracing"), NULL/*"tutorial_tracing"*/),
    new TutorialVerb(SP_VERB_TUTORIAL_CALLIGRAPHY, "TutorialsCalligraphy", N_("Inkscape: _Calligraphy"),
        N_("Using the Calligraphy pen tool"), NULL),
    new TutorialVerb(SP_VERB_TUTORIAL_DESIGN, "TutorialsDesign", N_("_Elements of Design"),
        N_("Principles of design in the tutorial form"), NULL/*"tutorial_design"*/),
    new TutorialVerb(SP_VERB_TUTORIAL_TIPS, "TutorialsTips", N_("_Tips and Tricks"),
        N_("Miscellaneous tips and tricks"), NULL/*"tutorial_tips"*/),

    /* Effect */
    new EffectLastVerb(SP_VERB_EFFECT_LAST, "EffectLast", N_("Previous Effect"),
        N_("Repeat the last effect with the same settings"), NULL/*"tutorial_tips"*/),
    new EffectLastVerb(SP_VERB_EFFECT_LAST_PREF, "EffectLastPref", N_("Previous Effect..."),
        N_("Repeat the last effect with new settings"), NULL/*"tutorial_tips"*/),

    /* Footer */
    new Verb(SP_VERB_LAST, NULL, NULL, NULL, NULL)
};


}  /* namespace Inkscape */

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
