/** Area module --
    This module provides all services for area definitions in the
    generic SDCC linker.

    An area is a group of code or data snippets which share some
    properties.  E.g., code within an area may be overlayed if the
    area specifies that.  Areas are abstract groups that are defined
    by segments within modules.  I.e., segments within object modules
    with the same name are considered to belong to the same area and
    inherit its attributes.

    One can query this module for all areas, for the name and
    attributes of a specific area, the list of its segments and so on.
    It is also possible to set most of those properties as well.

    There is also the concept of a current area, which is the area
    currently active during processing of a module.  All definitions
    go to the segment belonging to this area and the current module.

    Finally an area may be linked: all addresses of segments within
    that area are resolved (depending on whether they are overlayed or
    concatenated).

    Original version by Thomas Tensi, 2006-07
    based on the module lkarea.c by Alan R. Baldwin
*/

#ifndef __AREA_H
#define __AREA_H

/*========================================*/

#include "globdefs.h"
#include "list.h"
#include "set.h"
#include "string.h"
#include "target.h"
#include "typedescriptor.h"

/* the types are declared here because they are needed in module.h and
   symbol.h */
typedef struct Area__SegmentRecord *Area_Segment;
  /** a segment within an area (as an opaque type) */

typedef List_Type Area_SegmentList;
  /** a list of segments */

#include "module.h"
#include "symbol.h"

/*========================================*/

typedef struct Area__Record *Area_Type;
  /** type representing a group of link segments */


typedef enum {
  Area_Attribute_isAbsolute, Area_Attribute_hasOverlayedSegments,
  Area_Attribute_hasPagedSegments, Area_Attribute_isInCodeSpace,
  Area_Attribute_isInExternalDataSpace, Area_Attribute_isInBitSpace,
  Area_Attribute_isNonloadable
} Area_Attribute;
  /** properties of an area like being absolute, having overlayed or
      paged segments and special flags for different other target
      platforms; absence of certain properties is specified by not
      including them into the set */


typedef Set_Type Area_AttributeSet;
  /** set of Area_Attribute */


typedef List_Type Area_List;
  /** a list of areas */

/*--------------------*/

extern TypeDescriptor_Type Area_typeDescriptor;
  /** variable used for describing the type properties when area
      objects occur in generic types like lists */


extern TypeDescriptor_Type Area_segmentTypeDescriptor;
  /** variable used for describing the type properties when segment
      objects occur in generic types like lists */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Area_initialize (void);
  /** sets up all internal data structures */

/*--------------------*/

void Area_finalize (void);
  /** cleans up all internal data structures */

/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Area_Type Area_make (in String_Type areaName, 
		     in Area_AttributeSet attributeSet);
  /** ensures that a new area with <areaName> exists; its attributes
      are given by <attributeSet>; when set of attributes is not
      identical to previous area definition, this is an error */

/*--------------------*/

void Area_makeSegment (in String_Type areaName, 
		       in Target_Address totalSize,
		       in Area_AttributeSet attributeSet);
  /** adds a new segment to area with <areaName>; size of segment is
      given by <totalSize>, attributes by <attributeSet>; when set of
      attributes is not identical to previous area definition, this is
      an error */

/*--------------------*/

void Area_makeAbsoluteSegment (void);
  /** adds a new segment to absolute area */

/*--------------------*/

Area_AttributeSet Area_makeAttributeSet (in UINT8 attributeSetEncoding);
  /** constructs an area attribute set from some external encoding */

/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Area_destroy (inout Area_Type *area);
  /** destroys <area> completely and frees all its resources */


/*--------------------*/
/* STATUS REPORT      */
/*--------------------*/

Area_Segment Area_currentSegment (void);
  /** returns currently active segment */

/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Area_getList (inout Area_List *areaList);
  /** returns list of all known areas in <areaList> */

/*--------------------*/

void Area_getName (in Area_Type area, out String_Type *name);
  /** returns name of <area> in <name> */

