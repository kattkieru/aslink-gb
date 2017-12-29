/** Library module --
    This module provides all services for object file libraries in the
    generic SDCC linker.

    Object file libraries are searched for in directories given by the
    (inbound) routine <addDirectory>.  The names of those libraries
    are also specified by the routine <addFilePathName>.

    A call to the central routine <resolveUndefinedSymbols> searches
    all matching files for symbol definitions.  Those definitions are
    used to satisfy unresolved references from the object modules
    linked so far.  This process is repeated until no more resolutions
    can be done.  All encountered symbols are added to the linker
    symbol table and the associated code from the libraries can be
    added to the code later.

    Note that SDCCLIBs with an XML-structure are not yet supported.

    Original version by Thomas Tensi, 2008-01
    based on the module lklibr.c by Alan R. Baldwin
*/


#ifndef __LIBRARY_H
#define __LIBRARY_H

/*--------------------*/

#include "globdefs.h"
#include "string.h"
#include "stringlist.h"

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Library_initialize (void);
  /** initializes the internal data structures of the library
      manager */

/*--------------------*/

void Library_finalize (void);
  /** cleans up the internal data structures of the library manager */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Library_getFileNameList (out StringList_Type *libraryFileNameList);
  /** returns list of library object files used so far in
      <libraryFileNameList> */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Library_addCodeSequences (void);
  /** adds code defined in all referenced library object files */

/*--------------------*/

void Library_addDirectory (in String_Type path);
  /** adds some directory <path> to the list of paths */

/*--------------------*/

void Library_addFilePathName (in String_Type path, out Boolean *isFound);
  /** adds some library file relative or absolute <path>; returns in
      <isFound> whether library has been found in directory path list
      or not */


/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

void Library_resolveUndefinedSymbols (void);
  /** searches all specified library files and library directories for
      undefined symbols until no more resolutions can be done; adds
      newly referenced symbols to symbol table and keeps track of
      all used library files */

#endif /* __LIBRARY_H */
