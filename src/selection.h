#ifndef __SP_SELECTION_H__
#define __SP_SELECTION_H__

/*
 * Per-desktop selection container
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   MenTaLguY <mental@rydia.net>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2001-2002 Ximian, Inc.
 * Copyright (C) 2004 MenTaLguY
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <vector>
#include <sigc++/sigc++.h>

#include "libnr/nr-rect.h"
#include "forward.h"
#include "sp-item.h"
#include "gc-managed.h"
#include "gc-finalized.h"
#include "gc-anchored.h"
#include "util/list.h"

#include <list>

namespace Inkscape {
namespace XML {
class Node;
}
}

/**
 * @brief The set of selected SPObjects for a given desktop.
 *
 * This class represents the set of selected SPItems for a given
 * SPDesktop.
 *
 * An SPObject and its parent cannot be simultaneously selected;
 * selecting an SPObjects has the side-effect of unselecting any of
 * its children which might have been selected.
 *
 */
class SPSelection : public Inkscape::GC::Managed<>,
                    public Inkscape::GC::Finalized,
                    public Inkscape::GC::Anchored
{
public:
	/**
	 * Constructs an selection object, bound to a particular
	 * SPDesktop
	 *
	 * @param desktop the desktop in question
	 */
	SPSelection(SPDesktop *desktop);
	~SPSelection();

	/**
	 * @brief Returns the desktop the seoection is bound to
	 *
	 * @return the desktop the selection is bound to
	 */
	SPDesktop *desktop() { return _desktop; }

	/**
	 * @brief Add an SPObject to the set of selected objects
	 *
	 * @param obj the SPObject to add
	 */
	void add(SPObject *obj);

	/**
	 * @brief Add an XML node's SPObject to the set of selected objects
	 *
	 * @param the xml node of the item to add
	 */
	void addRepr(Inkscape::XML::Node *repr) { add(_objectForRepr(repr)); }

	/**
	 * @brief Set the selection to a single specific object
	 *
	 * @param obj the object to select
	 */
	void set(SPObject *obj);

	/**
	 * @brief Set the selection to an XML node's SPObject
	 *
	 * @param repr the xml node of the item to select
	 */
	void setRepr(Inkscape::XML::Node *repr) { set(_objectForRepr(repr)); }

	/**
	 * @brief Removes an item from the set of selected objects
	 *
	 * It is ok to call this method for an unselected item.
	 *
	 * @param item the item to unselect
	 */
	void remove(SPObject *obj);

	/**
	 * @brief Removes an item if selected, adds otherwise
	 *
	 * @param item the item to unselect
	 */
	void toggle(SPObject *obj);

	/**
	 * @brief Removes an item from the set of selected objects
	 *
	 * It is ok to call this method for an unselected item.
	 *
	 * @param repr the xml node of the item to remove
	 */
	void removeRepr(Inkscape::XML::Node *repr) { remove(_objectForRepr(repr)); }

	/**
	 * @brief Selects exactly the specified objects
	 *
	 * @param objs the objects to select
	 */
	void setList(GSList const *objs);

	/**
	 * @brief Adds the specified objects to selection, without deselecting first
	 *
	 * @param objs the objects to select
	 */
	void addList(GSList const *objs);

	/**
	 * @brief Clears the selection and selects the specified objects
	 *
	 * @param repr a list of xml nodes for the items to select
	 */
	void setReprList(GSList const *reprs);

	/** \brief  Add items from an STL list to the selection
		\param  list  The list of items to be added
	*/
	void addStlItemList(std::list<SPItem *> &list);

	/**
	 * @brief Unselects all selected objects.
	 */
	void clear();

	/**
	 * @brief Returns true if no items are selected
	 */
	bool isEmpty() const { return _objs == NULL; }

	/**
	 * @brief Returns true if the given object is selected
	 */
	bool includes(SPObject *obj) const;

	/**
	 * @brief Returns true if the given item is selected
	 */
	bool includesRepr(Inkscape::XML::Node *repr) const {
		return includes(_objectForRepr(repr));
	}

	/**
	 * @brief Returns a single selected object
	 *
	 * @return NULL unless exactly one object is selected
	 */
	SPObject *single();

	/**
	 * @brief Returns a single selected item
	 *
	 * @return NULL unless exactly one object is selected
	 */
	SPItem *singleItem();

	/**
	 * @brief Returns a single selected object's xml node
	 *
	 * @return NULL unless exactly one object is selected
	 */
	Inkscape::XML::Node *singleRepr();

	/** @brief Returns the list of selected objects */
	GSList const *list();
	/** @brief Returns the list of selected SPItems */
	GSList const *itemList();
	/** @brief Appends selected SPItems to list */
	void list(std::list<SPItem *> &l);
	/** @brief Returns a list of the xml nodes of all selected objects */
	// TODO only returns reprs of SPItems currently; need a separate
	//      method for that
	GSList const *reprList();

	/** @brief Returns the number of layers in which there are selected objects */
	guint numberOfLayers();

	/** @brief Returns the bounding rectangle of the selection */
	NRRect *bounds(NRRect *dest) const;
	/** @brief Returns the bounding rectangle of the selection */
	NR::Rect bounds() const;

	/**
	 * @brief Returns the bounding rectangle of the selection
	 *
	 * TODO: how is this different from bounds()?
	 */ 
	NRRect *boundsInDocument(NRRect *dest) const;

	/**
	 * @brief Returns the bounding rectangle of the selection
	 *
	 * TODO: how is this different from bounds()?
	 */
	NR::Rect boundsInDocument() const;

	/**
	 * @brief Gets the selection's snap points.
	 * @return Selection's snap points
	 */
	std::vector<NR::Point> getSnapPoints() const;

	/**
	 * @return A vector containing the top-left and bottom-right
	 * corners of each selected object's bounding box.
	 */
	std::vector<NR::Point> getBBoxPoints() const;

	/**
	 * @brief Connects a slot to be notified of selection changes
	 *
	 * This method connects the given slot such that it will
	 * be called upon any change in the set of selected objects.
	 *
	 * @param slot the slot to connect
	 *
	 * @return the resulting connection
	 */
	sigc::connection connectChanged( const sigc::slot<void, SPSelection *> &slot) {
		return _changed_signal.connect(slot);
	}

	/**
	 * @brief Connects a slot to be notified of selected 
	 *        object modifications 
	 *
	 * This method connects the given slot such that it will
	 * receive notifications whenever any selected item is
	 * modified.
	 *
	 * @param slot the slot to connect
	 *
	 * @return the resulting connection
	 *
	 */
	sigc::connection connectModified(sigc::slot<void, SPSelection *, guint> slot) {
		return _modified_signal.connect(slot);
	}

private:
	/** @brief no copy */
	SPSelection(SPSelection const &);
	/** @brief no assign */
	void operator=(SPSelection const &);

	/** @brief Issues modification notification signals */
	static gboolean _emit_modified(SPSelection *selection);
	/** @brief Schedules an item modification signal to be sent */
	static void _schedule_modified(SPObject *obj, guint flags, SPSelection *selection);
	/** @brief Releases a selected object that is being removed */
	static void _release(SPObject *obj, SPSelection *selection);

	/** @breif Issues modified selection signal */
	void _emitModified(guint flags);
	/** @breif Issues changed selection signal */
	void _emitChanged();

	void _invalidateCachedLists();

	/** @brief unselect all descendants of the given item */
	void _removeObjectDescendants(SPObject *obj);
	/** @brief unselect all ancestors of the given item */
	void _removeObjectAncestors(SPObject *obj);
	/** @brief clears the selection (without issuing a notification) */
	void _clear();
	/** @brief adds an object (without issuing a notification) */
	void _add(SPObject *obj);
	/** @brief removes an object (without issuing a notification) */
	void _remove(SPObject *obj);
	/** @brief returns the SPObject corresponding to an xml node (if any) */
	SPObject *_objectForRepr(Inkscape::XML::Node *repr) const;

	GSList *_objs;
	mutable GSList *_reprs;
	mutable GSList *_items;
	mutable GSList *_item_reprs;
	SPDesktop *_desktop;
	guint _flags;
	guint _idle;

	mutable unsigned _refcount;

	sigc::signal<void, SPSelection *> _changed_signal;
	sigc::signal<void, SPSelection *, guint> _modified_signal;
};

struct SPSelectionClass {
	GObjectClass parent_class;
};

#endif /* !__SP_SELECTION_H__ */
