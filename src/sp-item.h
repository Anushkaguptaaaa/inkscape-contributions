#ifndef __SP_ITEM_H__
#define __SP_ITEM_H__

/** \file
 * Some things pertinent to all visible shapes: SPItem, SPItemView, SPItemCtx, SPItemClass, SPEvent.
 */

/*
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 * Copyright (C) 2004 Monash University
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <libnr/nr-matrix.h>
#include <libnr/nr-rect.h>

#include "display/nr-arena-forward.h"
#include "forward.h"
#include "sp-object.h"
#include <vector>

namespace Inkscape { class URIReference; }
class SPGuideConstraint;

/* fixme: This is just placeholder */
/*
 * Plan:
 * We do extensible event structure, that hold applicable (ui, non-ui)
 * data pointers. So it is up to given object/arena implementation
 * to process correct ones in meaningful way.
 * Also, this probably goes to SPObject base class
 *
 */

enum {
	SP_EVENT_INVALID,
	SP_EVENT_NONE,
	SP_EVENT_ACTIVATE,
	SP_EVENT_MOUSEOVER,
	SP_EVENT_MOUSEOUT
};

struct SPEvent {
	unsigned int type;
	gpointer data;
};

class SPItemView;

struct SPItemView {
	SPItemView *next;
	unsigned int flags;
	unsigned int key;
	/* SPItem *item; */
	NRArenaItem *arenaitem;
};

/* flags */

#define SP_ITEM_BBOX_VISUAL 1

#define SP_ITEM_SHOW_DISPLAY (1 << 0)

/*
 * Flag for referenced views (i.e. clippaths, masks and patterns); always display
 */
#define SP_ITEM_REFERENCE_FLAGS SP_ITEM_SHOW_DISPLAY

class SPItemCtx;

struct SPItemCtx {
	SPCtx ctx;
	/* Item to document transformation */
	NRMatrix i2doc;
	/* Viewport size */
	NRRect vp;
	/* Item to viewport transformation */
	NRMatrix i2vp;
};

/** Abstract base class for all visible shapes. */
struct SPItem : public SPObject {
	unsigned int sensitive : 1;
	unsigned int stop_paint: 1;

	NR::Matrix transform;

	SPClipPathReference *clip_ref;
	SPMaskReference *mask_ref;

	SPItemView *display;

	std::vector<SPGuideConstraint> constraints;

	sigc::signal<void, NR::Matrix const *, SPItem *> _transformed_signal;

	bool isLocked() const;
	void setLocked(bool lock);

	bool isHidden() const;
	void setHidden(bool hidden);

	bool isHidden(unsigned display_key) const;

	bool isExplicitlyHidden() const;

	void setExplicitlyHidden(bool val);

	bool isVisibleAndUnlocked() const;

	bool isVisibleAndUnlocked(unsigned display_key) const;

	NR::Matrix getRelativeTransform(SPObject const *obj) const;

	void raiseOne();
	void lowerOne();
	void raiseToTop();
	void lowerToBottom();

	sigc::connection connectTransformed(sigc::slot<void, NR::Matrix const *, SPItem *> slot)  {
                return _transformed_signal.connect(slot);
        }
};

typedef std::back_insert_iterator<std::vector<NR::Point> > SnapPointsIter;

struct SPItemClass {
	SPObjectClass parent_class;

	/* BBox union in given coordinate system */
	void (* bbox) (SPItem const *item, NRRect *bbox, NR::Matrix const &transform, unsigned const flags);

	/* Printing method. Assumes ctm is set to item affine matrix */
	/* fixme: Think about it, and maybe implement generic export method instead (Lauris) */
	void (* print) (SPItem *item, SPPrintContext *ctx);

	/* Give short description of item (for status display) */
	gchar * (* description) (SPItem * item);

