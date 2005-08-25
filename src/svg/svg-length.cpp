#define __SP_SVG_LENGTH_C__

/*
 * SVG data parser
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *
 * Copyright (C) 1999-2002 Lauris Kaplinski
 *
 * This code is in public domain
 */
#include "config.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "svg.h"
#include "stringstream.h"
#include "../unit-constants.h"

#include <glib.h>

static unsigned sp_svg_length_read_lff(gchar const *str, SPSVGLengthUnit *unit, float *val, float *computed, char **next);

#ifndef MAX
#define MAX(a,b) ((a < b) ? (b) : (a))
#endif

unsigned int
sp_svg_number_read_f (const gchar *str, float *val)
{
	char *e;
	float v;

	if (!str) return 0;
	v = g_ascii_strtod (str, &e);
	if ((const gchar *) e == str) return 0;
	*val = v;
	return 1;
}

unsigned int
sp_svg_number_read_d (const gchar *str, double *val)
{
	char *e;
	double v;

	if (!str) return 0;
	v = g_ascii_strtod (str, &e);
	if ((const gchar *) e == str) return 0;
	*val = v;
	return 1;
}

static unsigned
sp_svg_number_write_i (gchar *buf, int val)
{
	char c[32];
	int p, i;
	p = 0;
	if (val < 0) {
		buf[p++] = '-';
		val = -val;
	}
	i = 0;
	do {
		c[32 - (++i)] = '0' + (val % 10);
		val /= 10;
	} while (val > 0);
	memcpy (buf + p, &c[32 - i], i);
	p += i;
	buf[p] = 0;
	return p;
}

static unsigned
sp_svg_number_write_d (gchar *buf, double val, unsigned int tprec, unsigned int fprec, unsigned int padf)
{
	double dival, fval;
	int idigits, ival, i;
	i = 0;
	/* Process sign */
	if (val < 0.0) {
		buf[i++] = '-';
		val = fabs (val);
	}
	/* Determine number of integral digits */
	if (val >= 1.0) {
		idigits = (int) floor (log10 (val));
	} else {
		idigits = 0;
	}
	/* Determine the actual number of fractional digits */
	fprec = MAX (fprec, tprec - idigits);
	/* Round value */
	val += 0.5 * pow (10.0, - ((double) fprec));
	/* Extract integral and fractional parts */
	dival = floor (val);
	ival = (int) dival;
	fval = val - dival;
	/* Write integra */
	i += sp_svg_number_write_i (buf + i, ival);
	if ((fprec > 0) && (padf || (fval > 0.0))) {
		buf[i++] = '.';
		while ((fprec > 0) && (padf || (fval > 0.0))) {
			fval *= 10.0;
			dival = floor (fval);
			fval -= dival;
			buf[i++] = '0' + (int) dival;
			fprec -= 1;
		}

	}
	buf[i] = 0;
	return i;
}

unsigned int
sp_svg_number_write_de (gchar *buf, double val, unsigned int tprec, unsigned int padf)
{
	if ((val == 0.0) || ((fabs (val) >= 0.1) && (fabs(val) < 10000000))) {
		return sp_svg_number_write_d (buf, val, tprec, 0, padf);
	} else {
		double eval;
		int p;
		eval = floor (log10 (fabs (val)));
		val = val / pow (10.0, eval);
		p = sp_svg_number_write_d (buf, val, tprec, 0, padf);
		buf[p++] = 'e';
		p += sp_svg_number_write_i (buf + p, (int) eval);
		return p;
	}
}

/* Length */

unsigned int
sp_svg_length_read (const gchar *str, SPSVGLength *length)
{
	SPSVGLengthUnit unit;
	float value, computed;

	if (!str) return 0;

	if (!sp_svg_length_read_lff (str, &unit, &value, &computed, NULL)) 
		return 0;

	length->set = 1;
	length->unit = unit;
	length->value = value;
	length->computed = computed;

	return 1;
}