/*--------------------*/

Area_AttributeSet Area_getAttributes (in Area_Type area);
  /** returns attributes of <area> */

/*--------------------*/

Target_Address Area_getAddress (in Area_Type area);
  /** returns address of <area> */

/*--------------------*/

void Area_getListOfSegments (in Area_Type area, 
			     out Area_SegmentList *segmentList);
  /** returns segments of <area> in <segmentList> */

/*--------------------*/

UINT8 Area_getMemoryPage (in Area_Type area);
  /** returns assigned memory page for <area> (if applicable for
      platform) */

/*--------------------*/

Target_Address Area_getSegmentAddress (in Area_Segment segment);
  /** returns address of <segment> */

/*--------------------*/

Area_Type Area_getSegmentArea (in Area_Segment segment);
  /** returns area of <segment> */

/*--------------------*/

void Area_getSegmentName (in Area_Segment segment, out String_Type *name);
  /** returns name of <segment> in <name> */

/*--------------------*/

Module_Type Area_getSegmentModule (in Area_Segment segment);
  /** returns associated module for <segment> */

/*--------------------*/

void Area_getSegmentSymbols (in Area_Segment segment, 
			     out Symbol_List *symbolList);
  /** returns all symbols in <segment> in <symbolList> */

/*--------------------*/

Target_Address Area_getSize (in Area_Type area);
  /** returns size of <area> */

/*--------------------*/
/* SELECTION          */
/*--------------------*/

void Area_setCurrent (in Area_Segment segment);
  /** sets current segment to <segment> */

/*--------------------*/

void Area_lookup (out Area_Type *area, in String_Type areaName);
  /** looks up area with <name> and sets area accordingly (or to NULL,
      if not found) */

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Area_addSymbolToSegment (inout Area_Segment *segment,
			      in Symbol_Type symbol);
  /** adds <symbol> to module of <segment> */

/*--------------------*/

void Area_clearListOfSegments (inout Area_Type *area);
  /** removes all segments of <area> */

/*--------------------*/

void Area_link (void);
  /** resolves all area addresses by traversing all the areas and the
      associated segments; the address allocation is done depending on
      the attributes of the area:
        - for overlayed areas all segments starts at the identical
	  base area address overlaying each other and the size of the
	  area is the maximum of the area segments
        - for concatenated areas all segments are concatenated with
	  the first segment starting at the base area address and the
	  size of the area is the sum of the segment sizes
      if a base address for an area is specified then the
      area will start at that address.	Any relocatable
      areas defined subsequently will be concatenated to the
      previous relocatable area if it does not have a base
      address specified;
      additionally the symbols named s_<areaName> and l_<areaName> are
      created to define the starting address and length of each
      area */

/*--------------------*/

void Area_replaceSegmentSymbol (inout Area_Segment *segment, 
				in Symbol_Type oldSymbol, 
				in Symbol_Type newSymbol);
  /** replaces <oldSymbol> in symbol list of <segment> by <newSymbol>;
      does nothing when <oldSymbol> does not occur */

/*--------------------*/

void Area_setBaseAddresses (in String_Type segmentName,
			    in Target_Address baseAddress);
  /** sets addresses of all segments with <segmentName> to <baseAddress> */

/*--------------------*/

void Area_setSegmentArea (inout Area_Segment *segment, in Area_Type area);
  /** sets area of <segment> to <area> */


/*--------------------*/
/* CONVERSION         */
/*--------------------*/

void Area_toString (in Area_Type area, out String_Type *representation);
  /** constructs a printable representation of <area> and its
      internal data (for debugging purposes) and concatenates it to
      <representation> */


/*--------------------*/

void Area_segmentToString (in Area_Segment segment,
			   out String_Type *representation);
  /** constructs a printable representation of <segment> and its
      internal data (for debugging purposes) and concatenates it to
      <representation> */


#endif /* __AREA_H */