	NRArenaItem * (* show) (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
	void (* hide) (SPItem *item, unsigned int key);

	/* Write to an iterator the points that should be considered for snapping
	** as the item's `nodes'.
	*/
        void (* snappoints) (SPItem const *item, SnapPointsIter p);

	/* Apply the transform optimally, and return any residual transformation */
	NR::Matrix (* set_transform)(SPItem *item, NR::Matrix const &transform);

	/* Emit event, if applicable */
	gint (* event) (SPItem *item, SPEvent *event);
};

/* Flag testing macros */

#define SP_ITEM_STOP_PAINT(i) (SP_ITEM (i)->stop_paint)

/* Methods */

void sp_item_invoke_bbox(SPItem const *item, NRRect *bbox, NR::Matrix const &transform, unsigned const clear);
void sp_item_invoke_bbox_full (SPItem const *item, NRRect *bbox, NR::Matrix const &transform, unsigned const flags, unsigned const clear);

unsigned sp_item_pos_in_parent(SPItem *item);

gchar * sp_item_description (SPItem * item);
void sp_item_invoke_print (SPItem *item, SPPrintContext *ctx);

/* Shows/Hides item on given arena display list */
unsigned int sp_item_display_key_new (unsigned int numkeys);
NRArenaItem *sp_item_invoke_show (SPItem *item, NRArena *arena, unsigned int key, unsigned int flags);
void sp_item_invoke_hide (SPItem *item, unsigned int key);

void sp_item_snappoints(SPItem const *item, SnapPointsIter p);

void sp_item_adjust_pattern (SPItem *item, /* NR::Matrix const &premul, */ NR::Matrix const &postmul, bool set = false);
void sp_item_adjust_gradient (SPItem *item, /* NR::Matrix const &premul, */ NR::Matrix const &postmul, bool set = false);
void sp_item_adjust_stroke (SPItem *item, gdouble ex);

void sp_item_write_transform(SPItem *item, SPRepr *repr, NRMatrix const *transform, NR::Matrix const *adv = NULL);
void sp_item_write_transform(SPItem *item, SPRepr *repr, NR::Matrix const &transform, NR::Matrix const *adv = NULL);

void sp_item_set_item_transform(SPItem *item, NR::Matrix const &transform);

gint sp_item_event (SPItem *item, SPEvent *event);

/* Utility */

NRArenaItem *sp_item_get_arenaitem (SPItem *item, unsigned int key);

void sp_item_bbox_desktop (SPItem *item, NRRect *bbox);
NR::Rect sp_item_bbox_desktop(SPItem *item);

NR::Matrix i2anc_affine(SPObject const *item, SPObject const *ancestor);
NR::Matrix i2i_affine(SPObject const *src, SPObject const *dest);

NR::Matrix sp_item_i2doc_affine(SPItem const *item);
NR::Matrix sp_item_i2root_affine(SPItem const *item);

/* Old NRMatrix version (deprecated). */
NRMatrix *sp_item_i2doc_affine (SPItem const *item, NRMatrix *transform);
NRMatrix *sp_item_i2root_affine(SPItem const *item, NRMatrix *transform);

/* fixme: - these are evil, but OK */

/* Fill *TRANSFORM with the item-to-desktop transform.  See doc/coordinates.txt
 * for a description of `Desktop coordinates'; though see also mental's comment
 * at the top of that file.
 *
 * Returns TRANSFORM.
 */
NR::Matrix sp_item_i2d_affine(SPItem const *item);
NRMatrix *sp_item_i2d_affine(SPItem const *item, NRMatrix *transform);

NR::Matrix sp_item_i2r_affine(SPItem const *item);

void sp_item_set_i2d_affine(SPItem *item, NR::Matrix const &transform);

NR::Matrix sp_item_dt2i_affine(SPItem const *item, SPDesktop *dt);
NRMatrix *sp_item_dt2i_affine(SPItem const *item, SPDesktop *dt, NRMatrix *transform);

int sp_item_repr_compare_position(SPItem *first, SPItem *second);

#endif
