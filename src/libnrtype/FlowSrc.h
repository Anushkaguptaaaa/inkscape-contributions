/*
 *  FlowSrc.h
 */

#ifndef my_flow_src
#define my_flow_src

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FlowDefs.h"
#include "FlowSrcText.h"

#include <glib.h>
#include "../svg/svg.h"

#include "FlowStyle.h"

class text_holder;
class text_style;
class flow_eater;
class line_solutions;
class SPStyle;
class flow_src;

class SPObject;

/*
 * Classes to collect the text from the SPObject tree
 *
 * 2 steps:
 *	- one_flow_src variants are included in each element that can contribute text, that is in SPText, SPTspan, SPTextpath, SPString,
 * SPFlowdiv, SPFlowpara, SPFlowspan, SPFlowLine SPFlowRegionBreak. Before the flow is collected, this one_flow_src instances are
 * linked in a doubly-linked list by sp-text and sp-flowtext.
 *  - This linked list is then converted to a 'flat' flow_src, which is basically an array of text control elements and text paragraphs.
 * paragraphs are stored in text_holder instances.
 *
 * additionally, one_flow_src instances contain utf8_st and utf8_en fields representing the interval they take in the text
 * element. these values are computed before the flow, and used when the text is modified.
 */


class div_flow_src;
// lightweight class to be included in those filling the flow source
class one_flow_src {
public:
	SPObject*        me; // The SPObject from thich this ofc is created; this may be any text element or SPString

	// the text interval held by this object
      // the range is: for SPString, the number of chars; for line tspan, 1; for other elements 0
	int              ucs4_st, ucs4_en;
	int              utf8_st, utf8_en; 

	// linking in the flow; 
	one_flow_src     *next, *prev; // the chain is a serialization of the text source tree, e.g. text->tspan1->string1->tspan2->string2
	one_flow_src		 *dad, *chunk; // chunk=NULL means it's a flow start, a paragraph or a sodipodi:role=line tspan
	
	one_flow_src(SPObject* i_me);
	virtual ~one_flow_src(void);
	
    /** joins this element as a child of \a inside and after \a after. */
	void              Link(one_flow_src* after, one_flow_src* inside);
	void              DoPositions(bool for_text);
	void              DoFill(flow_src* what);
	one_flow_src*     Locate(int utf8_pos, int &ucs4_pos, bool src_start, bool src_end, bool must_be_text);
    /** returns the total number of bytes before an absolute character
    index over the whole tree. */
	int               Do_UCS4_2_UTF8(int ucs4_pos);
    /** returns the total number of characters before an absolute byte
    index over the whole tree. */
	int               Do_UTF8_2_UCS4(int utf8_pos);
	
	// introspection
	virtual int       Type(void) =0;
	// asks for kerning info to be pushed in the text_holder. st/ en /offset are ucs4 positions
	virtual void      PushInfo(int st,int en,int offset,text_holder* into);
	// tells the element to remove the info such as x/y/dx/dy/rotate stuff it might hold for the specified portion
	virtual void      DeleteInfo(int i_utf8_st, int i_utf8_en, int i_ucs4_st, int i_ucs4_en);
	// returns a text_style for this element. the caller should take care of deallocating it
	virtual text_style*  GetStyle(void);
	// function called after SetPosition to fill the flow_src instance
	virtual void      Fill(flow_src* what);
	// function used to prepare the element, most notably to computed the interval in the text it represents
	virtual void      SetPositions(bool for_text, int &last_utf8, int &last_ucs4, bool &in_white);
	// insert some text 'n_text' at positoin utf8_pos in the text. 'done' is set if this call actually inserted all the text
	virtual void      Insert(int utf8_pos, int ucs4_pos, const char* n_text, int n_utf8_len, int n_ucs4_len, bool &done); 
	// delete a portion of the text
	virtual void      Delete(int i_utf8_st, int i_utf8_en);
	// set/ add some value at the given position. v_type=0 -> set the 'x' value; 1->'y'; 2->'dx'; 3->'dy'; 4->'rotate'
	virtual void      AddValue(int ucs4_pos, SPSVGLength &val, int v_type, bool increment, bool multiply);
	virtual int       UCS4_2_UTF8(int ucs4_pos);
	virtual int       UTF8_2_UCS4(int utf8_pos);
};
// text variant
class text_flow_src : public one_flow_src {
public:
	partial_text      cleaned_up;
	correspondance    string_to_me;
	
