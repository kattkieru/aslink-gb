/** Module module --
    This module provides all services for module definitions in the
    generic SDCC linker.

    A module is a group of code and data areas belonging together and
    is the root of all related linker objects.  Normally a module is
    defined by a single object file, but in case of a library several
    object files may contribute to a single module.

    Modules may have associated areas and symbols and appropriate
    routines provide access to those lists.

    There is also the notion of a current module, which is the one
    where the currently parsed symbols and areas are associated to.

    Original version by Thomas Tensi, 2006-08
    based on the module lkhead.c by Alan R. Baldwin
*/

#ifndef __MODULE_H
#define __MODULE_H

/*--------------------*/

/* the type is declared here because it is needed in area.h */
typedef struct Module__Record *Module_Type;
  /** type representing a module (as an opaque type) */

#include "area.h"
#include "file.h"
#include "globdefs.h"
#include "list.h"
#include "string.h"
#include "symbol.h"

/*========================================*/

typedef UINT16 Module_SegmentIndex;
  /** type for unique numbers of segments per module */

typedef UINT16 Module_SymbolIndex;
  /** type for unique numbers of symbols per module */

extern TypeDescriptor_Type Module_typeDescriptor;
  /** variable used for describing the type properties when module
      objects occur in generic types like lists */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Module_initialize (void);
  /** sets up all internal data structures */

/*--------------------*/

void Module_finalize (void);
  /** cleans up all internal data structures */


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

void Module_make (in String_Type associatedFileName,
		  in Module_SegmentIndex segmentCount,
		  in Module_SymbolIndex symbolCount);
  /** creates a new module structure and links it into the list of
      module structures; <associatedFileName> tells the file name,
      <segmentCount> and <symbolCount> tell how many area segments and
      symbols are in this module */


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Module_destroy (inout Module_Type *module);
  /** deallocates <module> */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Module_getFileName (in Module_Type module, out String_Type *fileName);
  /** returns associated file name of <module> in <fileName> */

/*--------------------*/

void Module_getName (in Module_Type module, out String_Type *name);
  /** returns name of <module> in <name> */

/*--------------------*/

Area_Segment Module_getSegment (in Module_Type module,
				in Module_SegmentIndex segmentIndex);
  /** returns segment with index <segmentIndex> within <module> or
      NULL if not found */

/*--------------------*/

Area_Segment Module_getSegmentByName (in Module_Type module,
				      in String_Type segmentName);
  /** returns segment with name <segmentName> within <module> or
      NULL if not found */

/*--------------------*/

Symbol_Type Module_getSymbol (in Module_Type module,
			      in Module_SymbolIndex symbolIndex);
  /** returns symbol with index <symbolIndex> within <module> or
      NULL if not found */

/*--------------------*/

Symbol_Type Module_getSymbolByName (in Module_Type module,
				    in String_Type symbolName);
  /** returns symbol with name <symbolName> within <module> or
      NULL if not found */


/*--------------------*/
/* STATUS REPORT      */
/*--------------------*/

Module_Type Module_currentModule (void);
  /** returns currently active module */


/*--------------------*/
/* ITERATION          */
/*--------------------*/

void Module_getModuleList (inout List_Type *moduleList);
  /** returns the list of all modules in <moduleList> */

/*--------------------*/

void Module_getSegmentList (in Module_Type module,
			    inout Area_SegmentList *segmentList);
  /** returns the list of all segments within <module> in <segmentList> */

/*--------------------*/

void Module_getSymbolList (in Module_Type module,
			   inout Symbol_List *symbolList);
  /** returns the list of all symbols within <module> in <symbolList> */


/*--------------------*/
/* SELECTION          */
/*--------------------*/

void Module_setCurrentByName (in String_Type name,
			      out Boolean *isFound);
  /** select current module by associated module name <name>;
      <isFound> tells whether search has been successful */

/*--------------------*/

void Module_setCurrentByFileName (in String_Type fileName,
				  out Boolean *isFound);
  /** select current module by associated file name <fileName>;
      <isFound> tells whether search has been successful */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Module_setName (in String_Type name);
  /** sets the name of the current module to <name> */

/*--------------------*/

void Module_addSegment (inout Module_Type *module, in Area_Segment segment);
  /** adds <segment> to <module> and returns that module */

/*--------------------*/

void Module_addSymbol (inout Module_Type *module, in Symbol_Type symbol);
  /** adds <symbol> to <module> */

/*--------------------*/

void Module_replaceSymbol (inout Module_Type *module, in Symbol_Type oldSymbol,
			   in Symbol_Type newSymbol);
  /** replaces <oldSymbol> in symbol list of <module> by <newSymbol>;
      does nothing when <oldSymbol> does not occur */


/*--------------------*/
/* CONVERSION         */
/*--------------------*/

void Module_toString (in Module_Type module, out String_Type *representation);
  /** constructs a printable representation of <module>, its internal
      data and its associated segments (for debugging purposes) and
      concatenates it to <representation> */

#endif /* __MODULE_H */
