/** Banking module --
    This module provides all services for code banking in the generic
    linker.

    Banking is done by reading a banking configuration file which
    tells the assignments of modules to bank numbers.  Whenever a jump
    from one code bank to a parallel bank occurs, this jump is
    replaced by a jump to stub code in a nonbanked area which takes
    care of the bank switching and also restores the calling bank when
    a return occurs (for a subroutine call).

    Unfortunately this mechanism only works for control flow via bank
    boundaries!  It fails e.g. when some constant should be read from
    another code bank.  So you should not do it (also not accidentally
    by passing constants as parameters to routines in other banks)...

    All this code change is done on-the-fly in the linker based on the
    configuration file.  So the decision about bank assignment can be
    done at link time and must not be done at compile time.  This also
    means that libraries can be arbitrarily placed in banks without
    recompilation.

    After the first link pass when all symbols have been resolved, the
    linker tracks whether interbank references occur.  For each
    interbank reference to some symbol, another artificial symbol is
    introduced (e.g. by adding a prefix).  This symbol will be defined
    by the linker and is located in a nonbanked area (which can be
    reached from all banks).  It stands for a trampoline call where
    the current bank is stored, the bank is switched to the target
    bank and finally the jump to the target address is done.

    For all that, the linker generates a temporary object file
    containing the definitions of the surrogate symbols and the
    program-specific trampoline code.  The glue code for bank
    switching comes from a library which is searched after inserting
    the generated banking object file.

    Of course, the specifics (and especially the code!) for banking
    heavily depend on the target platform.  Hence the generic routines
    in the banking module rely on a variable which describes all
    characteristics of the target platform needed for banking.  This
    variable tells the names of the generic banked area and the
    nonbanked area for the trampoline calls, how to construct a real
    banked area name from the bank number and most importantly a
    routine which generates the trampoline code for a single call.
    Note that the latter is somewhat tricky because this routine must
    generate object code (which references indexes of symbols...).

    The variable is set when the target platform plugin is
    initialized.  When banking is not used, the variable is null.

    Original version by Thomas Tensi, 2008-04
*/

#ifndef __BANKING_H
#define __BANKING_H

/*========================================*/

#include "globdefs.h"
#include "string.h"
#include "stringlist.h"

//#include "target.h"
typedef int Target_Bank;
  /** type defined here to break circular include when importing "target.h" */

/*========================================*/

typedef void (*Banking_CallTemplateProc) (in UINT16 startAddress,
					  in UINT16 referencedAreaIndex,
					  in UINT16 targetSymbolIndex,
					  in UINT16 jumpLabelSymbolIndex,
					  out String_Type *codeSequence);
  /** routine type for constructing the code for a trampoline call in
      the nonbanked code area; the start address of the code within
      the segment is given as <startAddress>, the target symbol is
      given by <targetSymbolIndex> in area with index
      <referencedAreaIndex> and the jump label symbol as index
      <jumpLabelSymbolIndex> in the same area; the routine returns
      several code lines in <codeSequence>: a T line with the call and
      a relocation line referencing the target symbol and the bank
      switch label

      e.g. in the GBZ80 implementation for a call to routine XYZ in
      bank 23 the following trampoline call code (in assembler
      notation) is generated

	BC_XYZ: LD   HL,#XYZ // JMP  Banking_switchTo_23

      this means that a definition for "BC_XYZ" in the nonbanked area
      is used and a reference to "Banking_switchTo_23" in banked area
      23; the code sequence effectively consists of six bytes (a 16
      bit load and an absolute jump) represented by a pair of a T and
      R line */


typedef void (*Banking_NameConstructionProc) (out String_Type *name,
					      in Target_Bank bank);
  /** routine type for constructing <name> from <bank> (e.g. for a jump
      label for some bank) */


typedef void (*Banking_SurrogateNameProc) (
				   out String_Type *surrogateSymbolName,
				   in String_Type symbolName);
  /** routine type for constructing the surrogate symbol name
      <surrogateSymbolName> used for a trampoline call from
      <symbolName> */


typedef Boolean (*Banking_TargetValidationProc) (
				 in String_Type moduleName,
				 in String_Type segmentName,
				 in String_Type symbolName);
  /** routine type for checking that some symbol is a valid target for
      an interbank call (i.e. it has to lie in a code segment) where
      symbol is characterized by <symbolName>, <segmentName> and
      <moduleName> */


typedef struct {
  String_Type genericBankedCodeAreaName;
    /** name of generic area used for banked code symbols when bank
        assignment is not yet done (e.g. "CODE_0") */
  String_Type nonbankedCodeAreaName;
    /** name of area used for nonbanked code symbols (e.g. "BASE") */
  Banking_NameConstructionProc makeBankedCodeAreaName;
    /** template routine for constructing a banked code area name from
        a bank (also handling undefined bank correctly) */
  Banking_NameConstructionProc makeJumpLabelName;
    /** template routine for constructing the jump label (for bank
        switching within a trampoline call) from a bank */
  Banking_CallTemplateProc makeTrampolineCallCode;
    /** template routine for constructing a concrete trampoline call
        code */
  Banking_SurrogateNameProc makeSurrogateSymbolName;
    /** template routine for constructing a concrete trampoline
        surrogate symbol name from a symbol name */
  Banking_TargetValidationProc ensureAsCallTarget;
    /** template routine for checking that some symbol is a valid
	target for an interbank call (i.e. it has to lie in a code
	segment) */
  UINT8 offsetPerTrampolineCall;
    /** number of code bytes used for the trampoline call (which is
        fixed, because no code relaxation will occur) */
} Banking_Configuration;

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Banking_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void Banking_finalize (void);
  /** cleans up internal data structures for this module */

/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Banking_adaptAreaNameWhenBanked (in /*Module_Type*/ void *module,
				      inout String_Type *areaName);
  /** returns adapted <areaName> whenever multiple conditions hold:
      banking is active, the area name specifies the generic banked
      area and additionally <module> is banked */

/*--------------------*/

Boolean Banking_isActive (void);
  /** tells whether banking is used at all */

/*--------------------*/

Target_Bank Banking_getModuleBank (in String_Type moduleName);
  /** returns associated bank for module given by <moduleName> or
      <undefinedBank> if none exists */

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Banking_readConfigurationFile (in String_Type fileName);
  /** reads assignments of module to bank from file given by
      <fileName>; the file consists of lines of assignments
      "modulename = bank" */

/*--------------------*/

Boolean Banking_resolveInterbankReferences (inout StringList_Type *fileList);
  /** traverses symbol list for interbank references; if such are
      found, a temporary object file is generated containing the
      trampoline code and its name is added to <fileList>; returns
      false when no interbank reference has occured */
    
#endif /* __BANKING_H */
