/*
 * SVG <text> and <tspan> implementation
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * fixme:
 *
 * These subcomponents should not be items, or alternately
 * we have to invent set of flags to mark, whether standard
 * attributes are applicable to given item (I even like this
 * idea somewhat - Lauris)
 *
 */

#include "config.h"

#include <string.h>

#include <libnr/nr-rect.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-matrix-ops.h>
#include <libnr/nr-matrix-fns.h>
#include <libnr/nr-rotate.h>
//#include <libnrtype/nr-typeface.h>
#include <libnrtype/FontFactory.h>
#include <libnrtype/font-instance.h>
#include <libnrtype/font-style-to-pos.h>
#include <libnrtype/FlowDefs.h>

#include <libnrtype/FlowRes.h>
#include <libnrtype/FlowSrc.h>
#include <libnrtype/FlowEater.h>
#include <libnrtype/FlowStyle.h>
#include <libnrtype/FlowBoxes.h>

#include <libnrtype/TextWrapper.h>

#include <livarot/LivarotDefs.h>
#include <livarot/Shape.h>
#include <livarot/Path.h>

#include <glib.h>
//#include <gtk/gtk.h>

#include <glibmm/i18n.h>
#include "xml/repr-private.h"
#include "svg/svg.h"
#include "svg/stringstream.h"
#include "display/curve.h"
#include "display/nr-arena-group.h"
#include "display/nr-arena-glyphs.h"
#include "attributes.h"
#include "document.h"
#include "desktop.h"
#include "fontsize-expansion.h"
#include "style.h"
#include "version.h"
#include "inkscape.h"
#include "view.h"
#include "print.h"

#include "sp-shape.h"
#include "sp-text.h"
#include "sp-tspan.h"
#include "sp-string.h"

#include "text-editing.h"

//#include "sp-use-reference.h"
//#include "prefs-utils.h"


/*#####################################################
#  SPTEXT
#####################################################*/

static void sp_text_class_init (SPTextClass *classname);
static void sp_text_init (SPText *text);
static void sp_text_release (SPObject *object);

static void sp_text_build (SPObject *object, SPDocument *document, SPRepr *repr);
static void sp_text_set (SPObject *object, unsigned key, gchar const *value);
static void sp_text_child_added (SPObject *object, SPRepr *rch, SPRepr *ref);
static void sp_text_remove_child (SPObject *object, SPRepr *rch);
static void sp_text_update (SPObject *object, SPCtx *ctx, guint flags);
static void sp_text_modified (SPObject *object, guint flags);
static SPRepr *sp_text_write (SPObject *object, SPRepr *repr, guint flags);

static void sp_text_bbox(SPItem const *item, NRRect *bbox, NR::Matrix const &transform, unsigned const flags);
static NRArenaItem *sp_text_show (SPItem *item, NRArena *arena, unsigned key, unsigned flags);
static void sp_text_hide (SPItem *item, unsigned key);
static char *sp_text_description (SPItem *item);
static void sp_text_snappoints(SPItem const *item, SnapPointsIter p);
static NR::Matrix sp_text_set_transform(SPItem *item, NR::Matrix const &xform);
static void sp_text_print (SPItem *item, SPPrintContext *gpc);

static void sp_text_request_relayout (SPText *text, guint flags);
static void sp_text_update_immediate_state (SPText *text);
static void sp_text_set_shape (SPText *text);

static SPObject *sp_text_get_child_by_position (SPText *text, gint pos);

static SPItemClass *text_parent_class;

