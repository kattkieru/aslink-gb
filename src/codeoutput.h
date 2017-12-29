/** CodeOutput module --
    This module provides a generic service for putting out code
    sequences to file in the generic SDCC linker and some standard
    implementations for this service (like putting out Intel Hex
    format).

    There are several code output streams available which have to be
    activated and get some code output routine assigned.  Other
    modules feed code sequences into the CodeOutput module and it
    dispatches them to all listening code output streams.

    For convenience the code output routines for Intel Hex and
    Motorola S19 format are provided.  When those formats are not okay
    for some platform an own routine must be provided in the platform
    specific module and registered upon startup.  An example can be
    found for the Gameboy platform which uses some simple binary
    memory dump format.

    Original version by Thomas Tensi, 2007-02
    based on the module lkihx.c by Alan R. Baldwin
*/

#ifndef __CODEOUTPUT_H
#define __CODEOUTPUT_H

/*========================================*/

#include "globdefs.h"
#include "codesequence.h"
#include "file.h"
#include "string.h"
#include "stringlist.h"

/*========================================*/

typedef enum {
  CodeOutput_State_atBegin, CodeOutput_State_inCode, CodeOutput_State_atEnd
} CodeOutput_State;
   /** state where an output proc may be called: <atBegin> is for any
       processing before output of the first code sequence, <inCode>
       is when putting out some intermediate code line and <atEnd> is
       for putting out the final record */


typedef void (*CodeOutput_Proc)(inout File_Type *file,
				in CodeOutput_State state,
				in Boolean isBigEndian,
				in CodeSequence_Type sequence);
  /** type representing a routine to put out a code sequence processed
      by linker; <file> is the file descriptor of the executable file,
      <state> tells whether the processing is started, in code
      processing or done, <isBigEndian> tells the endianness of the
      target platform and <sequence> is the code sequence to be put
      out (when state is <inCode>) */


/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void CodeOutput_initialize (in Boolean targetIsBigEndian);
  /** initializes internal data structures; <targetIsBigEndian> tells
      about the endianness of the target platform */

/*--------------------*/

void CodeOutput_finalize (void);
  /** cleans up internal data structures */


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Boolean CodeOutput_create (in String_Type fileName,
			   in CodeOutput_Proc outputProc);
  /** creates another code output stream on file with <filename> with
      a routine formatting the code sequences <outputProc>; when
      opening the file for writing fails, the routine returns false */


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void CodeOutput_closeStreams (void);
  /** puts the terminating record to all open code output streams and
      additionally closes all code stream files */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void CodeOutput_getFileNames (out StringList_Type *fileNameList);
  /** returns list of file names for all registered output streams */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void CodeOutput_writeLine (in CodeSequence_Type sequence);
  /** puts the representation of code sequence <sequence> to all open
      code output streams */

/*--------------------*/

void CodeOutput_writeIHXLine (inout File_Type *file, in CodeOutput_State state,
			      in Boolean isBigEndian,
			      in CodeSequence_Type sequence);
  /** predefined code output routine producing Intel Hex format */

/*--------------------*/

void CodeOutput_writeS19Line (inout File_Type *file, in CodeOutput_State state,
			      in Boolean isBigEndian,
			      in CodeSequence_Type sequence);
  /** predefined code output routine producing Motorola S19 format */

#endif /* __CODEOUTPUT_H */
