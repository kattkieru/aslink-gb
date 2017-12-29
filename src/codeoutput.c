/** CodeOutput module --
    Implementation of module providing all services for putting out
    code sequences to file in the generic SDCC linker and some
    standard implementations for this service (like putting out Intel
    Hex format).

    Original version by Thomas Tensi, 2007-02
    based on the module lkihx.c by Alan R. Baldwin

    NOTE: as a naming convention all file scope names have the module
    name as a prefix with a single underscore for externally visible
    names and two underscores for internal names
*/

#include "codeoutput.h"

/*========================================*/

#include "codesequence.h"
#include "error.h"
#include "file.h"
#include "globdefs.h"
#include "list.h"
#include "stringlist.h"

/*========================================*/

typedef struct {
  Boolean isInUse;
  File_Type file;
  String_Type fileName;
  CodeOutput_Proc outputProc;
} CodeOutput__StreamDescriptor;
  /** type to store the properties of output streams with an assigned
      code output procedure */


#define CodeOutput__maxStreamCount 10
  /** at most 10 different code output streams may be open
      simultaneously */

static CodeOutput__StreamDescriptor 
  CodeOutput__streamList[CodeOutput__maxStreamCount];

static Boolean CodeOutput__targetIsBigEndian;
  /** tells whether target platform is big endian */

/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/


static UINT8 CodeOutput__checkSum (in UINT32 addressValue)
  /** returns checksum over all bytes of <addressValue> */
{
  UINT8 result = 0;

  while (addressValue > 0) {
    result = result + (UINT8) (addressValue & 0xFF);
    addressValue = addressValue / 256;
  }

  return result;
}

/*--------------------*/

void CodeOutput__writeToAllStreams (in CodeOutput_State state,
				    in CodeSequence_Type sequence)
  /** depending on <state> either puts out code sequence <sequence>
      processed by linker (when <state> is <isCode>) or the beginning
      or terminating sequence (when <state> is <atBegin> or <atEnd>)
      to all currently open code output streams; when <state> is
      <atEnd> also the streams are closed and all descriptors are set
      to unused */
{
  UINT8 i;

  for (i = 0;  i < CodeOutput__maxStreamCount;  i++) {
    CodeOutput__StreamDescriptor *currentDescriptor;

    currentDescriptor = &(CodeOutput__streamList[i]);

    if (currentDescriptor->isInUse) {
      File_Type file;

      file = currentDescriptor->file;
      currentDescriptor->outputProc(&file, state,
				    CodeOutput__targetIsBigEndian, sequence);

      if (state == CodeOutput_State_atEnd) {
	File_close(&file);
	currentDescriptor->isInUse = false;
      }
    }
  }
}

/*========================================*/
/*            EXPORTED ROUTINES           */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void CodeOutput_initialize (in Boolean targetIsBigEndian)
{
  UINT8 i;

  CodeOutput__targetIsBigEndian = targetIsBigEndian;

  for (i = 0;  i < CodeOutput__maxStreamCount;  i++) {
    CodeOutput__StreamDescriptor *currentDescriptor;
    currentDescriptor = &(CodeOutput__streamList[i]);
    currentDescriptor->isInUse  = false;
    currentDescriptor->fileName = String_make();
  }
}

/*--------------------*/

void CodeOutput_finalize (void)
{
  UINT8 i;
  Boolean isOkay = true;

  for (i = 0;  i < CodeOutput__maxStreamCount;  i++) {
    CodeOutput__StreamDescriptor *currentDescriptor;
    currentDescriptor = &(CodeOutput__streamList[i]);
    String_destroy(&currentDescriptor->fileName);

    if (currentDescriptor->isInUse) {
      isOkay = false;
    }
  }

  if (!isOkay) {
    Error_raise(Error_Criticality_fatalError,
		"still some code output streams open when ending program");
  }
}

/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Boolean CodeOutput_create (in String_Type fileName,
			   in CodeOutput_Proc outputProc)
{
  CodeOutput__StreamDescriptor *currentDescriptor = NULL;
  File_Type file = NULL;
  UINT8 i;
  Boolean isOkay;

  isOkay = false;

  for (i = 0;  i < CodeOutput__maxStreamCount;  i++) {
    if (!CodeOutput__streamList[i].isInUse) {
      isOkay = true;
      currentDescriptor = &(CodeOutput__streamList[i]);
      break;
    }
  }

  if (isOkay) {
    /* try to open the file for writing */
    isOkay = File_open(&file, fileName, File_Mode_writeBinary);
  }

  if (isOkay) {
    CodeSequence_Type junk;

    junk.length = 0;
    currentDescriptor->isInUse    = true;
    currentDescriptor->file       = file;
    currentDescriptor->outputProc = outputProc;
    String_copy(&currentDescriptor->fileName, fileName);

    currentDescriptor->outputProc(&file, CodeOutput_State_atBegin,
				  CodeOutput__targetIsBigEndian, junk);
  }

  return isOkay;
}

/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void CodeOutput_closeStreams (void)
{
  CodeSequence_Type junk;
  junk.length = 0;
  CodeOutput__writeToAllStreams(CodeOutput_State_atEnd, junk);
}

/*--------------------*/
/* ACCESS             */
/*--------------------*/