	text_flow_src(SPObject* i_me);
	virtual ~text_flow_src(void);
	
	void              SetStringText(partial_text* iTxt);
			
	virtual int       Type(void) {return flw_text;};
	virtual void      Fill(flow_src* what);
	virtual void      SetPositions(bool for_text, int &last_utf8, int &last_ucs4, bool &in_white);
	virtual void      Insert(int utf8_pos, int ucs4_pos, const char* n_text, int n_utf8_len, int n_ucs4_len, bool &done); 
	virtual void      Delete(int i_utf8_st, int i_utf8_en);
	virtual void      AddValue(int ucs4_pos, SPSVGLength &val, int v_type, bool increment, bool multiply);
	virtual int       UCS4_2_UTF8(int ucs4_pos);
	virtual int       UTF8_2_UCS4(int utf8_pos);
};
// control stuff in the flow, like line and region breaks
class control_flow_src : public one_flow_src {
public:
	int               type;
	
	control_flow_src(SPObject* i_me, int i_type);
	virtual ~control_flow_src(void);
	
	virtual int       Type(void) {return type;};
	virtual void      Fill(flow_src* what);
	virtual void      SetPositions(bool for_text, int &last_utf8, int &last_ucs4, bool &in_white);
	virtual int       UCS4_2_UTF8(int ucs4_pos);
	virtual int       UTF8_2_UCS4(int utf8_pos);
};
// object variant, to hold placement info. it's a text/ tspan/ textpath/ flowdiv/ flowspan/ flowpara
class div_flow_src : public one_flow_src {
public:
	int 							type;
	bool              is_chunk_start;
	bool              is_chunk_end;
	bool              vertical_layout;
	SPStyle           *style; // only for simplicity
	                          // this has to last as long as the flow_res we're going to derive from it
	                          // hence the style_holder class
	int               nb_x, nb_y, nb_rot, nb_dx, nb_dy;
	SPSVGLength       *x_s,*y_s,*rot_s,*dx_s,*dy_s;
	
	div_flow_src(SPObject* i_me, int i_type);
	virtual ~div_flow_src(void);
	
	// general purpose functions for manipulating the various attributes
	static void       ReadArray(int &nb,SPSVGLength* &array,const char* from);
	static char*      WriteArray(int nb,SPSVGLength* array);
	static void       InsertArray(int l,int at,int &nb,SPSVGLength* &array,bool is_delta);
	static void       SuppressArray(int l,int at,int &nb,SPSVGLength* &array);
	static void       ForceVal(int at,SPSVGLength &val,int &nb,SPSVGLength* &array,bool increment, bool multiply);
	static void       UpdateArray(double size,double scale,int &nb,SPSVGLength* &array);

	// transform (x_s[i], y_s[i]) points by the matrix t
	void       TransformXY(NR::Matrix const &t, bool toplevel);
	// scale dx/dy arrays by ex
	void       ScaleDXDY(double ex);

	// sets the SPSVGLength 'computed' field of the attributes it holds
	void              UpdateLength(double size,double scale);
	// returns the style for this element
	void              SetStyle(SPStyle* i_style);
	// wrappers
	void							SetX(const char* val);
	void							SetY(const char* val);
	void							SetDX(const char* val);
	void							SetDY(const char* val);
	void							SetRot(const char* val);
	char*							GetX(int st=-1,int en=-1);
	char*							GetY(int st=-1,int en=-1);
	char*							GetDX(int st=-1,int en=-1);
	char*							GetDY(int st=-1,int en=-1);
	char*							GetRot(int st=-1,int en=-1);
	// called by text_flow_src->AddValue(), because only the text element can do the utf8->ucs4 conversion
	void              DoAddValue(int ucs4_pos, SPSVGLength &val, int v_type, bool increment, bool multiply);
	
	int               UCS4Pos(int i_utf8_pos);
	
