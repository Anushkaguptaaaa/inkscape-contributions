#ifndef __SP_STYLE_H__
#define __SP_STYLE_H__

/*
 * SPStyle - a style object for SPItems
 *
 * Author:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "color.h"
#include "forward.h"
#include "sp-marker-loc.h"

namespace Inkscape {
namespace XML {
class Node;
}
}

class SPCSSAttr;

namespace Inkscape {

/**
 * Parses a CSS url() specification; temporary hack until
 * style stuff is redone.
 * \param string the CSS string to parse
 * \return a newly-allocated URL string (or NULL); free with g_free()
 */
gchar *parse_css_url(const gchar *string);

}  // namespace Inkscape

class SPIFloat;
class SPIScale24;
class SPIInt;
class SPIShort;
class SPIEnum;
class SPIString;
class SPILength;
class SPIPaint;
class SPIFontSize;

struct SPIFloat {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int data : 30;
	float value;
};

#define SP_SCALE24_MAX ((1 << 24) - 1)
#define SP_SCALE24_TO_FLOAT(v) ((float) ((double) (v) / SP_SCALE24_MAX))
#define SP_SCALE24_FROM_FLOAT(v) ((int) ((v) * ((double) SP_SCALE24_MAX + 0.9999)))

struct SPIScale24 {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int value : 24;
};

struct SPIInt {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int data : 30;
	int value;
};

struct SPIShort {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int data : 14;
	int value : 16;
};

struct SPIEnum {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int value : 8;
	unsigned int computed : 8;
};

struct SPIString {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int data : 30;
	gchar *value;
};

enum {
	SP_CSS_UNIT_NONE,
	SP_CSS_UNIT_PX,
	SP_CSS_UNIT_PT,
	SP_CSS_UNIT_PC,
	SP_CSS_UNIT_MM,
	SP_CSS_UNIT_CM,
	SP_CSS_UNIT_IN,
	SP_CSS_UNIT_EM,
	SP_CSS_UNIT_EX,
	SP_CSS_UNIT_PERCENT
};

struct SPILength {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int unit : 4;
	float value;
	float computed;
};

#define SP_STYLE_FILL_SERVER(s) (((SPStyle *) (s))->fill.value.paint.server)
#define SP_STYLE_STROKE_SERVER(s) (((SPStyle *) (s))->stroke.value.paint.server)
#define SP_OBJECT_STYLE_FILL_SERVER(o) (SP_OBJECT (o)->style->fill.value.paint.server)
#define SP_OBJECT_STYLE_STROKE_SERVER(o) (SP_OBJECT (o)->style->stroke.value.paint.server)

enum {
	SP_PAINT_TYPE_NONE,
	SP_PAINT_TYPE_COLOR,
	SP_PAINT_TYPE_PAINTSERVER
};

struct SPIPaint {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int currentcolor : 1;
	unsigned int type : 2;
	union {
		SPColor color;
		struct {
			SPPaintServer *server;
			gchar *uri;
		} paint;
	} value;
};

enum {
	SP_FONT_SIZE_LITERAL,
	SP_FONT_SIZE_LENGTH,
	SP_FONT_SIZE_PERCENTAGE
};

#define SP_FONT_SIZE ((1 << 24) - 1)

#define SP_F8_16_TO_FLOAT(v) ((gdouble) (v) / (1 << 16))
#define SP_F8_16_FROM_FLOAT(v) ((int) ((v) * ((1 << 16) + 0.9999)))

#define SP_STYLE_FLAG_IFSET (1 << 0)
#define SP_STYLE_FLAG_IFDIFF (1 << 1)
#define SP_STYLE_FLAG_ALWAYS (1 << 2)

struct SPIFontSize {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int type : 2;
	unsigned int value : 24;
	float computed;
};