GType
sp_text_get_type ()
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof (SPTextClass),
            NULL,    /* base_init */
            NULL,    /* base_finalize */
            (GClassInitFunc) sp_text_class_init,
            NULL,    /* class_finalize */
            NULL,    /* class_data */
            sizeof (SPText),
            16,    /* n_preallocs */
            (GInstanceInitFunc) sp_text_init,
            NULL,    /* value_table */
        };
        type = g_type_register_static (SP_TYPE_ITEM, "SPText", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_text_class_init (SPTextClass *classname)
{
    SPObjectClass *sp_object_class = (SPObjectClass *) classname;
    SPItemClass *item_class = (SPItemClass *) classname;

    text_parent_class = (SPItemClass*)g_type_class_ref (SP_TYPE_ITEM);

    sp_object_class->release = sp_text_release;
    sp_object_class->build = sp_text_build;
    sp_object_class->set = sp_text_set;
    sp_object_class->child_added = sp_text_child_added;
    sp_object_class->remove_child = sp_text_remove_child;
    sp_object_class->update = sp_text_update;
    sp_object_class->modified = sp_text_modified;
    sp_object_class->write = sp_text_write;

    item_class->bbox = sp_text_bbox;
    item_class->show = sp_text_show;
    item_class->hide = sp_text_hide;
    item_class->description = sp_text_description;
    item_class->snappoints = sp_text_snappoints;
    item_class->set_transform = sp_text_set_transform;
    item_class->print = sp_text_print;
}

static void
sp_text_init (SPText *text)
{
    new (&text->contents) div_flow_src(SP_OBJECT(text),txt_text);
    text->f_res = NULL;
    text->f_src = NULL;
    text->linespacing.set = 0;
    text->linespacing.value = text->linespacing.computed = 1.0;
}

static void
sp_text_release (SPObject *object)
{
    SPText *text = SP_TEXT(object);
    if ( text->f_src ) delete text->f_src;
    if ( text->f_res ) delete text->f_res;
    text->contents.~div_flow_src();

    if (((SPObjectClass *) text_parent_class)->release)
        ((SPObjectClass *) text_parent_class)->release(object);
}

static void
sp_text_build (SPObject *object, SPDocument *doc, SPRepr *repr)
{
    SPText *text = SP_TEXT (object);

    sp_object_read_attr(object, "x");
    sp_object_read_attr(object, "y");
    sp_object_read_attr(object, "dx");
    sp_object_read_attr(object, "dy");
    sp_object_read_attr(object, "rotate");
    sp_object_read_attr(object, "sodipodi:linespacing");

    if (((SPObjectClass *) text_parent_class)->build)
        ((SPObjectClass *) text_parent_class)->build(object, doc, repr);

    sp_text_update_immediate_state(text);
}

static void
sp_text_set(SPObject *object, unsigned key, gchar const *value)
{
    SPText *text = SP_TEXT (object);

    /* fixme: Vectors (Lauris) */
    switch (key) {
        case SP_ATTR_X:
            text->contents.SetX(value);
            if ( text->contents.nb_x > 0 ) {
                text->x=text->contents.x_s[0];
            } else {
                text->x.set=0;
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_Y:
            text->contents.SetY(value);
            if ( text->contents.nb_y > 0 ) {
                text->y=text->contents.y_s[0];
            } else {
                text->y.set=0;
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_DX:
            text->contents.SetDX(value);
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_DY:
            text->contents.SetDY(value);
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_ROTATE:
            text->contents.SetRot(value);
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
            break;
        case SP_ATTR_SODIPODI_LINESPACING:
            if (value) {
                text->linespacing.set=1;
                text->linespacing.unit=SP_SVG_UNIT_PERCENT;
                text->linespacing.value=text->linespacing.computed=sp_svg_read_percentage (value, 1.0);
            } else {
                text->linespacing.set=0;
            }
            object->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG);
            break;
        default:
            if (((SPObjectClass *) text_parent_class)->set)
                ((SPObjectClass *) text_parent_class)->set (object, key, value);
            break;
    }
}

static void
sp_text_child_added (SPObject *object, SPRepr *rch, SPRepr *ref)
{
    //SPItem *item = SP_ITEM (object);
    SPText *text = SP_TEXT (object);

    if (((SPObjectClass *) text_parent_class)->child_added)
        ((SPObjectClass *) text_parent_class)->child_added (object, rch, ref);

    sp_text_request_relayout (text, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_CONTENT_MODIFIED_FLAG);
    /* fixme: Instead of forcing it, do it when needed */
    sp_text_update_immediate_state (text);
}

static void
sp_text_remove_child (SPObject *object, SPRepr *rch)
{
    SPText *text = SP_TEXT (object);

    if (((SPObjectClass *) text_parent_class)->remove_child)
        ((SPObjectClass *) text_parent_class)->remove_child (object, rch);

    sp_text_request_relayout (text, SP_OBJECT_MODIFIED_FLAG | SP_TEXT_CONTENT_MODIFIED_FLAG);
    sp_text_update_immediate_state (text);
}

static void
sp_text_update (SPObject *object, SPCtx *ctx, guint flags)
{
    SPText *text = SP_TEXT (object);

    if (((SPObjectClass *) text_parent_class)->update)
        ((SPObjectClass *) text_parent_class)->update (object, ctx, flags);

    guint cflags = (flags & SP_OBJECT_MODIFIED_CASCADE);
    if (flags & SP_OBJECT_MODIFIED_FLAG) cflags |= SP_OBJECT_PARENT_MODIFIED_FLAG;


    /* Create temporary list of children */
    GSList *l = NULL;
    for (SPObject *child = sp_object_first_child(object) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
        sp_object_ref (SP_OBJECT (child), object);
        l = g_slist_prepend (l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        SPObject *child = SP_OBJECT (l->data);
        l = g_slist_remove (l, child);
        if (cflags || (child->uflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            /* fixme: Do we need transform? */
            child->updateDisplay(ctx, cflags);
        }
        sp_object_unref (SP_OBJECT (child), object);
    }
    if ( text->relayout
         || (flags & ( SP_OBJECT_STYLE_MODIFIED_FLAG |
                       SP_OBJECT_CHILD_MODIFIED_FLAG |
                       SP_TEXT_LAYOUT_MODIFIED_FLAG   )) )
    {
        /* fixme: It is not nice to have it here, but otherwise children content changes does not work */
        /* fixme: Even now it may not work, as we are delayed */
        /* fixme: So check modification flag everywhere immediate state is used */
        sp_text_update_immediate_state (text);
        sp_text_set_shape (text);
        text->relayout = FALSE;
    }
}

static void
sp_text_modified (SPObject *object, guint flags)
{
    if (((SPObjectClass *) text_parent_class)->modified)
        ((SPObjectClass *) text_parent_class)->modified (object, flags);

    guint cflags = (flags & SP_OBJECT_MODIFIED_CASCADE);
    if (flags & SP_OBJECT_MODIFIED_FLAG) cflags |= SP_OBJECT_PARENT_MODIFIED_FLAG;

    /* Create temporary list of children */
    GSList *l = NULL;
    SPObject *child;
    for (child = sp_object_first_child(object) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
        sp_object_ref (SP_OBJECT (child), object);
        l = g_slist_prepend (l, child);
    }
    l = g_slist_reverse (l);
    while (l) {
        child = SP_OBJECT (l->data);
        l = g_slist_remove (l, child);
        if (cflags || (child->mflags & (SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_CHILD_MODIFIED_FLAG))) {
            child->emitModified(cflags);
        }
        sp_object_unref (SP_OBJECT (child), object);
    }
}

static SPRepr *
sp_text_write (SPObject *object, SPRepr *repr, guint flags)
{
    SPText *text = SP_TEXT (object);

    // when stamping, _write is called on the newly created object before _set_shape, so we need to
    // reflow here too (bug 1077559)
    if (flags & SP_OBJECT_WRITE_EXT)
        text->UpdateFlowSource();

    if (flags & SP_OBJECT_WRITE_BUILD) {
        if (!repr)
            repr = sp_repr_new ("svg:text");
        GSList *l = NULL;
        for (SPObject *child = sp_object_first_child(object) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
            SPRepr *crepr = NULL;
            if (SP_IS_TSPAN (child)) {
                crepr = child->updateRepr(NULL, flags);
            } else if (SP_IS_TEXTPATH (child)) {
                crepr = child->updateRepr(NULL, flags);
            } else {
                crepr = sp_xml_document_createTextNode (sp_repr_document (repr), SP_STRING_TEXT (child));
            }
            if (crepr) l = g_slist_prepend (l, crepr);
        }
        while (l) {
            sp_repr_add_child (repr, (SPRepr *) l->data, NULL);
            sp_repr_unref ((SPRepr *) l->data);
            l = g_slist_remove (l, l->data);
        }
    } else {
        for (SPObject *child = sp_object_first_child(object) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
            if (SP_IS_TSPAN (child)) {
                child->updateRepr(flags);
            } else if (SP_IS_TEXTPATH (child)) {
                child->updateRepr(flags);
            } else {
                sp_repr_set_content (SP_OBJECT_REPR (child), (SP_STRING_TEXT (child))?SP_STRING_TEXT (child):"");
            }
        }
    }

    char *nlist=NULL;
    if ( (nlist=text->contents.GetX()) ) {
        if (text->x.set)
            sp_repr_set_attr (repr, "x", nlist); 
        else 
            sp_repr_set_attr (repr, "x", NULL);
        g_free(nlist);
    } else {
        if ( text->x.set ) sp_repr_set_double (repr, "x", text->x.computed); else sp_repr_set_attr (repr, "x", NULL);
    }
    if ( text->y.set && (nlist=text->contents.GetY()) ) {
        if (text->y.set)
            sp_repr_set_attr (repr, "y", nlist);
        else 
            sp_repr_set_attr (repr, "y", NULL);
        g_free(nlist);
    } else {
        if ( text->y.set ) sp_repr_set_double (repr, "y", text->y.computed); else sp_repr_set_attr (repr, "y", NULL);
    }
    if ( (nlist=text->contents.GetDX()) ) {
        sp_repr_set_attr(repr,"dx",nlist);
        g_free(nlist);
    } else {
        sp_repr_set_attr (repr, "dx", NULL);
    }
    if ( (nlist=text->contents.GetDY()) ) {
        sp_repr_set_attr(repr,"dy",nlist);
        g_free(nlist);
    } else {
        sp_repr_set_attr (repr, "dy", NULL);
    }
    if ( (nlist=text->contents.GetRot()) ) {
        sp_repr_set_attr(repr,"rotate",nlist);
        g_free(nlist);
    } else {
        sp_repr_set_attr (repr, "rotate", NULL);
    }

    if (((SPObjectClass *) (text_parent_class))->write)
        ((SPObjectClass *) (text_parent_class))->write (object, repr, flags);

    return repr;
}

static void
sp_text_bbox(SPItem const *item, NRRect *bbox, NR::Matrix const &transform, unsigned const /*flags*/)
{
    SPText *group = SP_TEXT(item);
    if ( group->f_res ) group->f_res->BBox(bbox,transform);
}



static NRArenaItem *
sp_text_show(SPItem *item, NRArena *arena, unsigned /* key*/, unsigned /*flags*/)
{
    SPText *group = (SPText *) item;

    NRArenaGroup *flowed = NRArenaGroup::create(arena);
    nr_arena_group_set_transparent (flowed, FALSE);

    // pass the bbox of the text object as paintbox (used for paintserver fills)
    NRRect paintbox;
    sp_item_invoke_bbox(item, &paintbox, NR::identity(), TRUE);
    group->BuildFlow(flowed, &paintbox);

    return flowed;
}

static void
sp_text_hide(SPItem *item, unsigned key)
{
    if (((SPItemClass *) text_parent_class)->hide)
        ((SPItemClass *) text_parent_class)->hide (item, key);
}

static char *
sp_text_description(SPItem *item)
{
    SPText *text = (SPText *) item;
    SPStyle *style = SP_OBJECT_STYLE(text);

    font_instance *tf = (font_factory::Default())->Face(style->text->font_family.value,
                                                        font_style_to_pos(*style));
    char name_buf[256];
    char const *n;
    if (tf) {
        tf->Name(name_buf, sizeof(name_buf));
        n = name_buf;
        tf->Unref();
    } else {
        n = _("<no name found>");
    }

    if (SP_IS_TEXT_TEXTPATH(item)) {
        return g_strdup_printf (_("<b>Text on path</b> (%s, %.5gpt)"), n, style->font_size.computed );
    } else {
        return g_strdup_printf (_("<b>Text</b> (%s, %.5gpt)"), n, style->font_size.computed );
    }
}


static void
sp_text_set_shape (SPText *text)
{
    // brutal: reflow at each change
    text->UpdateFlowSource();
    text->ComputeFlowRes();

    if ( text->f_res ) {
        text->f_res->ApplyLetterSpacing();
        text->f_res->ComputeIntervals();
        // change the kern_y format to get actual dy values
        for (int i=0;i<text->f_res->nbChunk;i++) {
            text->f_res->ComputeDY(i);
        }

        // use the dimensions of the text chunks to position lines for tspans with sodipodi:role = line
        double    cur_x=0,cur_y=0;
        double    cur_a=0,cur_d=0,cur_l=0;
        bool      use_linespacing=false;
        if ( text->x.set ) cur_x=text->x.computed;
        if ( text->y.set ) cur_y=text->y.computed;
        if ( text->linespacing.set ) {
            // use the linespacing computed for the text object
            use_linespacing=true;
            cur_l=text->linespacing.value*(SP_OBJECT_STYLE(SP_OBJECT(text))->font_size.computed);
            cur_a=cur_d=0;
        } else {
            use_linespacing=true;
            cur_l=SP_OBJECT_STYLE(SP_OBJECT(text))->font_size.computed; // should be using the max of the font-size on the line instead
            cur_a=cur_d=0;
        }
        for (int i=0;i<text->f_src->nbElem;i++) {
            if ( text->f_src->elems[i].type != flw_text ) continue; // nothing to do
            SPObject *tst_o = text->f_src->elems[i].text->source_start->me;
            if ( !(SP_IS_TSPAN(tst_o)) ) continue;
            one_flow_src *div_o = text->f_src->elems[i].text->source_start;
            int  div_o_type = ( div_o ? div_o->Type() : flw_none );
            if ( div_o_type == txt_span ) {
                // has its x/y set, or first tspan of the text
                SPTSpan *cspan = SP_TSPAN(div_o->me);
                if ( cspan->x.set ) cur_x=cspan->x.computed; else cspan->x.value=cspan->x.computed=cur_x;
                if ( cspan->y.set ) cur_y=cspan->y.computed; else cspan->y.value=cspan->y.computed=cur_y;
                continue;
            }
            if ( div_o_type != txt_firstline && div_o_type != txt_tline ) continue;
            SPTSpan *tspan = SP_TSPAN(div_o->me);
            SPStyle *tspan_style = SP_OBJECT_STYLE(tspan);
            flow_res::flow_styled_chunk *cur = NULL;
            for (int j = 0; j < text->f_res->nbChunk; j++) {
                if ( text->f_res->chunks[j].mommy == text->f_src->elems[i].text ) {
                    // found
                    cur=&(text->f_res->chunks[j]);
                    break;
                }
            }
            if ( cur == NULL || use_linespacing == true ) {
                if ( tspan->role != SP_TSPAN_ROLE_UNSPECIFIED ) {
                    if ( /*tspan_style->writing_mode.set &&*/ tspan_style->writing_mode.computed == SP_CSS_WRITING_MODE_TB ) {
                        if ( div_o_type != txt_firstline ) cur_x=cur_x+cur_d+cur_a+cur_l; // the chunk box is rotated
                    } else {
                        if ( div_o_type != txt_firstline )  cur_y=cur_y+cur_d+cur_a+cur_l;
                    }
                }
            } else {
                if ( tspan->role != SP_TSPAN_ROLE_UNSPECIFIED ) {
                    if ( /*tspan_style->writing_mode.set &&*/ tspan_style->writing_mode.computed == SP_CSS_WRITING_MODE_TB ) {
                        if ( div_o_type != txt_firstline ) cur_x=cur_x+cur_d+cur->ascent+cur->leading; // the chunk box is rotated
                    } else {
                        if ( div_o_type != txt_firstline ) cur_y=cur_y+cur_d+cur->ascent+cur->leading;
                    }
                }
                if ( use_linespacing == false ) {
                    cur_d=cur->descent;
                    cur_a=cur->ascent;
                    cur_l=cur->leading;
                }
            }
            tspan->x.computed=cur_x;
            tspan->x.set=1;
            tspan->y.computed=cur_y;
            tspan->y.set=1;
            // this was labeled as 'evil' in the old sp-text.cpp
            gboolean saved = sp_document_get_undo_sensitive(SP_OBJECT_DOCUMENT (tspan));
            sp_document_set_undo_sensitive (SP_OBJECT_DOCUMENT (tspan), FALSE);
            sp_repr_set_double (SP_OBJECT_REPR (tspan), "x", cur_x);
            sp_repr_set_double (SP_OBJECT_REPR (tspan), "y", cur_y);
            sp_document_set_undo_sensitive (SP_OBJECT_DOCUMENT (tspan), saved);
        }
        // the layout was done with lines all starting at (0,0) -> translate to the correct position, according to the start of each chunk
        cur_x=cur_y=0;
        if ( text->x.set ) cur_x=text->x.computed;
        if ( text->y.set ) cur_y=text->y.computed;
        for (int i=0;i<text->f_src->nbElem;i++) {
            if ( text->f_src->elems[i].type != flw_text ) continue; // nothing to do
            SPObject *tst_o = text->f_src->elems[i].text->source_start->me;
            if ( !(SP_IS_TSPAN(tst_o)) && !(SP_IS_TEXT(tst_o)) && !(SP_IS_TEXTPATH(tst_o)) ) continue;
            one_flow_src *div_o = text->f_src->elems[i].text->source_start;
            int div_o_type = ( div_o ? div_o->Type() : flw_none );

            flow_res::flow_styled_chunk *cur = NULL;
            int                          cur_no = -1;
            for (int j = 0; j < text->f_res->nbChunk; j++) {
                if ( text->f_res->chunks[j].mommy == text->f_src->elems[i].text ) {
                    // found
                    cur = &(text->f_res->chunks[j]);
                    cur_no = j;
                    break;
                }
            }
            if ( cur == NULL || cur_no < 0 ) continue;

            if ( div_o_type == txt_text ) {
                //printf("text %f %f\n",cur_x,cur_y);
                SPStyle *text_style = SP_OBJECT_STYLE(text);
                if ( /*text_style->writing_mode.set &&*/ text_style->writing_mode.computed == SP_CSS_WRITING_MODE_TB ) {
                    text->f_res->Verticalize(cur_no,cur_x,cur_y);
                } else {
                    // gentle horizontal text
                    int tr_start=1;
                    //if ( text_style->text_anchor.set ) {
                    if ( text_style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_START ) tr_start=1;
                    if ( text_style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_END ) tr_start=0;
                    if ( text_style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_MIDDLE ) {
                        cur_x-=0.5*(cur->x_en-cur->x_st);
                        tr_start=1;
                    }
                    //}
                    text->f_res->TranslateChunk(cur_no,cur_x,cur_y,(tr_start));
                }
            } else if ( div_o_type == txt_tline || div_o_type == txt_firstline) {
                SPTSpan* tspan=SP_TSPAN(tst_o);
                cur_x=cur_y=0;
                cur_x=tspan->x.computed;
                cur_y=tspan->y.computed;
                //printf("tline|firstline %f %f\n",cur_x,cur_y);
                SPStyle* tspan_style=SP_OBJECT_STYLE(tspan);
                if ( /*tspan_style->writing_mode.set &&*/ tspan_style->writing_mode.computed == SP_CSS_WRITING_MODE_TB ) {
                    text->f_res->Verticalize(cur_no,cur_x,cur_y);
                } else {
                    // gentle horizontal text
                    int tr_start=1;
                    //if ( tspan_style->text_anchor.set ) {
                    if ( tspan_style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_START ) tr_start=1;
                    if ( tspan_style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_END ) tr_start=0;
                    if ( tspan_style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_MIDDLE ) {
                        cur_x-=0.5*(cur->x_en-cur->x_st);
                        tr_start=1;
                    }
                    //}
                    text->f_res->TranslateChunk(cur_no,cur_x,cur_y,(tr_start));
                }
            } else if ( div_o_type == txt_span ) {
                SPTSpan* tspan=SP_TSPAN(tst_o);
                cur_x=cur_y=0;
                cur_x=tspan->x.computed;
                cur_y=tspan->y.computed;
                //printf("tspan %f %f\n",cur_x,cur_y);
                SPStyle* tspan_style=SP_OBJECT_STYLE(tspan);
                if ( /*tspan_style->writing_mode.set &&*/ tspan_style->writing_mode.computed == SP_CSS_WRITING_MODE_TB ) {
                    text->f_res->Verticalize(cur_no,cur_x,cur_y);
                } else {
                    // gentle horizontal text
                    int tr_start=1;
                    //if ( tspan_style->text_anchor.set ) {
                    if ( tspan_style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_START ) tr_start=1;
                    if ( tspan_style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_END ) tr_start=0;
                    if ( tspan_style->text_anchor.computed == SP_CSS_TEXT_ANCHOR_MIDDLE ) {
                        cur_x-=0.5*(cur->x_en-cur->x_st);
                        tr_start=1;
                    }
                    //}
                    text->f_res->TranslateChunk(cur_no,cur_x,cur_y,(tr_start));
                }
            } else if ( div_o_type == txt_textpath ) {
                SPTextPath* textpath=SP_TEXTPATH(tst_o);
                text->f_res->ApplyPath(cur_no,textpath->originalPath);
            } else {
                // no owner, this is just an ordinary tspan
                // shouldn't happen, chunks always have a good definition
                //printf("nothing %f %f\n",cur_x,cur_y);
                text->f_res->TranslateChunk(cur_no,cur_x,cur_y,true);
            }
        }
    }


    NRRect paintbox;
    sp_item_invoke_bbox(SP_ITEM(text), &paintbox, NR::identity(), TRUE);
    for (SPItemView* v = SP_ITEM(text)->display; v != NULL; v = v->next) {
        // pass the bbox of the text object as paintbox (used for paintserver fills)
        text->BuildFlow(NR_ARENA_GROUP(v->arenaitem), &paintbox);
    }

}


static void sp_text_snappoints(SPItem const *item, SnapPointsIter p)
{
    if (((SPItemClass *) text_parent_class)->snappoints) {
        ((SPItemClass *) text_parent_class)->snappoints (item, p);
    }
}

void
sp_text_adjust_fontsize_recursive (SPItem *item, double ex)
{
    SPStyle *style = SP_OBJECT_STYLE (item);

    if (style && !NR_DF_TEST_CLOSE (ex, 1.0, NR_EPSILON)) {
        style->font_size.computed *= ex;
        if (style->text) {
            style->text->letterspacing.computed *= ex;
            style->text->wordspacing.computed *= ex;
        }
        SP_OBJECT(item)->updateRepr();
    }

    for (SPObject *o = SP_OBJECT(item)->children; o != NULL; o = o->next) {
        if (SP_IS_ITEM(o))
            sp_text_adjust_fontsize_recursive (SP_ITEM(o), ex);
    }
}

void
sp_text_adjust_coords_recursive (SPItem *item, NR::Matrix const &m, double ex)
{
    div_flow_src *contents = NULL; 
    SPSVGLength *x = NULL, *y = NULL;
    // if someone knows how to make this less ugly, you're welcome:
    if (SP_IS_TSPAN(item)) {
        contents = &(SP_TSPAN(item)->contents);
        x = &(SP_TSPAN(item)->x);
        y = &(SP_TSPAN(item)->y);
    } else if (SP_IS_TEXT(item)) {
        contents = &(SP_TEXT(item)->contents);
        x = &(SP_TEXT(item)->x);
        y = &(SP_TEXT(item)->y);
    }

    if (contents) {
        if (!(SP_IS_TSPAN(item) && SP_TSPAN(item)->role == SP_TSPAN_ROLE_LINE)) {
            // Do not touch x/y for line tspans, because they will get updated automatically from parent

            /* Recalculate x/y lists */
            contents->TransformXY (m, SP_IS_TEXT(item));
            if ( contents->nb_x > 0 ) {
                *x = contents->x_s[0];
                x->set = 1;
            } else {
                x->set = 0;
            }
            if ( contents->nb_y > 0 ) {
                *y = contents->y_s[0];
                y->set = 1;
            } else {
                y->set = 0;
            }
        }

        /* Recalculate dx/dy lists */
        if (sp_repr_attr(SP_OBJECT_REPR(item), "dx") || sp_repr_attr(SP_OBJECT_REPR(item), "dy")) {
          // FIXME: The entire chain of ancestors share the same dx/dy lists, but we only want to
          // scale it once, so we do it for the element that has the actual attributes. This is
          // STILL not 100% correct because the actual list may be the result of a combination of
          // several dx/dy attrs in an ancestor chain.
            contents->ScaleDXDY (ex);
        }
    }

    for (SPObject *o = SP_OBJECT(item)->children; o != NULL; o = o->next) {
        if (SP_IS_ITEM(o))
            sp_text_adjust_coords_recursive (SP_ITEM(o), m, ex);
    }
}

static NR::Matrix
sp_text_set_transform (SPItem *item, NR::Matrix const &xform)
{
    SPText *text = SP_TEXT(item);

    // we cannot optimize textpath because changing its fontsize will break its match to the path
    if (SP_IS_TEXT_TEXTPATH (text))
        return xform;

    /* This function takes care of scaling & translation only, we return whatever parts we can't
       handle. */
    double const ex = fontsize_expansion(xform);
    if (ex == 0) {
        return xform;
    }

    NR::Matrix ret(NR::transform(xform));
    ret[0] /= ex;
    ret[1] /= ex;
    ret[2] /= ex;
    ret[3] /= ex;

    // Adjust x/y, dx/dy
    sp_text_adjust_coords_recursive (item, xform * ret.inverse(), ex);

    // Adjust font size
    sp_text_adjust_fontsize_recursive (item, ex);

    // Adjust stroke width
    sp_shape_adjust_stroke(item, ex);

    // Adjust pattern fill
    sp_shape_adjust_pattern(item, xform * ret.inverse());

    // Adjust gradient fill
    sp_shape_adjust_gradient(item, xform * ret.inverse());

    item->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG);

    return ret;
}

static void
sp_text_print (SPItem *item, SPPrintContext *ctx)
{
    NRRect     pbox, dbox, bbox;
    SPText *group = SP_TEXT (item);

    sp_item_invoke_bbox(item, &pbox, NR::identity(), TRUE);
    sp_item_bbox_desktop (item, &bbox);
    dbox.x0 = 0.0;
    dbox.y0 = 0.0;
    dbox.x1 = sp_document_width (SP_OBJECT_DOCUMENT (item));
    dbox.y1 = sp_document_height (SP_OBJECT_DOCUMENT (item));
    NRMatrix ctm;
    sp_item_i2d_affine (item, &ctm);

    if ( group->f_res ) group->f_res->Print(ctx,&pbox,&dbox,&bbox,ctm);
}


gchar *
sp_text_get_string_multiline (SPText *text)
{
    if ( text->f_src == NULL ) text->UpdateFlowSource();
    if ( text->f_src ) {
        char* res=text->f_src->Summary();
        return res;
    }
    return NULL;
}

void
sp_text_set_repr_text_multiline(SPText *text, gchar const *str)
{
    g_return_if_fail (text != NULL);
    g_return_if_fail (SP_IS_TEXT (text));

    SPRepr *repr;
    bool is_textpath = false;
    if (SP_IS_TEXT_TEXTPATH (text)) {
        repr = SP_OBJECT_REPR (sp_object_first_child(SP_OBJECT (text)));
        is_textpath = true;
    } else {
        repr = SP_OBJECT_REPR (text);
    }
    SPStyle *style = SP_OBJECT_STYLE (text);

    if (!str) str = "";
    gchar *content = g_strdup (str);

    sp_repr_set_content (SP_OBJECT_REPR (text), "");
    while (repr->children) {
        sp_repr_remove_child (repr, repr->children);
    }

    NR::Point cp(text->x.computed, text->y.computed);

    gchar *p = content;
    while (p) {
        gchar *e = strchr (p, '\n');
        if (is_textpath) {
            if (e) *e = ' '; // no lines for textpath, replace newlines with spaces
        } else {
            if (e) *e = '\0'; // create a tspan for each line
            SPRepr *rtspan = sp_repr_new ("svg:tspan");
            sp_repr_set_double (rtspan, "x", cp[NR::X]);
            sp_repr_set_double (rtspan, "y", cp[NR::Y]);
            if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
                cp[NR::X] -= style->font_size.computed;
            } else {
                cp[NR::Y] += style->font_size.computed;
            }
            sp_repr_set_attr (rtspan, "sodipodi:role", "line");
            SPRepr *rstr = sp_xml_document_createTextNode (sp_repr_document (repr), p);
            sp_repr_add_child (rtspan, rstr, NULL);
            sp_repr_unref(rstr);
            sp_repr_append_child (repr, rtspan);
            sp_repr_unref(rtspan);
        }
        p = (e) ? e + 1 : NULL;
    }
    if (is_textpath) { 
        SPRepr *rstr = sp_xml_document_createTextNode (sp_repr_document (repr), content);
        sp_repr_add_child (repr, rstr, NULL);
        sp_repr_unref(rstr);
    }

    g_free (content);
}

SPCurve *
sp_text_normalized_bpath (SPText *text)
{
    g_return_val_if_fail (text != NULL, NULL);
    g_return_val_if_fail (SP_IS_TEXT (text), NULL);

    if ( text->f_res ) return text->f_res->NormalizedBPath();
    return sp_curve_new();
}


static void
sp_text_update_immediate_state (SPText */*text*/)
{
}


static void
sp_text_request_relayout (SPText *text, guint flags)
{
    text->relayout = TRUE;

    SP_OBJECT (text)->requestDisplayUpdate(flags);
}


SPTSpan *
sp_text_append_line(SPText *text)
{
    g_return_val_if_fail (text != NULL, NULL);
    g_return_val_if_fail (SP_IS_TEXT (text), NULL);

    SPStyle *style = SP_OBJECT_STYLE (text);

    NR::Point cp(text->x.computed, text->y.computed);

    for (SPObject *child = sp_object_first_child(SP_OBJECT(text)) ; child != NULL; child = SP_OBJECT_NEXT(child)) {
        if (SP_IS_TSPAN (child)) {
            SPTSpan *tspan = SP_TSPAN (child);
            if (tspan->role == SP_TSPAN_ROLE_LINE) {
                cp[NR::X] = tspan->x.computed;
                cp[NR::Y] = tspan->y.computed;
            }
        }
    }

    /* Create <tspan> */
    SPRepr *rtspan = sp_repr_new ("svg:tspan");
    if (style->writing_mode.computed == SP_CSS_WRITING_MODE_TB) {
        /* fixme: real line height */
        /* fixme: What to do with mixed direction tspans? */
        sp_repr_set_double (rtspan, "x", cp[NR::X] - style->font_size.computed);
        sp_repr_set_double (rtspan, "y", cp[NR::Y]);
    } else {
        sp_repr_set_double (rtspan, "x", cp[NR::X]);
        sp_repr_set_double (rtspan, "y", cp[NR::Y] + style->font_size.computed);
    }
    sp_repr_set_attr (rtspan, "sodipodi:role", "line");

    /* Create TEXT */
    SPRepr *rstring = sp_xml_document_createTextNode (sp_repr_document (rtspan), "");
    sp_repr_add_child (rtspan, rstring, NULL);
    sp_repr_unref (rstring);
    /* Append to text */
    sp_repr_append_child (SP_OBJECT_REPR (text), rtspan);
    sp_repr_unref (rtspan);

    return (SPTSpan *) SP_OBJECT_DOCUMENT (text)->getObjectByRepr(rtspan);
}


int
sp_text_insert_line (SPText *text, gint i_ucs4_pos)
{
    int utf8_pos=text->contents.Do_UCS4_2_UTF8(i_ucs4_pos);
    // no updateRepr in this function because SPRepr are handled directly
    if ( text->f_src == NULL ) return 0;

    // Disable newlines in a textpath; TODO: maybe on Enter in a textpath, separate it into two
    // texpaths attached to the same path, with a vertical shift
    if (SP_IS_TEXT_TEXTPATH (text)) 
        return 0;
	
    int  ucs4_pos=0;
    one_flow_src* into=text->contents.Locate(utf8_pos, ucs4_pos, true, false, false);
    //printf("pos=%i -> %i in %x\n",utf8_pos,ucs4_pos,into);

    if ( into == NULL ) {
        // it's a 'append line' in fact
        SPRepr*   rtspan = sp_repr_new ("svg:tspan");
        sp_repr_set_attr (rtspan, "sodipodi:role", "line");
        SPRepr*   rstring = sp_xml_document_createTextNode (sp_repr_document (rtspan), "");
        sp_repr_add_child (rtspan, rstring, NULL);
        sp_repr_unref (rstring);
        sp_repr_append_child (SP_OBJECT_REPR (text), rtspan);
        sp_repr_unref (rtspan);
        SP_OBJECT(text)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

        return 1;

    } else if ( into && into->dad ) {
        if ( into->Type() == flw_text ) {
            text_flow_src* into_obj=dynamic_cast<text_flow_src*>(into);
            // the "usual" case of a newline in the middle of some text
            // we need to split into->me in 2 parts
            if ( into->dad->Type() == txt_span || into->dad->Type() == txt_tline || into->dad->Type() == txt_firstline ) {
                div_flow_src* into_dad=dynamic_cast<div_flow_src*>(into->dad);
                SPRepr*       into_repr=SP_OBJECT_REPR(into_dad->me);
                if ( into->dad->dad ) {
                    // just in case
                    //sp_repr_set_attr (into_repr, "sodipodi:role", "line");
                    // create the new tspan
                    SPRepr*   rtspan = sp_repr_new ("svg:tspan");
                    sp_repr_set_attr (rtspan, "sodipodi:role", "line");
                    SPRepr*   rstring = NULL;
                    if ( into_obj->utf8_st < into_obj->utf8_en ) {
                        rstring=sp_xml_document_createTextNode (sp_repr_document (rtspan), into_obj->cleaned_up.utf8_text+(utf8_pos-into_obj->utf8_st));
                    } else {
                        rstring=sp_xml_document_createTextNode (sp_repr_document (rtspan), NULL);
                    }
                    sp_repr_add_child (rtspan, rstring, NULL);
                    sp_repr_unref (rstring);
                    sp_repr_add_child (SP_OBJECT_REPR (into->dad->dad->me), rtspan,SP_OBJECT_REPR(into->dad->me));
                    sp_repr_unref (rtspan);
                    char*  nval=into_dad->GetX(ucs4_pos-into_dad->ucs4_en,-1);
                    if ( nval ) {
                        sp_repr_set_attr(rtspan,"x",nval);
                    }
                    nval=into_dad->GetY(ucs4_pos-into_dad->ucs4_en,-1);
                    if ( nval ) {
                        sp_repr_set_attr(rtspan,"y",nval);
                    }
                    nval=into_dad->GetDX(ucs4_pos-into_dad->ucs4_en,-1);
                    if ( nval ) {
                        sp_repr_set_attr(rtspan,"dx",nval);
                    }
                    nval=into_dad->GetDY(ucs4_pos-into_dad->ucs4_en,-1);
                    if ( nval ) {
                        sp_repr_set_attr(rtspan,"dy",nval);
                    }
                    nval=into_dad->GetRot(ucs4_pos-into_dad->ucs4_en,-1);
                    if ( nval ) {
                        sp_repr_set_attr(rtspan,"rotate",nval);
                    }
                    // cut the old at the given position
                    if ( into_obj->utf8_st < into_obj->utf8_en ) {
                        //char savC=into_obj->cleaned_up.utf8_text[utf8_pos-into_obj->utf8_st];
                        into_obj->cleaned_up.utf8_text[utf8_pos-into_obj->utf8_st]=0;
                        sp_repr_set_content(SP_OBJECT_REPR(into->me),(into_obj->cleaned_up.utf8_text)?into_obj->cleaned_up.utf8_text:"");
                        //into_obj->cleaned_up.utf8_text[utf8_pos-into_obj->utf8_st]=savC; // has been invalidated by the previous call, through the sp-string stuff
                    }
                    nval=into_dad->GetX(0,ucs4_pos-into_dad->ucs4_en);
                    if ( nval ) {
                        sp_repr_set_attr(into_repr,"x",nval);
                    }
                    nval=into_dad->GetY(0,ucs4_pos-into_dad->ucs4_en);
                    if ( nval ) {
                        sp_repr_set_attr(into_repr,"y",nval);
                    }
                    nval=into_dad->GetDX(0,ucs4_pos-into_dad->ucs4_en);
                    if ( nval ) {
                        sp_repr_set_attr(into_repr,"dx",nval);
                    }
                    nval=into_dad->GetDY(0,ucs4_pos-into_dad->ucs4_en);
                    if ( nval ) {
                        sp_repr_set_attr(into_repr,"dy",nval);
                    }
                    nval=into_dad->GetRot(0,ucs4_pos-into_dad->ucs4_en);
                    if ( nval ) {
                        sp_repr_set_attr(into_repr,"rotate",nval);
                    }
                    // transfer the children of into->dad that lie after into to the new tspan
                    GList* templ=NULL;
                    for (SPObject* child=into->me->next;child;child=child->next) {
                        templ=g_list_append(templ,child);
                    }
                    for (GList *l = templ; l; l = l->next) {
                        SPObject *child = (SPObject*) l->data;
                        SPRepr *c_repr = SP_OBJECT_REPR(child);
                        sp_repr_ref(c_repr);
                        sp_repr_remove_child(into_repr, c_repr);
                        sp_repr_append_child(rtspan, c_repr);
                        sp_repr_unref(c_repr);
                    }
                    g_list_free(templ);
                    SP_OBJECT(text)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

                    return 1;

                } else {
                    // since it's a string, there's always at least the sp-text for dad
                }

            } else if ( into->dad->Type() == txt_text ) {
                // special case
                SPRepr*   firstspan = sp_repr_new ("svg:tspan");
                SPRepr*   rtspan = sp_repr_new ("svg:tspan");
                sp_repr_set_attr (firstspan, "sodipodi:role", "line");
                sp_repr_set_attr (rtspan, "sodipodi:role", "line");
                SPRepr*   firststring =NULL;
                SPRepr*   rstring =NULL;
                if ( into_obj->utf8_st < into_obj->utf8_en ) {
                    char savC=into_obj->cleaned_up.utf8_text[utf8_pos-into_obj->utf8_st];
                    into_obj->cleaned_up.utf8_text[utf8_pos-into_obj->utf8_st]=0;
                    firststring = sp_xml_document_createTextNode (sp_repr_document (firstspan), into_obj->cleaned_up.utf8_text);
                    into_obj->cleaned_up.utf8_text[utf8_pos-into_obj->utf8_st]=savC;
                    rstring = sp_xml_document_createTextNode (sp_repr_document (rtspan), into_obj->cleaned_up.utf8_text+(utf8_pos-into_obj->utf8_st));
                } else {
                    firststring = sp_xml_document_createTextNode (sp_repr_document (firstspan), NULL);
                    rstring = sp_xml_document_createTextNode (sp_repr_document (rtspan), NULL);
                }
                sp_repr_add_child (rtspan, rstring, NULL);
                sp_repr_unref (rstring);
                sp_repr_add_child (firstspan, firststring, NULL);
                sp_repr_unref (firststring);
                // remove old string
                sp_repr_remove_child(SP_OBJECT_REPR(into->dad->me), SP_OBJECT_REPR(into->me));
                // add 2 lines
                sp_repr_append_child (SP_OBJECT_REPR (into->dad->me), firstspan);
                sp_repr_unref (firstspan);
                sp_repr_append_child (SP_OBJECT_REPR (into->dad->me), rtspan);
                sp_repr_unref (rtspan);
                SP_OBJECT(text)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

                return 1;

            }
        } else if ( into->Type() == txt_text ) {
            // ???
        } else if ( into->Type() == txt_textpath ) {
            // what to do in this case?
        } else if ( into->Type() == txt_firstline || into->Type() == txt_tline ) {
            // insert a new empty tspan in front of this one
            if ( into->dad ) {
                SPRepr*   rtspan = sp_repr_new ("svg:tspan");
                sp_repr_set_attr (rtspan, "sodipodi:role", "line");
                SPRepr*   rstring = sp_xml_document_createTextNode (sp_repr_document (rtspan), "");
                SPObject* prec=NULL;
                for (SPObject* child = sp_object_first_child(into->dad->me) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
                    if ( child == into->me ) break;
                    prec=child;
                }
                sp_repr_add_child (rtspan, rstring, NULL);
                sp_repr_unref (rstring);
                sp_repr_add_child (SP_OBJECT_REPR (into->dad->me), rtspan,(prec)?SP_OBJECT_REPR(prec):NULL);
                sp_repr_unref (rtspan);
                SP_OBJECT(text)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

                return 1;

            } else {
                // since it's a tline, there's always at least the sp-text for dad
            }
        } else if ( into->Type() == txt_span ) {
            // add a sodipodi:role=line and we're done
            sp_repr_set_attr (SP_OBJECT_REPR (into->me), "sodipodi:role", "line");
            SP_OBJECT(text)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);

            return 1;
        }
    }
    return 0;
}

gint
sp_text_append (SPText */*text*/, gchar const */*utf8*/)
{
    return 0;
}

static SPObject *
sp_text_get_child_by_position (SPText *text, gint i_ucs4_pos)
{
 	int utf8_pos=text->contents.Do_UCS4_2_UTF8(i_ucs4_pos);
   if ( text->f_res == NULL ) return NULL;
    int   ucs4_pos=0;
    one_flow_src *into = text->contents.Locate(utf8_pos,ucs4_pos, true, true, true);
    //printf("ucs4 at offset %i = %i -> txt=%x",utf8_pos,ucs4_pos,into);
    if ( into && into->Type() == flw_text ) {
        if ( into->dad ) return into->dad->me;
    }
    return NULL;
}

void
sp_adjust_kerning_screen (SPText *text, gint i_position, SPDesktop *desktop, NR::Point by)
{
	int position=text->contents.Do_UCS4_2_UTF8(i_position);
    if ( text->f_src == NULL ) return;
    one_flow_src* cur=&text->contents;

    // divide increment by zoom
    // divide increment by matrix expansion
    gdouble factor = 1 / SP_DESKTOP_ZOOM (desktop);
    NR::Matrix t = sp_item_i2doc_affine (SP_ITEM(text));
    factor = factor / NR::expansion(t);
    by = factor * by;

    SPSVGLength  by_x,by_y;
    by_x.set=by_y.set=1;
    by_x.value=by_x.computed=by[0];
    by_y.value=by_y.computed=by[1];
    while ( cur ) {
        cur->AddValue(position, by_x, 2, true, false);
        cur->AddValue(position, by_y, 3, true, false);
        cur=cur->next;
    }
    SP_OBJECT(text)->updateRepr();
    SP_OBJECT(text)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

void
sp_adjust_tspan_letterspacing_screen(SPText *text, gint i_position, SPDesktop *desktop, gdouble by)
{
    gdouble val;
    int     nb_let=0;
    SPObject *child = sp_text_get_child_by_position (text, i_position);
    if (!child) return;
    {
        int    c_p=-1,s_p=-1,l_p=-1;
        bool   l_start=false,l_end=false;
        //printf("letter at offset %i : ",pos);
        text->f_res->OffsetToLetter(i_position, c_p, s_p, l_p, l_start, l_end);
        //printf(" c=%i s=%i l=%i st=%i en=%i ",c_p,s_p,l_p,(l_start)?1:0,(l_end)?1:0);
        if ( c_p >= 0 ) {
            nb_let = text->f_res->chunks[c_p].l_en - text->f_res->chunks[c_p].l_st;
            // printf(" -> nblet %i \n",nb_let);
        } else {
            //printf("none\n");
        }
    }
    SPStyle *style = SP_OBJECT_STYLE (child);

    // calculate real value
    /* TODO: Consider calculating val unconditionally, i.e. drop the first `if' line, and
       get rid of the `else val = 0.0'.  Similarly below and in sp-string.cpp. */
    if (style->text->letterspacing.value != 0 && style->text->letterspacing.computed == 0) { // set in em or ex
        if (style->text->letterspacing.unit == SP_CSS_UNIT_EM) {
            val = style->font_size.computed * style->text->letterspacing.value;
        } else if (style->text->letterspacing.unit == SP_CSS_UNIT_EX) {
            val = style->font_size.computed * style->text->letterspacing.value * 0.5;
        } else { // unknown unit - should not happen
            val = 0.0;
        }
    } else { // there's a real value in .computed, or it's zero
        val = style->text->letterspacing.computed;
    }

    // divide increment by zoom and by the number of characters in the line,
    // so that the entire line is expanded by by pixels, no matter what its length
    gdouble const zoom = SP_DESKTOP_ZOOM(desktop);
    gdouble const zby = (by
                         / (zoom * (nb_let > 1 ? nb_let - 1 : 1))
                         / NR::expansion(sp_item_i2doc_affine(SP_ITEM(child))));
    val += zby;

    // set back value
    style->text->letterspacing_normal = FALSE;
    if (style->text->letterspacing.value != 0 && style->text->letterspacing.computed == 0) { // set in em or ex
        if (style->text->letterspacing.unit == SP_CSS_UNIT_EM) {
            style->text->letterspacing.value = val / style->font_size.computed;
        } else if (style->text->letterspacing.unit == SP_CSS_UNIT_EX) {
            style->text->letterspacing.value = val / style->font_size.computed * 2;
        }
    } else {
        style->text->letterspacing.computed = val;
    }

    style->text->letterspacing.set = TRUE;

    gchar *str = sp_style_write_difference (style, SP_OBJECT_STYLE (SP_OBJECT (text)));
    sp_repr_set_attr (SP_OBJECT_REPR (child), "style", str);
    g_free (str);
}

void
sp_adjust_linespacing_screen (SPText *text, SPDesktop *desktop, gdouble by)
{
    SPStyle *style = SP_OBJECT_STYLE (text);

    // the value is stored as multiple of font size (i.e. in em)
    double val = style->font_size.computed * text->linespacing.value;

    // calculate the number of lines
    SPObject *child;
    int lines = 0;
    for (child = sp_object_first_child(SP_OBJECT(text)) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
        if (SP_IS_TSPAN (child) && SP_TSPAN (child)->role == SP_TSPAN_ROLE_LINE) {
            lines ++;
        }
    }

    // divide increment by zoom and by the number of lines,
    // so that the entire object is expanded by by pixels
    gdouble zoom = SP_DESKTOP_ZOOM (desktop);
    gdouble zby = by / (zoom * (lines > 1 ? lines - 1 : 1));

    // divide increment by matrix expansion
    NR::Matrix t = sp_item_i2doc_affine (SP_ITEM(text));
    zby = zby / NR::expansion(t);

    val += zby;

    // fixme: why not allow it to be negative? needs fixing in many places, though
    if (val < 0) val = 0;

    // set back value
    sp_repr_set_double (SP_OBJECT_REPR (text), "sodipodi:linespacing", val / style->font_size.computed);
    SP_OBJECT (text)->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG | SP_TEXT_LAYOUT_MODIFIED_FLAG);
}

/*
 *
 */


static void TextReLink(SPObject* object, one_flow_src* &after, one_flow_src* from, bool &first_line)
{
    one_flow_src*  mine=NULL;
    if ( SP_IS_TEXT(object) ) {
        SPText* text=SP_TEXT(object);
        mine=&(text->contents);
        if ( /*SP_OBJECT_STYLE(object)->writing_mode.set &&*/ SP_OBJECT_STYLE(object)->writing_mode.computed == SP_CSS_WRITING_MODE_TB ) {
            text->contents.vertical_layout=true;
        } else {
            text->contents.vertical_layout=false;
        }
        text->contents.SetStyle(SP_OBJECT_STYLE(object));
    } else if ( SP_IS_TSPAN(object) ) {
        SPTSpan* tspan=SP_TSPAN(object);
        mine=&(tspan->contents);
        if ( /*SP_OBJECT_STYLE(object)->writing_mode.set &&*/ SP_OBJECT_STYLE(object)->writing_mode.computed == SP_CSS_WRITING_MODE_TB ) {
            tspan->contents.vertical_layout=true;
        } else {
            tspan->contents.vertical_layout=false;
        }
        if ( tspan->role == SP_TSPAN_ROLE_UNSPECIFIED ) {
            tspan->contents.type=txt_span;
            first_line=false;
        } else {
            tspan->contents.type=(first_line)?txt_firstline:txt_tline;
            first_line=false;
        }
        tspan->contents.SetStyle(SP_OBJECT_STYLE(object));
    } else if ( SP_IS_TEXTPATH(object) ) {
        SPTextPath* textpath=SP_TEXTPATH(object);
        mine=&(textpath->contents);
        textpath->contents.SetStyle(SP_OBJECT_STYLE(object));
    } else if ( SP_IS_STRING(object) ) {
        mine=&(SP_STRING(object)->contents);
    } else {
        return;
    }
    if ( SP_IS_TEXT(object) || SP_IS_TSPAN(object) || SP_IS_TEXTPATH(object) ) {
        (dynamic_cast<div_flow_src*>(mine))->UpdateLength(SP_OBJECT_STYLE(object)->font_size.computed,1.0);
    }

    mine->Link(after,from);
    after=mine;

    if ( SP_IS_TEXT(object) || SP_IS_TSPAN(object) || SP_IS_TEXTPATH(object) ) {
        for (SPObject* child = sp_object_first_child(object) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
            TextReLink(child,after,mine,first_line);
        }
    }
}

void SPText::ClearFlow(NRArenaGroup *in_arena)
{
    nr_arena_item_request_render (NR_ARENA_ITEM (in_arena));
    for (NRArenaItem *child = in_arena->children; child != NULL; ) {
        NRArenaItem *nchild = child->next;

        nr_arena_glyphs_group_clear(NR_ARENA_GLYPHS_GROUP(child));
        nr_arena_item_remove_child (NR_ARENA_ITEM (in_arena), child);

        child=nchild;
    }
}

void SPText::BuildFlow(NRArenaGroup* in_arena, NRRect *paintbox)
{
    if ( f_res ) f_res->Show(in_arena, paintbox);
}

void SPText::UpdateFlowSource(void)
{
    SPItem*   item=SP_ITEM((SPText*)this);
    SPObject* object=SP_OBJECT(item);

    // because the text_styles are held by f_src, we need to delete this before, to avoid dangling pointers
    if ( f_res ) {
        for (SPItemView* v = item->display; v != NULL; v = v->next) {
            ClearFlow(NR_ARENA_GROUP(v->arenaitem));
        }
        delete f_res;
        f_res=NULL;
    }

    if ( f_src ) delete f_src;
    f_src=new flow_src;

    one_flow_src *last = NULL;
    bool first_line = true;
    TextReLink(object, last, NULL, first_line);
		{
			one_flow_src *cur=&contents;
			one_flow_src *last_line=NULL;
			while ( cur ) {
				if ( cur->Type() == txt_tline || cur->Type() == txt_firstline) {
					last_line=cur;
					SPTSpan *cspan = SP_TSPAN(cur->me);
					cspan->last_tspan=false;
				}
				cur=cur->next;
			}
			if ( last_line ) {
				SPTSpan *cspan = SP_TSPAN(last_line->me);
				cspan->last_tspan=true;
			}
		}

    contents.DoPositions(true);

    contents.DoFill(f_src);

    f_src->Prepare();
}

void SPText::ComputeFlowRes(void)
{
    //SPItem*   item=SP_ITEM((SPText*)this);
    //SPObject* object=SP_OBJECT(item);

    if ( f_src->nbElem <= 0 ) return;

    flow_maker* f_mak=new flow_maker(f_src,NULL);
    f_mak->justify=false;
    f_mak->par_indent=0;
    f_res=f_mak->TextWork();
    delete f_mak;

    if ( f_res ) {
        f_res->ComputeIntervals();
        f_res->ComputeLetterOffsets();
    }
    //f_res->AfficheOutput();
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
