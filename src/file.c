/** File module --
    Implementation of module providing services for handling files

    Original version by Thomas Tensi, 2007-09
*/

#include "file.h"

/*========================================*/

#include <ctype.h>
# define CType_isdigit isdigit
#include <stdarg.h>
#  define StdArg_VarArgList va_list
#include <stdio.h>
typedef FILE *StdIO_File;
# define StdIO_endOfFile EOF
# define StdIO_fclose    fclose
# define StdIO_fopen     fopen
# define StdIO_fprintf   fprintf
# define StdIO_fseek     fseek
# define StdIO_fwrite    fwrite
# define StdIO_stderr    stderr
# define StdIO_seekSet   SEEK_SET
# define StdIO_vfprintf  vfprintf
#include <stdlib.h>
#  define StdLib_strtol  strtol
#include <string.h>
# define STRING_isEqual(a,b) (strcmp(a,b) == 0)
# define STRING_length       strlen

#include "globdefs.h"
#include "string.h"

/*========================================*/

#define File__magicNumber 0x99998888

/*--------------------*/

typedef struct File__Record {
  UINT32 magicNumber;
  StdIO_File filePointer;
} File__Record;
  /** file type with a pointer to a Standard IO file (this type
      information should only be used internally!) */

/*--------------------*/

File_Type File_stderr;
  /** standard error file stream */

String_Type File_directorySeparator;
  /** string to separate parts of a directory specification; "/" in
      Unix, "\" in Windows */

/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static Boolean File__isValid (in Object file)
{
  return isValidObject(file, File__magicNumber);
}

/*--------------------*/

static Boolean File__checkValidityPRE (in Object file,
				       in char *procName)
  /** checks as a precondition of routine <procName> whether <file> is
      a valid file and returns the check result */
{
  return PRE(File__isValid(file), procName, "invalid file");
}

/*--------------------*/
/*--------------------*/

void File__findOffset (inout char *name, out SizeType *offset)
    /** checks whether this is a file with offset: if yes, find offset
       string and remove it from plain file name */
{
  /* a file with offset must have trailing digits after a
     <File_offsetSeparator> character */
  char *ptr = name + STRING_length(name) - 1;
  Boolean inDigits = true;

  *offset = 0;

  while (ptr > name && inDigits) {
    inDigits = CType_isdigit(*ptr--);
  }

  if (ptr > name && *ptr == File_offsetSeparator) {
    /* we found a correct offset specification ==> get offset and
       remove specification from name */
    char *endPtr;
    int base = 10;
    *ptr = String_terminator;
    ptr++;
    *offset = StdLib_strtol(ptr, &endPtr, base);
  }
}

/*--------------------*/

static File_Type File__make (in StdIO_File filePointer)
{
  File_Type file = NEW(File__Record);
  file->magicNumber = File__magicNumber;
  file->filePointer = filePointer;
  return file;
}

/*========================================*/
/*            EXPORTED ROUTINES           */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void File_initialize (void)
{
  File_stderr = File__make(StdIO_stderr);
  File_directorySeparator = String_makeFromCharArray("\\");
}

/*--------------------*/

void File_finalize (void)
{
  File_close(&File_stderr);
  String_destroy(&File_directorySeparator);
}

/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Boolean File_open (out File_Type *result,
		   in String_Type fileName, in File_Mode mode)
{
  char *name = String_asCharPointer(fileName);
  StdIO_File filePointer;

  if (STRING_isEqual(name, "stderr")) {
    *result = File_stderr;
    filePointer = StdIO_stderr;
  } else {
    char *openMode = "";
    SizeType offset;

    if (mode == File_Mode_read) {
      openMode = "r";
    } else if (mode == File_Mode_readBinary) {
      openMode = "rb";
    } else if (mode == File_Mode_write) {
      openMode = "w";
    } else if (mode == File_Mode_writeBinary) {
      openMode = "wb";
    }

    File__findOffset(name, &offset);
    filePointer = StdIO_fopen(name, openMode);

    if (filePointer != NULL) {
      *result = File__make(filePointer);

      if (offset != 0) {
	StdIO_fseek(filePointer, offset, StdIO_seekSet);
      }
    }
  }

  return (filePointer != NULL);
}

/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void File_close (inout File_Type *file)
{
  char *procName = "File_close";
  File_Type currentFile = *file;
  Boolean precondition = File__checkValidityPRE(currentFile, procName);

  if (precondition) {  
    StdIO_fclose(currentFile->filePointer);
    DESTROY(currentFile);
    *file = NULL;
  }
}

/*--------------------*/
/* ACCESS             */
/*--------------------*/

Boolean File_exists (in String_Type fileName)
{
  File_Type file;
  Boolean isFound = File_open(&file, fileName, File_Mode_read);

  if (isFound) {
    File_close(&file);
  }

  return isFound;
}

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void File_readLine (inout File_Type *file, out String_Type *st)
{
  char *procName = "File_readLine";
  File_Type currentFile = *file;
  Boolean precondition = File__checkValidityPRE(currentFile, procName);

  if (precondition) {  
    int ch;

    String_clear(st);

    do {
      ch = fgetc(currentFile->filePointer);

      if (ch != StdIO_endOfFile) {
	String_appendChar(st, (char) ch);
      }
    } while (ch != StdIO_endOfFile && ch != '\n');
  }
}

/*--------------------*/

void File_writeBytes (inout File_Type *file, in UINT8 *data, in SizeType size)
{
  char *procName = "File_writeBytes";
  File_Type currentFile = *file;
  Boolean precondition = File__checkValidityPRE(currentFile, procName);

  if (precondition) {
    StdIO_fwrite(data, size, 1, currentFile->filePointer);
  }
}

/*--------------------*/

void File_writeChar (inout File_Type *file, in char ch)
{
  char *procName = "File_writeChar";
  File_Type currentFile = *file;
  Boolean precondition = File__checkValidityPRE(currentFile, procName);

  if (precondition) {
    StdIO_fprintf(currentFile->filePointer, "%c", ch);
  }
}

/*--------------------*/

void File_writeCharArray (inout File_Type *file, in char *st)
{
  char *procName = "File_writeCharArray";
  File_Type currentFile = *file;
  Boolean precondition = File__checkValidityPRE(currentFile, procName);

  if (precondition) {
    StdIO_fprintf(currentFile->filePointer, "%s", st);
  }
}

/*--------------------*/

void File_writeHex (inout File_Type *file,
		    in UINT32 value, in UINT8 digitCount)
{
  char *procName = "File_writeHex";
  File_Type currentFile = *file;
  Boolean precondition = File__checkValidityPRE(currentFile, procName);

  if (precondition) {
    StdIO_fprintf(currentFile->filePointer, "%.*x", digitCount, value);
  }
}

/*--------------------*/

void File_writePrintfArguments (inout File_Type *file, in char *format,
				in StdArg_VarArgList argumentList)
{
  char *procName = "File_writePrintfArguments";
  File_Type currentFile = *file;
  Boolean precondition = File__checkValidityPRE(currentFile, procName);

  if (precondition) {
    StdIO_vfprintf(currentFile->filePointer, format, argumentList);
  }
}

/*--------------------*/

void File_writeString (inout File_Type *file, in String_Type st)
{
  char *procName = "File_writeString";
  File_Type currentFile = *file;
  Boolean precondition = File__checkValidityPRE(currentFile, procName);

  if (precondition) {
    StdIO_fprintf(currentFile->filePointer, "%s", String_asCharPointer(st));
  }
}
