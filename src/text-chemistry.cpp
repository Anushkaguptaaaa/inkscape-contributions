#define __SP_TEXT_CHEMISTRY_C__

/*
 * Text commands
 *
 * Authors:
 *   bulia byak
 *
 * Copyright (C) 2004 authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>
#include "libnr/nr-macros.h"
#include "xml/repr.h"
#include "svg/svg.h"
#include "display/curve.h"
#include <glibmm/i18n.h>
#include "sp-object.h"
#include "sp-path.h"
#include "sp-rect.h"
#include "sp-text.h"
#include "sp-tspan.h"
#include "style.h"
#include "inkscape.h"
#include "document.h"
#include "selection.h"
#include "desktop.h"
#include "desktop-handles.h"
#include "text-editing.h"


SPItem *
text_in_selection(Inkscape::Selection *selection)
{
    for (GSList *items = (GSList *) selection->itemList();
         items != NULL;
         items = items->next) {
        if (SP_IS_TEXT(items->data))
            return ((SPItem *) items->data);
    }
    return NULL;
}

SPItem *
shape_in_selection(Inkscape::Selection *selection)
{
    for (GSList *items = (GSList *) selection->itemList();
         items != NULL;
         items = items->next) {
        if (SP_IS_SHAPE(items->data))
            return ((SPItem *) items->data);
    }
    return NULL;
}

void
scale_text_recursive(SPItem *item, gdouble scale)
{
    SPStyle *style = SP_OBJECT_STYLE(item);
    if (style) {
        style->font_size.computed *= scale;
    }
    SP_OBJECT(item)->updateRepr();

    for (SPObject *child = sp_object_first_child(SP_OBJECT(item));
         child != NULL; child = SP_OBJECT_NEXT(child))
    {
        if (SP_IS_ITEM(child))
            scale_text_recursive((SPItem *) child, scale);
    }
}


void
text_put_on_path()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;
    if (!desktop)
        return;

    Inkscape::Selection *selection = SP_DT_SELECTION(desktop);

    SPItem *text = text_in_selection(selection);
    SPItem *shape = shape_in_selection(selection);

    if (!text || !shape || g_slist_length((GSList *) selection->itemList()) != 2) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>a text and a path</b> to put text on path."));
        return;
    }

    if (SP_IS_TEXT_TEXTPATH(text)) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("This text object is <b>already put to a path</b>. Remove it from the path first. Use <b>Shift+D</b> to look up its path."));
        return;
    }

    if (SP_IS_RECT(shape)) {
        // rect is the only SPShape which is not <path> yet, and thus SVG forbids us from putting text on it
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("You cannot put text on a rectangle in this version. Convert rectangle to path first."));
        return;
    }

    Inkscape::Text::Layout const *layout = te_get_layout(text);
    Inkscape::Text::Layout::Alignment text_alignment = layout->paragraphAlignment(layout->begin());

    // remove transform from text, but recursively scale text's fontsize by the expansion
    scale_text_recursive(text, NR::expansion(SP_ITEM(text)->transform));
    sp_repr_set_attr(SP_OBJECT_REPR(text), "transform", NULL);

    // make a list of text children
    GSList *text_reprs = NULL;
    for (SPObject *o = SP_OBJECT(text)->children; o != NULL; o = o->next) {
        text_reprs = g_slist_prepend(text_reprs, SP_OBJECT_REPR(o));
    }

    // create textPath and put it into the text
    Inkscape::XML::Node *textpath = sp_repr_new("svg:textPath");
    // reference the shape
    sp_repr_set_attr(textpath, "xlink:href", g_strdup_printf("#%s", SP_OBJECT_REPR(shape)->attribute("id")));
    if (text_alignment == Inkscape::Text::Layout::RIGHT)
        sp_repr_set_attr(textpath, "startOffset", "100%");
    else if (text_alignment == Inkscape::Text::Layout::CENTER)
        sp_repr_set_attr(textpath, "startOffset", "50%");
    sp_repr_add_child(SP_OBJECT_REPR(text), textpath, NULL);

    for ( GSList *i = text_reprs ; i ; i = i->next ) {
        // make a copy of each text child
        Inkscape::XML::Node *copy = ((Inkscape::XML::Node *) i->data)->duplicate();
        // We cannot have multiline in textpath, so remove line attrs from tspans
        if (!strcmp(copy->name(), "svg:tspan")) {
            sp_repr_set_attr(copy, "sodipodi:role", NULL);
            sp_repr_set_attr(copy, "x", NULL);
            sp_repr_set_attr(copy, "y", NULL);
        }
        // remove the old repr from under text
        sp_repr_remove_child(SP_OBJECT_REPR(text), (Inkscape::XML::Node *) i->data);
        // put its copy into under textPath
        sp_repr_add_child(textpath, copy, NULL); // fixme: copy id
    }

    // x/y are useless with textpath, and confuse Batik 1.5
    sp_repr_set_attr(SP_OBJECT_REPR(text), "x", NULL);
    sp_repr_set_attr(SP_OBJECT_REPR(text), "y", NULL);

    sp_document_done(SP_DT_DOCUMENT(desktop));
    g_slist_free(text_reprs);
}

void
text_remove_from_path()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    Inkscape::Selection *selection = SP_DT_SELECTION(desktop);

    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>a text on path</b> to remove it from path."));
        return;
    }

    bool did = false;

    for (GSList *items = g_slist_copy((GSList *) selection->itemList());
         items != NULL;
         items = items->next) {

        if (!SP_IS_TEXT_TEXTPATH(SP_OBJECT(items->data))) {
            continue;
        }

        SPObject *tp = sp_object_first_child(SP_OBJECT(items->data));

        did = true;

        sp_textpath_to_text(tp);
    }

    if (!did) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("<b>No texts-on-paths</b> in the selection."));
    } else {
        selection->setList(g_slist_copy((GSList *) selection->itemList())); // reselect to update statusbar description
        sp_document_done(SP_DT_DOCUMENT(desktop));
    }
}

void
text_remove_all_kerns_recursively(SPObject *o)
{
    sp_repr_set_attr(SP_OBJECT_REPR(o), "dx", NULL);
    sp_repr_set_attr(SP_OBJECT_REPR(o), "dy", NULL);
    sp_repr_set_attr(SP_OBJECT_REPR(o), "rotate", NULL);

    for (SPObject *i = sp_object_first_child(o); i != NULL; i = SP_OBJECT_NEXT(i)) {
        text_remove_all_kerns_recursively(i);
    }
}

//FIXME: must work with text selection
void
text_remove_all_kerns()
{
    SPDesktop *desktop = SP_ACTIVE_DESKTOP;

    Inkscape::Selection *selection = SP_DT_SELECTION(desktop);

    if (selection->isEmpty()) {
        desktop->messageStack()->flash(Inkscape::WARNING_MESSAGE, _("Select <b>text(s)</b> to remove kerns from."));
        return;
    }

    bool did = false;

    for (GSList *items = g_slist_copy((GSList *) selection->itemList());
         items != NULL;
         items = items->next) {

        if (!SP_IS_TEXT(SP_OBJECT(items->data))) {
            continue;
        }

        text_remove_all_kerns_recursively(SP_OBJECT(items->data));
        did = true;
    }

    if (!did) {
        desktop->messageStack()->flash(Inkscape::ERROR_MESSAGE, _("Select <b>text(s)</b> to remove kerns from."));
    } else {
        sp_document_done(SP_DT_DOCUMENT(desktop));
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