void CodeOutput_getFileNames (out StringList_Type *fileNameList)
{
  UINT8 i;

  List_clear(fileNameList);

  for (i = 0;  i < CodeOutput__maxStreamCount;  i++) {
    CodeOutput__StreamDescriptor *currentDescriptor;

    currentDescriptor = &(CodeOutput__streamList[i]);

    if (currentDescriptor->isInUse) {
      StringList_append(fileNameList, currentDescriptor->fileName);
    }
  }
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void CodeOutput_writeLine (in CodeSequence_Type sequence)
{
  CodeOutput__writeToAllStreams(CodeOutput_State_inCode, sequence);
}

/*--------------------*/

/* Intel Hex Format has the following grammar (with hexFile as the axiom):

     checkSumField     ::= hexDigit hexDigit .
     dataField         ::= hexDigit hexDigit .
     endOfFileRecord   ::= recordMarkField '00' '0000' '01' 'FF' .
     hexDigit          ::= '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' |
                           '8' | '9' | 'A' | 'B' | 'C' | 'D' | 'E' | 'F' .
     hexFile           ::= { record newLine } endOfFileRecord newLine.
     loadAddressField  ::= hexDigit hexDigit hexDigit hexDigit .
     newLine           ::= <<newline character>> .
     record            ::= recordMarkField recordLengthField loadAddressField
                           recordTypeField { dataField } checkSumField .
     recordLengthField ::= hexDigit hexDigit .
     recordMarkField   ::= ':' .
     recordTypeField   ::= '00' .

   Note that all long words and words are specified in big endian
   order.  The checksum is the negated eight-bit sum of all previous
   byte values in the record (where the two's complement is used).
 */

void CodeOutput_writeIHXLine (inout File_Type *file, in CodeOutput_State state,
			      in Boolean isBigEndian,
			      in CodeSequence_Type sequence)
{
  UINT8 checkSum;
  UINT8 i;
  UINT8 recordType = 0;

  switch (state) {
    case CodeOutput_State_atBegin:
      break;

    case CodeOutput_State_inCode:
      File_writeCharArray(file, ":");
      File_writeHex(file, sequence.length, 2);
      File_writeHex(file, sequence.offsetAddress, 4);
      File_writeHex(file, recordType, 2);
     
      for (i = 0;  i < sequence.length;  i++) {
	File_writeHex(file, sequence.byteList[i], 2);
      }

      /* finally output checksum */
      checkSum = sequence.length;
      checkSum = checkSum + CodeOutput__checkSum(sequence.offsetAddress);
      
      for (i = 0;  i < sequence.length;  i++) {
	checkSum = checkSum + sequence.byteList[i];
      }
      
      checkSum = ((0 - checkSum) & 0xFF);
      File_writeHex(file, checkSum, 2);
      File_writeChar(file, '\n');
      break;

    case CodeOutput_State_atEnd:
      File_writeCharArray(file, ":00000001FF\n");
      break;

    default:
      Error_raise(Error_Criticality_fatalError, 
		  "unknown state in CodeOutput_writeIHXLine");
  }
}

/*--------------------*/

/* Motorola S19 Format has the following grammar (with s19File as the axiom):

     checkSumField     ::= hexDigit hexDigit .
     dataField         ::= hexDigit hexDigit .
     endOfFileRecord   ::= 'S9' '03' '0000' 'FC' .
     hexDigit          ::= '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' |
                           '8' | '9' | 'A' | 'B' | 'C' | 'D' | 'E' | 'F' .
     s19File           ::= { record newLine } endOfFileRecord newLine.
     loadAddressField  ::= hexDigit hexDigit .
     newLine           ::= <<newline character>> .
     record            ::= recordMarkField recordLengthField loadAddressField
                           { dataField } checkSumField .
     recordLengthField ::= hexDigit hexDigit .
     recordMarkField   ::= 'S1' .

   Note that all long words and words are specified in big endian
   order.  The checksum is the negated eight-bit sum of all previous
   byte values in the record (where the one's complement is used).
 */

void CodeOutput_writeS19Line (inout File_Type *file, in CodeOutput_State state,
			      in Boolean isBigEndian,
			      in CodeSequence_Type sequence)
{
  UINT8 checkSum;
  UINT8 i;

  switch (state) {
    case CodeOutput_State_atBegin:
      break;

    case CodeOutput_State_inCode:
      File_writeCharArray(file, "S1");
      File_writeHex(file, sequence.length + 3, 2);
      File_writeHex(file, sequence.offsetAddress, 4);
     
      for (i = 0;  i < sequence.length;  i++) {
	File_writeHex(file, sequence.byteList[i], 2);
      }

      /* finally output checksum */
      checkSum = sequence.length;
      checkSum = checkSum + CodeOutput__checkSum(sequence.offsetAddress);

      for (i = 0;  i < sequence.length;  i++) {
	checkSum = checkSum + sequence.byteList[i];
      }

      checkSum = ~checkSum;
      File_writeHex(file, checkSum, 2);
      File_writeChar(file, '\n');
      break;

    case CodeOutput_State_atEnd:
      File_writeCharArray(file, "S9030000FC\n");
      break;

    default:
      Error_raise(Error_Criticality_fatalError, 
		  "unknown state in CodeOutput_writeS19Line");
  }
}

