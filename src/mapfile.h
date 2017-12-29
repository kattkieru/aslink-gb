/** MapFile module --
    This module provides all services for putting out mapfiles.  Those
    map files give an overview about the object files read, the
    allocation of symbols and areas and the library files used.

    Several map files can be open at once and they may have different
    output routines.  The map target files and the routines have to be
    registered in this module and they are automatically activated
    when all map information is available.  The standard map file is
    produced via the output routine <generateStandardFile>, but there
    are also variants possible.  One can be found in the Gameboy
    module, where a map file for the NoGMB emulator is produced.

    Original version by Thomas Tensi, 2008-01
*/

#ifndef __MAPFILE_H
#define __MAPFILE_H

/*========================================*/

#include "area.h"
#include "file.h"
#include "globdefs.h"
#include "string.h"

/*========================================*/

typedef void (*MapFile_CommentOutputProc)(inout File_Type *file,
					  in String_Type comment);
  /** type representing a routine to conditionally add text from magic
      comments to some map file */

/*--------------------*/

typedef void (*MapFile_SymbolTableOutputProc)(inout File_Type *file);
  /** type representing a routine to produce a map file of the linker
      output; <file> is the file descriptor of the map file which is
      already open and will be closed outside that routine */

/*--------------------*/

typedef struct {
  MapFile_CommentOutputProc commentOutputProc;
  MapFile_SymbolTableOutputProc symbolTableOutputProc;
} MapFile_ProcDescriptor;

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void MapFile_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void MapFile_finalize (void);
  /** cleans up internal data structures for this module */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

Boolean MapFile_isOpen (void);
  /** tells whether map files are open or not */

/*--------------------*/

void MapFile_getSortedAreaSymbolList (in Area_Type area, 
				      out Symbol_List *areaSymbolList);
  /** collects all symbols in <area> and returns them in
      <areaSymbolList> sorted by address; this is merely a convenience
      routine to be used by mapfile generators */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void MapFile_registerForOutput (in String_Type fileNameSuffix,
				in MapFile_ProcDescriptor routines);
  /** registers <routines> for mapfile output to file with map file
      specific extension <fileNameSuffix> */

/*--------------------*/

void MapFile_openAll (in String_Type fileNamePrefix);
  /** opens all map files with names given by <fileNamePrefix> plus a
      map file specific extension */

/*--------------------*/

void MapFile_closeAll (void);
  /** closes all open map files */

/*--------------------*/

void MapFile_setOptions (in UINT8 base, in StringList_Type linkFileList);
  /** sets options for map file output to radix <base> and list of
      linked files <linkFileList> */

/*--------------------*/

void MapFile_writeErrorMessage (in String_Type message);
  /** writes <message> as warning to all currently open map files */

/*--------------------*/

void MapFile_writeSpecialComment (in String_Type comment);
  /** writes <comment> to all currently open map files if relevant for
      them */

/*--------------------*/

void MapFile_writeLinkingData (void);
  /** writes linker symbol information to all currently open map
      files */

/*--------------------*/

void MapFile_generateStandardFile (inout File_Type *file);
  /** generates canonical map file output into <file> */

#endif /* __MAPFILE_H */
