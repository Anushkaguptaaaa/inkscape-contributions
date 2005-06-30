#ifndef __SP_DESKTOP_AFFINE_H__
#define __SP_DESKTOP_AFFINE_H__

/*
 * Desktop transformations
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "forward.h"
#include <libnr/nr-forward.h>

NR::Matrix const sp_desktop_w2dt_affine(SPDesktop const *desktop);
NR::Matrix const sp_desktop_dt2w_affine(SPDesktop const *desktop);

NR::Matrix const sp_desktop_dt2doc_affine(SPDesktop const *desktop);
NR::Matrix const sp_desktop_doc2dt_affine(SPDesktop const *desktop);

NR::Matrix const sp_desktop_w2doc_affine(SPDesktop const *desktop);
NR::Matrix const sp_desktop_doc2w_affine(SPDesktop const *desktop);

NR::Matrix const sp_desktop_root2dt_affine(SPDesktop const *dt);
NR::Matrix const sp_desktop_dt2root_affine(SPDesktop const *dt);

NR::Point sp_desktop_w2d_xy_point(SPDesktop const *desktop, const NR::Point p);
NR::Point sp_desktop_d2w_xy_point(SPDesktop const *desktop, const NR::Point p);
NR::Point sp_desktop_w2doc_xy_point(SPDesktop const *desktop, const NR::Point p);
NR::Point sp_desktop_doc2w_xy_point(SPDesktop const *desktop, const NR::Point p);
NR::Point sp_desktop_d2doc_xy_point(SPDesktop const *desktop, const NR::Point p);
NR::Point sp_desktop_doc2d_xy_point(SPDesktop const *desktop, const NR::Point p);

NR::Point sp_desktop_root2dt_xy_point(SPDesktop const *dt, const NR::Point p);
NR::Point sp_desktop_dt2root_xy_point(SPDesktop const *dt, const NR::Point p);

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