	virtual int       Type(void) {return type;};
	virtual void      PushInfo(int st,int en,int offset,text_holder* into);
	virtual void      DeleteInfo(int i_utf8_st,int i_utf8_en,int i_ucs4_st,int i_ucs4_en);
	virtual text_style*  GetStyle(void);
	virtual void      Fill(flow_src* what);
	virtual void      SetPositions(bool for_text,int &last_utf8,int &last_ucs4,bool &in_white);
	virtual void      Insert(int utf8_pos,int ucs4_pos,const char* n_text,int n_utf8_len,int n_ucs4_len,bool &done); 
	virtual void      Delete(int i_utf8_st,int i_utf8_en);
	virtual int       UCS4_2_UTF8(int ucs4_pos);
	virtual int       UTF8_2_UCS4(int utf8_pos);
};

/*
 * source of the flow
 * it has a flow_styles and thus holds an array of all the text_styles used in the source text
 */
/** \brief collects all the one_flow_src classes together into one place for easier processing.

Used as a store of amalgamated one_flow_src classes.

This class actually does very little by itself. The collection of data is
performed by the one_flow_src tree and the bulk of the processing is done
by the text_holder class.

Usage:
 -# construct
 -# call one_flow_src::DoFill() on the root element of your tree
 -# call Prepare()
 -# use a flow_maker
 -# destruct
*/
class flow_src {
public:
	typedef struct one_elem {
		int               type;
		text_holder*      text;     ///only valid for elements of type flw_text
		one_flow_src*     obj;
	} one_elem;
	int                 nbElem, maxElem;
	one_elem*           elems;

	text_holder*        cur_holder;
	
    /** all the text_style classes necessary to render this text. They are
    all deleted on destruction. */
    flow_styles styles;

	bool                min_mode;

	flow_src(void);
	~flow_src(void);
		
    /** internal libnrtype method. a simple append to the #elems array.
    Called by the one_flow_src::Fill() inheritors. */
	void                AddElement(int i_type,text_holder* i_text,one_flow_src* i_obj);

	/** extracts just the raw text from the #cur_holder element. Line
    breaks are correctly processed and returned as linefeeds. The caller
    is responsible for free()ing the return value. *FIXME many callers
    don't* */
    char*               Summary(void) const;
	
    /** internal libnrtype method. Conanicalises the values in it parameters,
    such that if \a pos is out of bounds it and \a no are corrected to the
    nearest neighbour.
     \param no   index into #elems
     \param pos  index into text_holder::boxes
    */
	void                Clean(int &no,int &pos);
	
    /** initialise the text_holder member of all the flw_text items of
    #elems. This includes calling text_holder::DoChunking() and
    text_holder::ComputeBoxes(). This must be called prior to using this
    class in a flow_maker. */
	void                Prepare(void);
	
	// wrapper for their text_holder counterparts, mainly.
    /** internal libnrtype method. Compute the four metrics for the piece of
    text at #elems \a from_no and text_holder::boxes \a from_pos. If
    \a from_no is not a text item, the metrics for the first text after it
    will be returned. */
	void                MetricsAt(int from_no,int from_pos,double &ascent,double &descent,double &leading,bool &flow_rtl);

    /** internal libnrtype method. Calls text_holder::ComputeSols() to
    calculate all the possible wrapping positions for the text starting
    at the given position. line_solutions::StartLine() must already have
    been called. */
	void                ComputeSol(int from_no,int from_pos,line_solutions *sols,bool &flow_rtl);

    /** internal libnrtype method. Calls text_holder::Feed() repeatedly to
    send all the glyphs from in the given range to the flow_res associated
    with \a baby. */
	void                Feed(int st_no,int st_pos,int en_no,int en_pos,bool flow_rtl,flow_eater* baby);

	// finds the text_holder in the specified input span in the one_elem array
    /** internal libnrtype method. Returns the first text_holder in the given
    span. */
	text_holder*        ParagraphBetween(int st_no,int st_pos,int en_no,int en_pos);

	/** debug method. Dumps an overview of the contents of #elems to
    stdout. */
    void                Affiche(void) const;
};


#endif