struct SPITextDecoration {
    unsigned int set : 1;
    unsigned int inherit : 1;
    unsigned int underline : 1;
    unsigned int overline : 1;
    unsigned int line_through : 1;
    unsigned int blink : 1;    // "Conforming user agents are not required to support this value." yay!
};

struct SPILengthOrNormal {
	unsigned int set : 1;
	unsigned int inherit : 1;
	unsigned int normal : 1;
	unsigned int unit : 4;
	float value;
	float computed;
};

class SPTextStyle;

class NRVpathDash {
 public:
  double offset;
  int n_dash;
  double *dash;
};

struct SPStyle {
	int refcount;
	/** Object we are attached to */
	SPObject *object;
	/** Our text style component */
	SPTextStyle *text;
	unsigned int text_private : 1;

	/* CSS2 */
	/* Font */
    /** Size of the font */
	SPIFontSize font_size;
    /** Style of the font */
	SPIEnum font_style;
    /** Which substyle of the font */
	SPIEnum font_variant;
    /** Weight of the font */
	SPIEnum font_weight;
    /** Stretch of the font */
	SPIEnum font_stretch;

    /** First line indent of paragraphs (css2 16.1) */
    SPILength text_indent;
    /** text alignment (css2 16.2) (not to be confused with text-anchor) */
    SPIEnum text_align;
    /** text decoration (css2 16.3.1) */
    SPITextDecoration text_decoration;
    // 16.3.2 is text-shadow. That's complicated.
    /** Line spacing (css2 10.8.1) */
	SPILengthOrNormal line_height;
    /** letter spacing (css2 16.4) */
	SPILengthOrNormal letter_spacing;
    /** word spacing (also css2 16.4) */
	SPILengthOrNormal word_spacing;
    /** capitalization (css2 16.5) */
    SPIEnum text_transform;

    /* CSS3 Text */
    /** text direction (css3 text 3.2) */
    SPIEnum direction;
    /** block progression (css3 text 3.2) */
    SPIEnum block_progression;
	/** Writing mode (css3 text 3.2 and svg1.1 10.7.2) */
	SPIEnum writing_mode;

	/* SVG */
	/** Anchor of the text (svg1.1 10.9.1) */
	SPIEnum text_anchor;

	/* Misc attributes */
	unsigned int clip_set : 1;
	unsigned int color_set : 1;
	unsigned int cursor_set : 1;
	unsigned int overflow_set : 1;
	unsigned int clip_path_set : 1;
	unsigned int clip_rule_set : 1;
	unsigned int mask_set : 1;

	/** display */
	SPIEnum display;

	/* overflow */
	SPIEnum overflow;

	/* visibility */
	SPIEnum visibility;

	/** opacity */
	SPIScale24 opacity;

	/** color */
	SPIPaint color;

	/** fill */
	SPIPaint fill;
	/** fill-opacity */
	SPIScale24 fill_opacity;
	/** fill-rule: 0 nonzero, 1 evenodd */
	SPIEnum fill_rule;

	/** stroke */
	SPIPaint stroke;
	/** stroke-width */
	SPILength stroke_width;
	/** stroke-linecap */
	SPIEnum stroke_linecap;
	/** stroke-linejoin */
	SPIEnum stroke_linejoin;
	/** stroke-miterlimit */
	SPIFloat stroke_miterlimit;
	/** stroke-dash* */
	NRVpathDash stroke_dash;
	unsigned int stroke_dasharray_set : 1;
	unsigned int stroke_dasharray_inherit : 1;
	unsigned int stroke_dashoffset_set : 1;
	/** stroke-opacity */
	SPIScale24 stroke_opacity;

	/** Marker list */
	SPIString marker[SP_MARKER_LOC_QTY];

	// style belongs to a cloned object, must not href anything
	bool cloned; 
};

/**
 *
 */
SPStyle *sp_style_new (void);

/**
 *
 */
SPStyle *sp_style_new_from_object (SPObject *object);

/**
 *
 */
SPStyle *sp_style_ref (SPStyle *style);

