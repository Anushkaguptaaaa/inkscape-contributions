// SPDX-License-Identifier: GPL-2.0-or-later
/** @file
 * Boolean operations.
 *//*
 * Authors:
 * see git history
 *  Created by fred on Fri Dec 05 2003.
 *  tweaked endlessly by bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 2018 Authors
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include "path-boolop.h"

#include <vector>

#include <glibmm/i18n.h>

#include <2geom/intersection-graph.h>
#include <2geom/svg-path-parser.h> // to get from SVG on boolean to Geom::Path
#include <2geom/utils.h>

#include "document-undo.h"
#include "document.h"
#include "message-stack.h"
#include "path-chemistry.h"     // copy_object_properties()
#include "path-util.h"

#include "display/curve.h"
#include "livarot/Path.h"
#include "livarot/Shape.h"
#include "object/object-set.h"  // This file defines some member functions of ObjectSet.
#include "object/sp-flowtext.h"
#include "object/sp-shape.h"
#include "object/sp-text.h"
#include "ui/icon-names.h"
#include "xml/repr-sorting.h"
#include "style.h"

using Inkscape::DocumentUndo;

/*
 * ObjectSet functions
 */

void Inkscape::ObjectSet::pathUnion(bool skip_undo, bool silent)
{
    _pathBoolOp(bool_op_union, INKSCAPE_ICON("path-union"), _("Union"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathIntersect(bool skip_undo, bool silent)
{
    _pathBoolOp(bool_op_inters, INKSCAPE_ICON("path-intersection"), _("Intersection"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathDiff(bool skip_undo, bool silent)
{
    _pathBoolOp(bool_op_diff, INKSCAPE_ICON("path-difference"), _("Difference"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathSymDiff(bool skip_undo, bool silent)
{
    _pathBoolOp(bool_op_symdiff, INKSCAPE_ICON("path-exclusion"), _("Exclusion"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathCut(bool skip_undo, bool silent)
{
    _pathBoolOp(bool_op_cut, INKSCAPE_ICON("path-division"), _("Division"), skip_undo, silent);
}

void Inkscape::ObjectSet::pathSlice(bool skip_undo, bool silent)
{
    _pathBoolOp(bool_op_slice, INKSCAPE_ICON("path-cut"), _("Cut path"), skip_undo, silent);
}

/*
 * Utilities
 */

/**
 * Create a flattened shape from a path.
 *
 * @param path The path to convert.
 * @param path_id The id to assign to all the edges in the resultant shape.
 * @param fill_rule The fill rule with which to flatten the path.
 * @param close_if_needed If the path is not closed, whether to add a closing segment.
 */
static Shape make_shape(Path &path, int path_id = -1, FillRule fill_rule = fill_nonZero, bool close_if_needed = true)
{
    Shape result;

    Shape tmp;
    path.Fill(&tmp, path_id, false, close_if_needed);
    result.ConvertToShape(&tmp, fill_rule);

    return result;
}

constexpr auto RELATIVE_THRESHOLD = 0.08;

/**
 * Create a path with backdata from a pathvector,
 * automatically estimating a suitable conversion threshold.
 */
static Path make_path(Geom::PathVector const &pathv, std::vector<Geom::PathVectorTime> const &cuts)
{
    Path result;

    result.LoadPathVector(pathv, cuts);
    result.ConvertWithBackData(RELATIVE_THRESHOLD, true);

    return result;
}

/**
 * Return whether a path is a single open line segment.
 */
static bool is_line(Path const &path)
{
    return path.pts.size() == 2 && path.pts[0].isMoveTo && !path.pts[1].isMoveTo;
}

static inline void distribute_intersection_times(std::vector<Geom::PathVectorTime> &dst1, std::vector<Geom::PathVectorTime> &dst2, std::vector<Geom::PathVectorIntersection> const &intersections)
{
    auto filter_and_add = [] (auto const &x, auto &dst) {
        if (x.t > Geom::EPSILON && x.t < 1.0 - Geom::EPSILON) {
            dst.emplace_back(x);
        }
    };

    for (auto const &x : intersections) {
        filter_and_add(x.first, dst1);
        filter_and_add(x.second, dst2);
    }
}

static void sort_and_clean_intersection_times(std::vector<Geom::PathVectorTime> &vec)
{
    std::sort(begin(vec), end(vec));

    auto prev = Geom::PathVectorTime{0, 0, 0.0};
    for (auto it = vec.begin(); it != vec.end(); ) {
        if (it->path_index == prev.path_index && it->curve_index == prev.curve_index && it->t < prev.t + Geom::EPSILON) {
            it = vec.erase(it);
        } else {
            prev = *it;
            ++it;
        }
    }
}

/*
 * Flattening
 */

Geom::PathVector flattened(Geom::PathVector const &pathv, FillRule fill_rule)
{
    std::vector<Geom::PathVectorTime> times;
    distribute_intersection_times(times, times, pathv.intersectSelf());
    sort_and_clean_intersection_times(times);

    auto path = make_path(pathv, times);
    auto shape = make_shape(path, 0, fill_rule);

    Path res;
    shape.ConvertToForme(&res, 1, std::begin({ &path }));

    return res.MakePathVector();
}

void flatten(Geom::PathVector &pathv, FillRule fill_rule)
{
    pathv = flattened(pathv, fill_rule);
}

/*
 * Boolean operations on pathvectors
 */

std::vector<Geom::PathVector> pathvector_cut(Geom::PathVector const &pathv, Geom::PathVector const &lines)
{
    std::vector<Geom::PathVectorTime> timesa, timesb;
    distribute_intersection_times(timesa, timesa, pathv.intersectSelf());
    distribute_intersection_times(timesb, timesb, lines.intersectSelf());
    distribute_intersection_times(timesa, timesb, pathv.intersect(lines));
    sort_and_clean_intersection_times(timesa);
    sort_and_clean_intersection_times(timesb);

    auto patha = make_path(pathv, timesa);
    auto pathb = make_path(lines, timesb);
    auto shapea = make_shape(patha, 0);
    auto shapeb = make_shape(pathb, 1, fill_justDont, is_line(pathb));

    Shape shape;
    shape.Booleen(&shapeb, &shapea, bool_op_cut, 1);

    Path path;
    int num_nesting = 0;
    int *nesting = nullptr;
    int *conts = nullptr;
    shape.ConvertToFormeNested(&path, 2, std::begin({ &patha, &pathb }), num_nesting, nesting, conts, true);

    int num_paths;
    auto paths = path.SubPathsWithNesting(num_paths, false, num_nesting, nesting, conts);

    std::vector<Geom::PathVector> result;
    result.reserve(num_paths);

    for (int i = 0; i < num_paths; i++) {
        result.emplace_back(paths[i]->MakePathVector());
        delete paths[i];
    }

    g_free(paths);
    g_free(conts);
    g_free(nesting);

    return result;
}

Geom::PathVector sp_pathvector_boolop(Geom::PathVector const &pathva, Geom::PathVector const &pathvb, BooleanOp bop, FillRule fra, FillRule frb)
{
    std::vector<Geom::PathVectorTime> timesa, timesb;
    distribute_intersection_times(timesa, timesa, pathva.intersectSelf());
    distribute_intersection_times(timesb, timesb, pathvb.intersectSelf());
    distribute_intersection_times(timesa, timesb, pathva.intersect(pathvb));
    sort_and_clean_intersection_times(timesa);
    sort_and_clean_intersection_times(timesb);

    auto patha = make_path(pathva, timesa);
    auto pathb = make_path(pathvb, timesb);

    Path result;

    if (bop == bool_op_inters || bop == bool_op_union || bop == bool_op_diff || bop == bool_op_symdiff) {
        // true boolean op
        // get the polygons of each path, with the winding rule specified, and apply the operation iteratively
        auto shapea = make_shape(patha, 0, fra);
        auto shapeb = make_shape(pathb, 1, frb);

        Shape shape;
        shape.Booleen(&shapeb, &shapea, bop);

        shape.ConvertToForme(&result, 2, std::begin({ &patha, &pathb }));

    } else if (bop == bool_op_cut) {
        // cuts= sort of a bastard boolean operation, thus not the axact same modus operandi
        // technically, the cut path is not necessarily a polygon (thus has no winding rule)
        // it is just uncrossed, and cleaned from duplicate edges and points
        // then it's fed to Booleen() which will uncross it against the other path
        // then comes the trick: each edge of the cut path is duplicated (one in each direction),
        // thus making a polygon. the weight of the edges of the cut are all 0, but
        // the Booleen need to invert the ones inside the source polygon (for the subsequent
        // ConvertToForme)

        // the cut path needs to have the highest pathID in the back data
        // that's how the Booleen() function knows it's an edge of the cut
        // fill_justDont doesn't compute winding numbers
        // see LP Bug 177956 for why is_line is needed
        auto shapea = make_shape(patha, 1, fill_justDont, is_line(patha));
        auto shapeb = make_shape(pathb, 0, frb);

        Shape shape;
        shape.Booleen(&shapea, &shapeb, bool_op_cut, 1);

        shape.ConvertToForme(&result, 2, std::begin({ &pathb, &patha }), true);

    } else if (bop == bool_op_slice) {
        // slice is not really a boolean operation
        // you just put the 2 shapes in a single polygon, uncross it
        // the points where the degree is > 2 are intersections
        // just check it's an intersection on the path you want to cut, and keep it
        // the intersections you have found are then fed to ConvertPositionsToMoveTo() which will
        // make new subpath at each one of these positions
        // inversion pour l'opération

        Shape tmp;
        pathb.Fill(&tmp, 0, false, false, false); // don't closeIfNeeded
        patha.Fill(&tmp, 1, true, false, false); // don't closeIfNeeded and just dump in the shape, don't reset it

        Shape shape;
        shape.ConvertToShape(&tmp, fill_justDont);

        std::vector<Path::cut_position> toCut;

        assert(shape.hasBackData());

        for (int i = 0; i < shape.numberOfPoints(); i++) {
            if (shape.getPoint(i).totalDegree() > 2) {
                // possibly an intersection
                // we need to check that at least one edge from the source path is incident to it
                // before we declare it's an intersection
                int nbOrig = 0;
                int nbOther = 0;
                int piece = -1;
                double t = 0.0;

                int cb = shape.getPoint(i).incidentEdge[FIRST];
                while (cb >= 0 && cb < shape.numberOfEdges()) {
                    if (shape.ebData[cb].pathID == 0) {
                        // the source has an edge incident to the point, get its position on the path
                        piece = shape.ebData[cb].pieceID;
                        t = shape.getEdge(cb).st == i ? shape.ebData[cb].tSt : shape.ebData[cb].tEn;
                        nbOrig++;
                    }
                    if (shape.ebData[cb].pathID == 1) {
                        nbOther++; // the cut is incident to this point
                    }
                    cb = shape.NextAt(i, cb);
                }

                if (nbOrig > 0 && nbOther > 0) {
                    // point incident to both path and cut: an intersection
                    // note that you only keep one position on the source; you could have degenerate
                    // cases where the source crosses itself at this point, and you wouyld miss an intersection
                    toCut.push_back({ .piece = piece, .t = t });
                }
            }
        }

        // I think it's useless now
        for (int i = shape.numberOfEdges() - 1; i >= 0; i--) {
            if (shape.ebData[i].pathID == 1) {
                shape.SubEdge(i);
            }
        }

        result.Copy(&pathb);
        result.ConvertPositionsToMoveTo(toCut.size(), toCut.data()); // cut where you found intersections
    }

    return result.MakePathVector();
}

void Inkscape::ObjectSet::_pathBoolOp(BooleanOp bop, char const *icon_name, char const *description, bool skip_undo, bool silent)
{
    try {
        ObjectSet::_pathBoolOp(bop);
        if (!skip_undo) {
            DocumentUndo::done(document(), description, icon_name);
        }
    } catch (char const *msg) {
        if (!silent) {
            if (desktop()) {
                desktop()->messageStack()->flash(ERROR_MESSAGE, msg);
            } else {
                g_printerr("%s\n", msg);
            }
        }
    }
}

void Inkscape::ObjectSet::_pathBoolOp(BooleanOp bop)
{
    auto const doc = document();

    // Grab the items list.
    auto const il = std::vector<SPItem*>(items().begin(), items().end());

    // Validate number of items.
    switch (bop) {
        case bool_op_union:
            if (il.size() < 1) { // Allow union of single item --> flatten.
                throw _("Select <b>at least 1 path</b> to perform a boolean union.");
            }
            break;
        case bool_op_inters:
        case bool_op_symdiff:
            if (il.size() < 2) {
                throw _("Select <b>at least 2 paths</b> to perform an intersection or symmetric difference.");
            }
            break;
        case bool_op_diff:
        case bool_op_cut:
        case bool_op_slice:
            if (il.size() != 2) {
                throw _("Select <b>exactly 2 paths</b> to perform difference, division, or path cut.");
            }
            break;
    }
    assert(!il.empty());

    // reverseOrderForOp marks whether the order of the list is the top->down order
    // it's only used when there are 2 objects, and for operations which need to know the
    // topmost object (differences, cuts)
    bool reverseOrderForOp = false;

    if (bop == bool_op_diff || bop == bool_op_cut || bop == bool_op_slice) {
        // check in the tree to find which element of the selection list is topmost (for 2-operand commands only)
        Inkscape::XML::Node *a = il.front()->getRepr();
        Inkscape::XML::Node *b = il.back()->getRepr();

        if (!a || !b) {
            return;
        }

        if (is_descendant_of(a, b)) {
            // a is a child of b, already in the proper order
        } else if (is_descendant_of(b, a)) {
            // reverse order
            reverseOrderForOp = true;
        } else {

            // objects are not in parent/child relationship;
            // find their lowest common ancestor
            Inkscape::XML::Node *parent = lowest_common_ancestor(a, b);
            if (!parent) {
                return;
            }

            // find the children of the LCA that lead from it to the a and b
            Inkscape::XML::Node *as = find_containing_child(a, parent);
            Inkscape::XML::Node *bs = find_containing_child(b, parent);

            // find out which comes first
            for (Inkscape::XML::Node *child = parent->firstChild(); child; child = child->next()) {
                if (child == as) {
                    /* a first, so reverse. */
                    reverseOrderForOp = true;
                    break;
                }
                if (child == bs)
                    break;
            }
        }
    }

    // first check if all the input objects have shapes
    // otherwise bail out
    for (auto item : il) {
        if (!is<SPShape>(item) && !is<SPText>(item) && !is<SPFlowtext>(item)) {
            return;
        }
    }

    struct Operand
    {
        // From source objects
        FillRule fill_rule{};
        Geom::PathVector pathv;

        // Computed
        std::vector<Geom::PathVectorTime> cuts;
        std::unique_ptr<Path> path;
    };

    std::vector<Operand> operands;
    operands.resize(il.size());

    // Extract the fill rules and pathvectors from the source objects.
    for (int i = 0; i < il.size(); i++) {
        auto item = il[i];
        auto &operand = operands[i];

        // apply live path effects prior to performing boolean operation
        char const *id = item->getAttribute("id");
        if (auto lpeitem = cast<SPLPEItem>(item)) {
            SPDocument * document = item->document;
            lpeitem->removeAllPathEffects(true);
            SPObject *elemref = document->getObjectById(id);
            if (elemref && elemref != item) {
                // If the LPE item is a shape, it is converted to a path
                // so we need to reupdate the item
                item = cast<SPItem>(elemref);
            }
        }

        // Get the fill rule.
        if (item->style->fill_rule.computed == SP_WIND_RULE_EVENODD) {
            operand.fill_rule = fill_oddEven;
        } else {
            operand.fill_rule = fill_nonZero;
        }

        // Get the pathvector.
        auto curve = curve_for_item(item);
        if (!curve) {
            return;
        }

        operand.pathv = curve->get_pathvector() * item->i2doc_affine();
    }

    // Compute the intersections and self-intersections, and use this information when converting to livarot paths.
    for (int i = 0; i < operands.size(); i++) {
        for (int j = 0; j < i; j++) {
            distribute_intersection_times(operands[i].cuts, operands[j].cuts, operands[i].pathv.intersect(operands[j].pathv));
        }
    }

    for (auto &operand : operands) {
        distribute_intersection_times(operand.cuts, operand.cuts, operand.pathv.intersectSelf());
        sort_and_clean_intersection_times(operand.cuts);

        operand.path = std::make_unique<Path>();
        operand.path->LoadPathVector(operand.pathv, operand.cuts);
        operand.path->ConvertWithBackData(RELATIVE_THRESHOLD, true);

        if (operand.path->descr_cmd.size() <= 1) {
            return;
        }
    }

    // reverse if needed
    // note that the selection list keeps its order
    if (reverseOrderForOp) {
        std::swap(operands[0], operands[1]);
    }

    // and work
    // some temporary instances, first
    Shape *theShapeA = new Shape;
    Shape *theShapeB = new Shape;
    Shape *theShape = new Shape;
    Path *res = new Path;
    Path::cut_position  *toCut=nullptr;
    int                  nbToCut=0;

    if (bop == bool_op_inters || bop == bool_op_union || bop == bool_op_diff || bop == bool_op_symdiff) {
        // true boolean op
        // get the polygons of each path, with the winding rule specified, and apply the operation iteratively

        operands[0].path->Fill(theShape, 0);

        theShapeA->ConvertToShape(theShape, operands[0].fill_rule);

        for (int i = 1; i < operands.size(); i++) {
            operands[i].path->Fill(theShape, i);

            theShapeB->ConvertToShape(theShape, operands[i].fill_rule);

            /* Due to quantization of the input shape coordinates, we may end up with A or B being empty.
             * If this is a union or symdiff operation, we just use the non-empty shape as the result:
             *   A=0  =>  (0 or B) == B
             *   B=0  =>  (A or 0) == A
             *   A=0  =>  (0 xor B) == B
             *   B=0  =>  (A xor 0) == A
             * If this is an intersection operation, we just use the empty shape as the result:
             *   A=0  =>  (0 and B) == 0 == A
             *   B=0  =>  (A and 0) == 0 == B
             * If this a difference operation, and the upper shape (A) is empty, we keep B.
             * If the lower shape (B) is empty, we still keep B, as it's empty:
             *   A=0  =>  (B - 0) == B
             *   B=0  =>  (0 - A) == 0 == B
             *
             * In any case, the output from this operation is stored in shape A, so we may apply
             * the above rules simply by judicious use of swapping A and B where necessary.
             */
            bool zeroA = theShapeA->numberOfEdges() == 0;
            bool zeroB = theShapeB->numberOfEdges() == 0;
            if (zeroA || zeroB) {
                // We might need to do a swap. Apply the above rules depending on operation type.
                bool resultIsB =   ((bop == bool_op_union || bop == bool_op_symdiff) && zeroA)
                                   || ((bop == bool_op_inters) && zeroB)
                                   ||  (bop == bool_op_diff);
                if (resultIsB) {
                    // Swap A and B to use B as the result
                    std::swap(theShapeA, theShapeB);
                }
            } else {
                // Just do the Boolean operation as usual
                // les elements arrivent en ordre inverse dans la liste
                theShape->Booleen(theShapeB, theShapeA, bop);
                std::swap(theShape, theShapeA);
            }
        }

        std::swap(theShape, theShapeA);

    } else if (bop == bool_op_cut) {
        // cuts= sort of a bastard boolean operation, thus not the axact same modus operandi
        // technically, the cut path is not necessarily a polygon (thus has no winding rule)
        // it is just uncrossed, and cleaned from duplicate edges and points
        // then it's fed to Booleen() which will uncross it against the other path
        // then comes the trick: each edge of the cut path is duplicated (one in each direction),
        // thus making a polygon. the weight of the edges of the cut are all 0, but
        // the Booleen need to invert the ones inside the source polygon (for the subsequent
        // ConvertToForme)

        // the cut path needs to have the highest pathID in the back data
        // that's how the Booleen() function knows it's an edge of the cut
        std::swap(operands[0], operands[1]);

        operands[0].path->Fill(theShape, 0);

        theShapeA->ConvertToShape(theShape, operands[0].fill_rule);

        operands[1].path->Fill(theShape, 1, false, is_line(*operands[1].path), false);

        theShapeB->ConvertToShape(theShape, fill_justDont);

        // les elements arrivent en ordre inverse dans la liste
        theShape->Booleen(theShapeB, theShapeA, bool_op_cut, 1);

    } else if (bop == bool_op_slice) {
        // slice is not really a boolean operation
        // you just put the 2 shapes in a single polygon, uncross it
        // the points where the degree is > 2 are intersections
        // just check it's an intersection on the path you want to cut, and keep it
        // the intersections you have found are then fed to ConvertPositionsToMoveTo() which will
        // make new subpath at each one of these positions
        // inversion pour l'opération
        std::swap(operands[0], operands[1]);

        operands[0].path->Fill(theShapeA, 0, false, false, false); // don't closeIfNeeded

        operands[1].path->Fill(theShapeA, 1, true, false, false); // don't closeIfNeeded and just dump in the shape, don't reset it

        theShape->ConvertToShape(theShapeA, fill_justDont);

        if (theShape->hasBackData()) {
            // should always be the case, but ya never know
            {
                for (int i = 0; i < theShape->numberOfPoints(); i++) {
                    if ( theShape->getPoint(i).totalDegree() > 2 ) {
                        // possibly an intersection
                        // we need to check that at least one edge from the source path is incident to it
                        // before we declare it's an intersection
                        int cb = theShape->getPoint(i).incidentEdge[FIRST];
                        int   nbOrig=0;
                        int   nbOther=0;
                        int   piece=-1;
                        float t=0.0;
                        while ( cb >= 0 && cb < theShape->numberOfEdges() ) {
                            if ( theShape->ebData[cb].pathID == 0 ) {
                                // the source has an edge incident to the point, get its position on the path
                                piece=theShape->ebData[cb].pieceID;
                                if ( theShape->getEdge(cb).st == i ) {
                                    t=theShape->ebData[cb].tSt;
                                } else {
                                    t=theShape->ebData[cb].tEn;
                                }
                                nbOrig++;
                            }
                            if ( theShape->ebData[cb].pathID == 1 ) nbOther++; // the cut is incident to this point
                            cb=theShape->NextAt(i, cb);
                        }
                        if ( nbOrig > 0 && nbOther > 0 ) {
                            // point incident to both path and cut: an intersection
                            // note that you only keep one position on the source; you could have degenerate
                            // cases where the source crosses itself at this point, and you wouyld miss an intersection
                            toCut=(Path::cut_position*)realloc(toCut, (nbToCut+1)*sizeof(Path::cut_position));
                            toCut[nbToCut].piece=piece;
                            toCut[nbToCut].t=t;
                            nbToCut++;
                        }
                    }
                }
            }
            {
                // i think it's useless now
                int i = theShape->numberOfEdges() - 1;
                for (;i>=0;i--) {
                    if ( theShape->ebData[i].pathID == 1 ) {
                        theShape->SubEdge(i);
                    }
                }
            }

        }
    }

    auto get_path_arr = [&] {
        std::vector<Path *> result;
        result.reserve(operands.size());
        for (auto &operand : operands) {
            result.emplace_back(operand.path.get());
        }
        return result;
    };

    int*    nesting=nullptr;
    int*    conts=nullptr;
    int     nbNest=0;
    // pour compenser le swap juste avant
    if (bop == bool_op_slice) {
        res->Copy(operands[0].path.get());
        res->ConvertPositionsToMoveTo(nbToCut, toCut); // cut where you found intersections
        free(toCut);
    } else if (bop == bool_op_cut) {
        // il faut appeler pour desallouer PointData (pas vital, mais bon)
        // the Booleen() function did not deallocate the point_data array in theShape, because this
        // function needs it.
        // this function uses the point_data to get the winding number of each path (ie: is a hole or not)
        // for later reconstruction in objects, you also need to extract which path is parent of holes (nesting info)
        theShape->ConvertToFormeNested(res, operands.size(), get_path_arr().data(), nbNest, nesting, conts, true);
    } else {
        theShape->ConvertToForme(res, operands.size(), get_path_arr().data());
    }

    delete theShape;
    delete theShapeA;
    delete theShapeB;

    if (res->descr_cmd.size() <= 1) {
        // only one command, presumably a moveto: it isn't a path
        for (auto l : il){
            l->deleteObject();
        }
        clear();

        delete res;
        return;
    }

    // get the source path object
    SPObject *source;
    if ( bop == bool_op_diff || bop == bool_op_cut || bop == bool_op_slice ) {
        if (reverseOrderForOp) {
            source = il[0];
        } else {
            source = il.back();
        }
    } else {
        // find out the bottom object
        std::vector<Inkscape::XML::Node*> sorted(xmlNodes().begin(), xmlNodes().end());

        sort(sorted.begin(),sorted.end(),sp_repr_compare_position_bool);

        source = doc->getObjectByRepr(sorted.front());
    }

    // adjust style properties that depend on a possible transform in the source object in order
    // to get a correct style attribute for the new path
    auto item_source = cast<SPItem>(source);
    Geom::Affine i2doc(item_source->i2doc_affine());

    Inkscape::XML::Node *repr_source = source->getRepr();

    // remember important aspects of the source path, to be restored
    gint pos = repr_source->position();
    Inkscape::XML::Node *parent = repr_source->parent();
    // remove source paths
    clear();
    for (auto l : il){
        if (l != item_source) {
            // delete the object for real, so that its clones can take appropriate action
            l->deleteObject();
        }
    }

    auto const source2doc_inverse = i2doc.inverse();
    char const *const old_transform_attibute = repr_source->attribute("transform");

    // now that we have the result, add it on the canvas
    if ( bop == bool_op_cut || bop == bool_op_slice ) {
        int    nbRP=0;
        Path** resPath;
        if ( bop == bool_op_slice ) {
            // there are moveto's at each intersection, but it's still one unique path
            // so break it down and add each subpath independently
            // we could call break_apart to do this, but while we have the description...
            resPath=res->SubPaths(nbRP, false);
        } else {
            // cut operation is a bit wicked: you need to keep holes
            // that's why you needed the nesting
            // ConvertToFormeNested() dumped all the subpath in a single Path "res", so we need
            // to get the path for each part of the polygon. that's why you need the nesting info:
            // to know in which subpath to add a subpath
            resPath=res->SubPathsWithNesting(nbRP, true, nbNest, nesting, conts);

            // cleaning
            if ( conts ) free(conts);
            if ( nesting ) free(nesting);
        }

        // add all the pieces resulting from cut or slice
        std::vector <Inkscape::XML::Node*> selection;
        for (int i=0;i<nbRP;i++) {
            resPath[i]->Transform(source2doc_inverse);

            Inkscape::XML::Document *xml_doc = doc->getReprDoc();
            Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

            Inkscape::copy_object_properties(repr, repr_source);

            // Delete source on last iteration (after we don't need repr_source anymore). As a consequence, the last
            // item will inherit the original's id.
            if (i + 1 == nbRP) {
                item_source->deleteObject(false);
            }

            repr->setAttribute("d", resPath[i]->svg_dump_path().c_str());

            // for slice, remove fill
            if (bop == bool_op_slice) {
                SPCSSAttr *css;

                css = sp_repr_css_attr_new();
                sp_repr_css_set_property(css, "fill", "none");

                sp_repr_css_change(repr, css, "style");

                sp_repr_css_attr_unref(css);
            }

            repr->setAttributeOrRemoveIfEmpty("transform", old_transform_attibute);

            // add the new repr to the parent
            // move to the saved position
            parent->addChildAtPos(repr, pos);

            selection.push_back(repr);
            Inkscape::GC::release(repr);

            delete resPath[i];
        }
        setReprList(selection);
        if ( resPath ) free(resPath);

    } else {
        res->Transform(source2doc_inverse);

        Inkscape::XML::Document *xml_doc = doc->getReprDoc();
        Inkscape::XML::Node *repr = xml_doc->createElement("svg:path");

        Inkscape::copy_object_properties(repr, repr_source);

        // delete it so that its clones don't get alerted; this object will be restored shortly, with the same id
        item_source->deleteObject(false);

        repr->setAttribute("d", res->svg_dump_path().c_str());

        repr->setAttributeOrRemoveIfEmpty("transform", old_transform_attibute);

        parent->addChildAtPos(repr, pos);

        set(repr);
        Inkscape::GC::release(repr);
    }

    delete res;
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
