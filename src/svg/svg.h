#ifndef __SP_SVG_H__
#define __SP_SVG_H__

/*
 * SVG data parser
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#include <glib.h>
#include <libnr/nr-forward.h>
#include <svg/svg-types.h>
#include <string>


/* Generic */

/*
 * These are very-very simple:
 * - they accept everything libc strtod accepts
 * - no valid end character checking
 * Return FALSE and let val untouched on error
 */
 
unsigned int sp_svg_number_read_f (const gchar *str, float *val);
unsigned int sp_svg_number_read_d (const gchar *str, double *val);

/*
 * No buffer overflow checking is done, so better wrap them if needed
 */
unsigned int sp_svg_number_write_de (gchar *buf, double val, unsigned int tprec, unsigned int padf);

/* Length */

/*
 * Parse number with optional unit specifier:
 * - for px, pt, pc, mm, cm, computed is final value accrding to SVG spec
 * - for em, ex, and % computed is left untouched
 * - % is divided by 100 (i.e. 100% is 1.0)
 * !isalnum check is done at the end
 * Any return value pointer can be NULL
 */

unsigned int sp_svg_length_read (const gchar *str, SPSVGLength *length);
unsigned int sp_svg_length_read_absolute (const gchar *str, SPSVGLength *length);
unsigned int sp_svg_length_read_computed_absolute (const gchar *str, float *length);
GList *sp_svg_length_list_read (const gchar *str);
unsigned int sp_svg_length_read_ldd (const gchar *str, SPSVGLengthUnit *unit, double *value, double *computed);

void sp_svg_length_set (SPSVGLength *length, SPSVGLengthUnit unit, float value, float computed);
void sp_svg_length_unset (SPSVGLength *length, SPSVGLengthUnit unit, float value, float computed);
void sp_svg_length_update (SPSVGLength *length, double em, double ex, double scale);

std::string sp_svg_length_write_with_units(SPSVGLength const &length);

bool sp_svg_transform_read(gchar const *str, NR::Matrix *transform);

unsigned sp_svg_transform_write(gchar str[], unsigned size, NR::Matrix const &transform);
unsigned sp_svg_transform_write(gchar str[], unsigned size, NRMatrix const *transform);

double sp_svg_read_percentage (const char * str, double def);

unsigned int sp_svg_read_color (const gchar * str, unsigned int def);
int sp_svg_write_color (char * buf, int buflen, unsigned int color);

/* NB! As paths can be long, we use here dynamic string */

#include <libnr/nr-path.h>

NArtBpath * sp_svg_read_path (const char * str);
char * sp_svg_write_path (const NArtBpath * bpath);


#endif

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