/**
 *
 */
SPStyle *sp_style_unref (SPStyle *style);

/**
 * 1. Reset existing object style
 * 2. Load current effective object style
 * 3. Load i attributes from immediate parent (which has to be up-to-date)
 */
void sp_style_read_from_object (SPStyle *style, SPObject *object);

/**
 *
 */
void sp_style_read_from_repr (SPStyle *style, Inkscape::XML::Node *repr);

/**
 *
 */
void sp_style_merge_from_style_string (SPStyle *style, const gchar *p);

/**
 *
 */
void sp_style_merge_from_parent (SPStyle *style, SPStyle *parent);

/**
 *
 */
gchar *sp_style_write_string(SPStyle const *style, guint flags = SP_STYLE_FLAG_IFSET);

/**
 *
 */
gchar *sp_style_write_difference(SPStyle const *from, SPStyle const *to);

/**
 *
 */
void sp_style_set_fill_color_alpha (SPStyle *style, const SPColor* color, float a, unsigned int fill_set, unsigned int opacity_set);

/**
 *
 */
void sp_style_set_stroke_color_alpha (SPStyle *style, const SPColor* color, float a, unsigned int fill_set, unsigned int opacity_set);

/**
 *
 */
void sp_style_set_opacity (SPStyle *style, float opacity, unsigned int opacity_set);

/* SPTextStyle */

enum SPCSSFontSize {
	SP_CSS_FONT_SIZE_XX_SMALL,
	SP_CSS_FONT_SIZE_X_SMALL,
	SP_CSS_FONT_SIZE_SMALL,
	SP_CSS_FONT_SIZE_MEDIUM,
	SP_CSS_FONT_SIZE_LARGE,
	SP_CSS_FONT_SIZE_X_LARGE,
	SP_CSS_FONT_SIZE_XX_LARGE,
	SP_CSS_FONT_SIZE_SMALLER,
	SP_CSS_FONT_SIZE_LARGER
};

enum SPCSSFontStyle {
	SP_CSS_FONT_STYLE_NORMAL,
	SP_CSS_FONT_STYLE_ITALIC,
	SP_CSS_FONT_STYLE_OBLIQUE
};

enum SPCSSFontVariant {
	SP_CSS_FONT_VARIANT_NORMAL,
	SP_CSS_FONT_VARIANT_SMALL_CAPS
};

enum SPCSSFontWeight {
	SP_CSS_FONT_WEIGHT_100,
	SP_CSS_FONT_WEIGHT_200,
	SP_CSS_FONT_WEIGHT_300,
	SP_CSS_FONT_WEIGHT_400,
	SP_CSS_FONT_WEIGHT_500,
	SP_CSS_FONT_WEIGHT_600,
	SP_CSS_FONT_WEIGHT_700,
	SP_CSS_FONT_WEIGHT_800,
	SP_CSS_FONT_WEIGHT_900,
	SP_CSS_FONT_WEIGHT_NORMAL,
	SP_CSS_FONT_WEIGHT_BOLD,
	SP_CSS_FONT_WEIGHT_LIGHTER,
	SP_CSS_FONT_WEIGHT_BOLDER
};

enum SPCSSFontStretch {
	SP_CSS_FONT_STRETCH_ULTRA_CONDENSED,
	SP_CSS_FONT_STRETCH_EXTRA_CONDENSED,
	SP_CSS_FONT_STRETCH_CONDENSED,
	SP_CSS_FONT_STRETCH_SEMI_CONDENSED,
	SP_CSS_FONT_STRETCH_NORMAL,
	SP_CSS_FONT_STRETCH_SEMI_EXPANDED,
	SP_CSS_FONT_STRETCH_EXPANDED,
	SP_CSS_FONT_STRETCH_EXTRA_EXPANDED,
	SP_CSS_FONT_STRETCH_ULTRA_EXPANDED,
	SP_CSS_FONT_STRETCH_NARROWER,
	SP_CSS_FONT_STRETCH_WIDER
};

