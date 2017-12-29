/** CodeSequence module --
    This module provides all services for scanning code and relocating
    it in the generic SDCC linker.

    As the name indicates it encapsulates code sequences, i.e. byte
    sequences to be put to some bank at some specific address.

    Often relocation is needed.  This is modeled as a sequence of
    simple relocations, where each simple relocation specifies some
    position in a code sequence, an offset value and some indication
    on how the value pointed to has to be combined with the offset.
    The central routine of this module applies a relocation list to
    some code sequence.

    Original version by Thomas Tensi, 2006-09
    based on the module lkrloc.c by Alan R. Baldwin
*/

#ifndef __CODESEQUENCE_H
#define __CODESEQUENCE_H

/*========================================*/

//#include "area.h"  -- used for Area_Segment below
#include "globdefs.h"
#include "target.h"

/*========================================*/

#define CodeSequence_maxLength 256
  /** maximum length of single codesequence to be relocated */

typedef struct {
  /* Area_Segment segment; //changed to break circular includes */
  struct Area__SegmentRecord *segment;
  Target_Bank romBank;
  UINT32 offsetAddress;
  UINT8 length;    /** number of bytes in sequence */
  UINT8 byteList[CodeSequence_maxLength];
} CodeSequence_Type;
  /** type representing a code sequence before or after relocation */


typedef struct {
  Boolean msbByteIsUsed : 1;
  Boolean isThreeByteAddress : 1;
  Boolean mostSignificantByteIsUsed : 1;
  Boolean pageIsReferenced : 1;
  Boolean zeroPageIsReferenced : 1;
  Boolean dataIsSigned : 1;
  Boolean slotWidthIsTwo : 1;
  Boolean isRelocatedPCRelative : 1;
  Boolean isSymbol : 1;
  Boolean elementsAreBytes : 1;
} CodeSequence_RelocationKind;
  /** relocation attribute for a single relocation entry */


typedef struct {
  CodeSequence_RelocationKind kind;
  UINT8 index;
  UINT16 value;
} CodeSequence_Relocation;
  /** a single relocation for a previous code line has a relocation
      <kind>, an <index> into the unrelocated code line and a
      relocation <value> (typically some offset) */


typedef struct {
  /* Area_Segment segment; //changed to break circular includes */
  struct Area__SegmentRecord *segment;
  CodeSequence_Relocation list[CodeSequence_maxLength];
  UINT8 count;
} CodeSequence_RelocationList;
/** a relocation list for a code line has a <segment> of
    relocation and several relocations and a count of relocations */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void CodeSequence_initialize (void);
  /** sets up internal data structures */

/*--------------------*/

void CodeSequence_finalize (void);
  /** cleans up internal data structures */


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

void CodeSequence_makeKindFromInteger (out CodeSequence_RelocationKind *kind,
				       in UINT8 value);
  /** makes a relocation kind <kind> from some external representation
      <value> */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void CodeSequence_relocate (inout CodeSequence_Type *sequence,
			    in UINT16 areaMode,
			    in CodeSequence_RelocationList *relocationList);
/** relocates code line <sequence> based on information in an R line
    of the linker input; an R line contains an area index specifying
    the associated area, a mode for that area and a list of four byte
    relocation data: that information is given in <areaMode> and
    <relocationList> */


/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

UINT8 CodeSequence_convertToInteger (in CodeSequence_RelocationKind kind);
  /** converts relocation kind <kind> to external integer
      representation */



#endif /* __CODESEQUENCE_H */
