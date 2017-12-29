/** File module --
    This module provides all services for handling files in
    the generic SDCC linker.

    A file is specified by a file name which is a string in a platform
    specific notation.  The path separator is a variable that is set
    according to the local convention for path separation (a slash in
    Unix and a backslash in Windows).

    When a file name ends in an at-character followed by a decimal
    number, its relevant information is considered to start at that
    offset.  This means that 'file' and 'file@0' mean the same.

    A file may be opened in several modes where read and write as well
    as binary and text variants are distinguished.  Writing to a file
    means that all previous contents starting at the write position
    are discarded.

    Because we are only dealing with text files as input to the
    linker, there is only one read routine which returns a single line
    from a file (terminated by a newline string).  The write routines
    are type-specific; there is intentionally no explicit printf-style
    routine.  Nevertheless for use by other modules there is also a
    vararg write routine.

    After processing a file must be explicitely closed.

    Original version by Thomas Tensi, 2006-08
    based on the module lkfile.c by Alan R. Baldwin
*/

#ifndef __FILE_H
#define __FILE_H

/*--------------------*/

#include <stdarg.h>
#  define StdArg_VarArgList va_list

#include "globdefs.h"
#include "string.h"

/*========================================*/


typedef struct File__Record *File_Type;
  /** type representing a file */


typedef enum {
  File_Mode_read, File_Mode_write, File_Mode_readBinary, File_Mode_writeBinary
} File_Mode;
  /** open mode for a file; binary and text modes are distinguished */

/*--------------------*/

extern File_Type File_stderr;
  /** standard error file stream, typically routed to the console */


extern String_Type File_directorySeparator;
  /** string to separate parts of a directory specification; stands
      for "/" in Unix and "\" in Windows */


#define File_offsetSeparator '@'
  /** character to separate offset part of a file name from the plain
      file name */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void File_initialize (void);
  /** sets up internal data structures for this module */

/*--------------------*/

void File_finalize (void);
  /** cleans up internal data structures for this module */

/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Boolean File_open (out File_Type *file,
		   in String_Type fileName, in File_Mode mode);
  /** opens file given by <fileName> for reading or writing depending
      on <mode>; if successful, <file> contains the associated file,
      otherwise false is returned; note that the system supports the
      special file names "stdin", "stdout" and "stderr" which access
      the appropriate terminal streams; when a file is opened for
      writing, its previous contents are discarded (possibly in
      between when the file name contains an offset separator) */

/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void File_close (inout File_Type *file);
  /** ends processing of <file> */

/*--------------------*/
/* ACCESS             */
/*--------------------*/

Boolean File_exists (in String_Type fileName);
  /** tells whether file given by <fileName> exists */

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void File_readLine (inout File_Type *file,  out String_Type *st);
  /** returns next line on <file> in <st> including a final newline
      character; when file is exhausted, <st> is empty */

/*--------------------*/

void File_writeBytes (inout File_Type *file, in UINT8 *data, in SizeType size);
  /** puts byte array <data> with length <size> to <file> */

/*--------------------*/

void File_writeChar (inout File_Type *file, in char ch);
  /** puts character <ch> to <file> */

/*--------------------*/

void File_writeCharArray (inout File_Type *file, in char *st);
  /** puts NUL-terminated character array <st> to <file> */

/*--------------------*/

void File_writeHex (inout File_Type *file,
		    in UINT32 value, in UINT8 digitCount);
  /** puts <digitCount> least significant bytes of <value> in
      hexadecimal to <file> */

/*--------------------*/

void File_writePrintfArguments (inout File_Type *file, in char *format,
				in StdArg_VarArgList argumentList);
  /** puts all arguments in <argumentList> according to printf-style
      <format> to <file> */

/*--------------------*/

void File_writeString (inout File_Type *file, in String_Type st);
  /** puts string <st> to <file> */

#endif /* __FILE_H */