unsigned int
sp_svg_length_read_absolute (const gchar *str, SPSVGLength *length)
{
	SPSVGLengthUnit unit;
	float value, computed;

	if (!str) return 0;

	if (!sp_svg_length_read_lff (str, &unit, &value, &computed, NULL)) 
		return 0;

	if ((unit == SP_SVG_UNIT_EM) || (unit == SP_SVG_UNIT_EX) || (unit == SP_SVG_UNIT_PERCENT))
		//not an absolute unit
		return 0;

	length->set = 1;
	length->unit = unit;
	length->value = value;
	length->computed = computed;

	return 1;
}


unsigned int
sp_svg_length_read_computed_absolute (const gchar *str, float *length)
{
	SPSVGLengthUnit unit;
	float computed;

	if (!str) return 0;

	if (!sp_svg_length_read_lff (str, &unit, NULL, &computed, NULL)) 
		// failed to read
		return 0;

	if ((unit == SP_SVG_UNIT_EM) || (unit == SP_SVG_UNIT_EX) || (unit == SP_SVG_UNIT_PERCENT))
		//not an absolute unit
		return 0;

	*length = computed;

	return 1;
}

GList *
sp_svg_length_list_read (const gchar *str)
{
	SPSVGLengthUnit unit;
	float value, computed;
	char *next = (char *) str;
	GList *list = NULL;

	if (!str) return NULL;

	while (sp_svg_length_read_lff (next, &unit, &value, &computed, &next)) {

		SPSVGLength *length = g_new (SPSVGLength, 1);

		length->set = 1;
		length->unit = unit;
		length->value = value;
		length->computed = computed;

		list = g_list_append (list, length);

		while (next && *next && (*next == ',' || *next == ' ' || *next == '\n' || *next == '\r' || *next == '\t')) 
			next++; // the list can be comma- or space-separated, but we will be generous and accept a mix, including newlines and tabs

		if (!next || !*next) break;
	}

	return list;
}


#define UVAL(a,b) (((unsigned int) (a) << 8) | (unsigned int) (b))

static unsigned
sp_svg_length_read_lff (const gchar *str, SPSVGLengthUnit *unit, float *val, float *computed, char **next)
{
	const gchar *e;
	float v;

	if (!str) return 0;
	v = g_ascii_strtod (str, (char **) &e);
	if (e == str) {
		return 0;
	}
	if (!e[0]) {
		/* Unitless */
		if (unit) *unit = SP_SVG_UNIT_NONE;
		if (val) *val = v;
		if (computed) *computed = v;
		if (next) *next = NULL; // no more values
		return 1;
	} else if (!g_ascii_isalnum (e[0])) {
		/* Unitless or percent */
		if (e[0] == '%') {
			/* Percent */
			if (e[1] && g_ascii_isalnum (e[1])) return 0;
			if (unit) *unit = SP_SVG_UNIT_PERCENT;
			if (val) *val = v * 0.01;
			if (computed) *computed = v * 0.01;
			if (next) *next = (char *) e + 1; 
			return 1;
		} else {
			/* Unitless */
			if (unit) *unit = SP_SVG_UNIT_NONE;
			if (val) *val = v;
			if (computed) *computed = v;
			if (next) *next = (char *) e; 
			return 1;
		}
	} else if (e[1] && !g_ascii_isalnum (e[2])) {
		/* TODO: Allow the number of px per inch to vary (document preferences, X server
		 * or whatever).  E.g. don't fill in computed here, do it at the same time as
		 * percentage units are done. */
		unsigned int uval;
		/* Units */
		uval = UVAL (e[0], e[1]);
		switch (uval) {
		case UVAL('p','x'):
			if (unit) *unit = SP_SVG_UNIT_PX;
			if (computed) *computed = v;
			break;
		case UVAL('p','t'):
			if (unit) *unit = SP_SVG_UNIT_PT;
			if (computed) *computed = v * PX_PER_PT;
			break;
		case UVAL('p','c'):
			if (unit) *unit = SP_SVG_UNIT_PC;
			if (computed) *computed = v * 12 * PX_PER_PT;
			break;
		case UVAL('m','m'):
			if (unit) *unit = SP_SVG_UNIT_MM;
			if (computed) *computed = v * PX_PER_MM;
			break;
		case UVAL('c','m'):
			if (unit) *unit = SP_SVG_UNIT_CM;
			if (computed) *computed = v * PX_PER_CM;
			break;
		case UVAL('i','n'):
			if (unit) *unit = SP_SVG_UNIT_IN;
			if (computed) *computed = v * PX_PER_IN;
			break;
		case UVAL('e','m'):
			if (unit) *unit = SP_SVG_UNIT_EM;
			break;
		case UVAL('e','x'):
			if (unit) *unit = SP_SVG_UNIT_EX;
			break;
		default:
			/* Invalid */
			return 0;
			break;
		}
		if (val) *val = v;
		if (next) *next = (char *) e + 2; 
		return 1;
	}

	/* Invalid */
	return 0;
}