enum SPCSSTextAlign {
	SP_CSS_TEXT_ALIGN_LEFT,
	SP_CSS_TEXT_ALIGN_RIGHT,
	SP_CSS_TEXT_ALIGN_CENTER,
    SP_CSS_TEXT_ALIGN_JUSTIFY
    // also <string> is allowed, but only within table calls
};

enum SPCSSTextTransform {
	SP_CSS_TEXT_TRANSFORM_CAPITALIZE,
	SP_CSS_TEXT_TRANSFORM_UPPERCASE,
	SP_CSS_TEXT_TRANSFORM_LOWERCASE,
	SP_CSS_TEXT_TRANSFORM_NONE
};

enum SPCSSDirection {
	SP_CSS_DIRECTION_LTR,
	SP_CSS_DIRECTION_RTL
};

enum SPCSSBlockProgression {
	SP_CSS_BLOCK_PROGRESSION_TB,
	SP_CSS_BLOCK_PROGRESSION_RL,
	SP_CSS_BLOCK_PROGRESSION_LR
};

enum SPCSSWritingMode {
	SP_CSS_WRITING_MODE_LR_TB,
	SP_CSS_WRITING_MODE_RL_TB,
	SP_CSS_WRITING_MODE_TB_RL,
	SP_CSS_WRITING_MODE_TB_LR
};

enum SPTextAnchor {
	SP_CSS_TEXT_ANCHOR_START,
	SP_CSS_TEXT_ANCHOR_MIDDLE,
	SP_CSS_TEXT_ANCHOR_END
};

enum SPVisibility {
	SP_CSS_VISIBILITY_HIDDEN,
	SP_CSS_VISIBILITY_COLLAPSE,
	SP_CSS_VISIBILITY_VISIBLE
};

enum SPOverflow {
	SP_CSS_OVERFLOW_VISIBLE,
	SP_CSS_OVERFLOW_HIDDEN,
	SP_CSS_OVERFLOW_SCROLL,
	SP_CSS_OVERFLOW_AUTO
};

// TODO: more display types
enum SPCSSDisplay {
	SP_CSS_DISPLAY_NONE,
	SP_CSS_DISPLAY_INLINE,
	SP_CSS_DISPLAY_BLOCK,
	SP_CSS_DISPLAY_LIST_ITEM,
	SP_CSS_DISPLAY_RUN_IN,
	SP_CSS_DISPLAY_COMPACT,
	SP_CSS_DISPLAY_MARKER,
	SP_CSS_DISPLAY_TABLE,
	SP_CSS_DISPLAY_INLINE_TABLE,
	SP_CSS_DISPLAY_TABLE_ROW_GROUP,
	SP_CSS_DISPLAY_TABLE_HEADER_GROUP,
	SP_CSS_DISPLAY_TABLE_FOOTER_GROUP,
	SP_CSS_DISPLAY_TABLE_ROW,
	SP_CSS_DISPLAY_TABLE_COLUMN_GROUP,
	SP_CSS_DISPLAY_TABLE_COLUMN,
	SP_CSS_DISPLAY_TABLE_CELL,
	SP_CSS_DISPLAY_TABLE_CAPTION
};

struct SPTextStyle {
	int refcount;

	/* CSS font properties */
	SPIString font_family;

	/* fixme: The 'font' property is ugly, and not working (lauris) */
	SPIString font;
};

SPCSSAttr * sp_css_attr_from_style (SPObject *object, guint flags = SP_STYLE_FLAG_IFSET);
SPCSSAttr * sp_css_attr_unset_text (SPCSSAttr *css);
SPCSSAttr * sp_css_attr_unset_uris (SPCSSAttr *css);
SPCSSAttr * sp_css_attr_scale (SPCSSAttr *css, double ex);

void sp_style_unset_property_attrs (SPObject *o);

#endif