unsigned int sp_svg_length_read_ldd (const gchar *str, SPSVGLengthUnit *unit, double *value, double *computed) {
	float a, b;
	unsigned int r = sp_svg_length_read_lff (str, unit, &a, &b, NULL);
	if (r) {
		if(value) *value = a;
		if(computed) *computed = b;
	}
	return r;
}

void
sp_svg_length_set (SPSVGLength *length, SPSVGLengthUnit unit, float value, float computed)
{
	length->set = 1;
	length->unit = unit;
	length->value = value;
	length->computed = computed;
}

void
sp_svg_length_unset (SPSVGLength *length, SPSVGLengthUnit unit, float value, float computed)
{
	length->set = 0;
	length->unit = unit;
	length->value = value;
	length->computed = computed;
}

void
sp_svg_length_update (SPSVGLength *length, double em, double ex, double scale)
{
	if (length->unit == SP_SVG_UNIT_EM) {
		length->computed = length->value * em;
	} else if (length->unit == SP_SVG_UNIT_EX) {
		length->computed = length->value * ex;
	} else if (length->unit == SP_SVG_UNIT_PERCENT) {
		length->computed = length->value * scale;
	}
}

double
sp_svg_read_percentage (const char * str, double def)
{
	char * u;
	double v;

	if (str == NULL) return def;

	v = g_ascii_strtod (str, &u);
	while (isspace (*u)) {
		if (*u == '\0') return v;
		u++;
	}
	if (*u == '%') v /= 100.0;

	return v;
}

const gchar *
sp_svg_length_get_css_units (SPSVGLengthUnit unit)
{
	switch (unit) {
	case SP_SVG_UNIT_NONE: return "";
	case SP_SVG_UNIT_PX: return "";
	case SP_SVG_UNIT_PT: return "pt";
	case SP_SVG_UNIT_PC: return "pc";
	case SP_SVG_UNIT_MM: return "mm";
	case SP_SVG_UNIT_CM: return "cm";
	case SP_SVG_UNIT_IN: return "in";
	case SP_SVG_UNIT_EM: return "em";
	case SP_SVG_UNIT_EX: return "ex";
	case SP_SVG_UNIT_PERCENT: return "%";
	}
	return "";
}

/**
 * N.B.\ This routine will sometimes return strings with `e' notation, so is unsuitable for CSS
 * lengths (which don't allow scientific `e' notation).
 */
const gchar *
sp_svg_length_write_with_units (SPSVGLength *length)
{
	Inkscape::SVGOStringStream os;
	if (length->unit == SP_SVG_UNIT_PERCENT)
		os << 100*length->value << sp_svg_length_get_css_units(length->unit);
	else 
		os << length->value << sp_svg_length_get_css_units(length->unit);
	return g_strdup(os.str().c_str());
}
