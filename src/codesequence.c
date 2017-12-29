/** CodeSequence module --
    Implementation of module providing all services for scanning code
    and relocating it in the generic SDCC linker

    Original version by Thomas Tensi, 2006-09
    based on the module lkrloc.c by Alan R. Baldwin

    NOTE: as a naming convention all file scope names have the module
    name as a prefix with a single underscore for externally visible
    names and two underscores for internal names
*/

#include "codesequence.h"

#include "area.h"
#include "error.h"
#include "globdefs.h"
#include "mapfile.h"
#include "module.h"
#include "string.h"
#include "symbol.h"
#include "target.h"

/*========================================*/


typedef enum {
  CodeSequence__RelocError_none, CodeSequence__RelocError_uByteError,
  CodeSequence__RelocError_pcrError, CodeSequence__RelocError_pageZeroError, 
  CodeSequence__RelocError_pageError 
} CodeSequence__RelocError;
  /** kind of error which may happen during relocation */

/*--------------------*/

typedef struct {
  CodeSequence_RelocationKind relocationKind;
  Symbol_Type referencedSymbol;
  Area_Segment referencedSegment;
  Target_Address codeAddress;
  Target_Address relocationValue;
} CodeSequence__ErrorRecord;
  /** type for describing some error which has occured during
      relocation: <relocationKind> tells whether some symbol or
      segment has been referenced, <referencedSymbol> or
      <referencedSegment> tell the address of the referenced entity,
      <codeAddress> is the referenced address in the previous code
      line and <relocationValue> the calculated value for that
      relocation */

/*--------------------*/

static char *CodeSequence__errorMessageList[] = {
  "none",
  "Unsigned Byte error",
  "Byte PCR relocation error",
  "Page0 relocation error",
  "Page Mode relocation error"
};
/** table of error messages for error kinds above */

/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static UINT8 CodeSequence__leastSignificantByte (in UINT16 value);
static UINT16 CodeSequence__makeWord (in UINT8 *byteList);
static UINT8 CodeSequence__mostSignificantByte (in UINT16 value);
static void CodeSequence__resetByteFlag (out Boolean *flagList, 
					 in Boolean isLowByte);
static void CodeSequence__setCodeWordToByte (in UINT8 value,
					     inout UINT8 *byteList);

/*--------------------*/

static UINT16 CodeSequence__addByteToByte (in UINT8 value,
					   inout UINT8 *changedByte)
  /** adds <value> to single byte <changedByte> and returns result */
{
  UINT8 result = *changedByte + value;

  *changedByte = result;
  return result;
}

/*--------------------*/

static UINT16 CodeSequence__addWordToWord (in UINT16 value,
					   inout UINT8 *byteList)
  /** adds <value> to byte list starting at <byteList> and returns
      result; the endianness of the target machine is respected */
{
  UINT16 result;
  UINT8 lsb;
  UINT8 msb;

  result = value + CodeSequence__makeWord(byteList);
  lsb = CodeSequence__leastSignificantByte(result);
  msb = CodeSequence__mostSignificantByte(result);

  if (Target_info.isBigEndian) {
    byteList[0] = msb;
    byteList[1] = lsb;
  } else {
    byteList[0] = lsb;
    byteList[1] = msb;
  }

  return result;
}

/*--------------------*/

static void CodeSequence__appendInformationLine (inout String_Type *message,
						 in Module_Type module,
						 in Area_Segment segment,
						 in Target_Address offset)
  /** appends a formatted and newline-terminated line to <message>
      giving details about <module>, <segment> and <offset> */
{
  String_Type fileName = String_make();
  String_Type moduleName = String_make();
  String_Type segmentName = String_make();

  Module_getFileName(module, &fileName);
  Module_getName(module, &moduleName);
  Area_getSegmentName(segment, &segmentName);

  String_append(message, fileName);
  String_appendCharArray(message, " / ");
  String_append(message, moduleName);
  String_appendCharArray(message, " / ");
  String_append(message, segmentName);
  String_appendCharArray(message, " / ");

  /* TODO: differentiate by radix defined in file */
  String_appendInteger (message, offset, 16);

  String_appendCharArray(message, "\n");

  String_destroy(&segmentName);
  String_destroy(&moduleName);
  String_destroy(&fileName);
}

/*--------------------*/

static UINT8 CodeSequence__leastSignificantByte (in UINT16 value)
   /** returns the least significant byte of <value> */
{
  return (UINT8)(value & 0xFF);
}

/*--------------------*/

static UINT16 CodeSequence__makeWord (in UINT8 *byteList)
  /** builds word from two consecutive bytes in <byteList> */
{
  UINT16 result;

  if (Target_info.isBigEndian) {
    result = byteList[0] * 256 + byteList[1];
  } else {
    result =  byteList[1] * 256 + byteList[0];
  }

  return result;
}

/*--------------------*/

static UINT8 CodeSequence__mostSignificantByte (in UINT16 value)
   /** returns the most significant byte of <value> */
{
  return (UINT8)(value / 256);
}

/*--------------------*/

static void CodeSequence__processOneRelocation (
				 in Target_Address codeSequenceBaseAddress,
				 in UINT16 *offsetByRelaxation,
				 in CodeSequence_Relocation relocation,
				 inout UINT8 *byteList, 
				 inout Boolean *isSignificantList,
				 out CodeSequence__RelocError *errorKind,
				 out CodeSequence__ErrorRecord *errorRecord)
  /** performs single <relocation> on code in <byteList>;
      <codeSequenceBaseAddress> gives the initial address of the code
      sequence in the current segment, <offsetByRelaxation> the
      accumulated correction offset by removals of superfluous bytes
      in the code sequence; <errorRecord> is set whenever an error
      occurs (flagged by <errorKind> != ErrorKind_none) */
{
  UINT8 infoIndex = relocation.index;
  CodeSequence_RelocationKind kind = relocation.kind;
  Module_Type module = Module_currentModule();
  Target_Address pagedArea = 0;
  Target_Address pagedSegment = 0;
  Area_Segment referencedSegment = NULL;
  Symbol_Type referencedSymbol = NULL;
  Target_Address relocatedAddress;
  UINT16 relocatedValue;

  UINT8 *affectedCodeByte = &(byteList[infoIndex]);

  /* process symbol or area reference */
  if (kind.isSymbol) {
    /* add 1 to take care that indexing starts with 1 */
    referencedSymbol = Module_getSymbol(module, relocation.value + 1);

    if (referencedSymbol != NULL) {
      relocatedAddress = Symbol_absoluteAddress(referencedSymbol);
    } else {
      Error_raise(Error_Criticality_warning, "R symbol error");
      return;
    }
  } else {
    /* add 1 for the absolute segment and one to take care that
       indexing starts with 1 */
    referencedSegment = Module_getSegment(module, relocation.value + 2);

    if (referencedSegment != NULL) {
      relocatedAddress = Area_getSegmentAddress(referencedSegment);
    } else {
      Error_raise(Error_Criticality_warning, "R area error");
      return;
    }
  }

  /* process PC relative addressing */
  if (kind.isRelocatedPCRelative) {
    /* TODO: pc relative addressing here*/
    Target_Address currentAddress = 0;
    /* pc = adw_w(a[aindex]->a_addr, 0); */

    relocatedAddress -= currentAddress + (infoIndex - *offsetByRelaxation);

    if (kind.elementsAreBytes) {
      relocatedAddress -= 1;
    } else {
      relocatedAddress -= 2;
    }
  }

  /* zero-page or general paged addressing */
  if (kind.pageIsReferenced || kind.zeroPageIsReferenced) {
    /* TODO: page references */
    /* pagedArea  = sdp.s_area->a_addr;
       pagedSegment  = sdp.s_addr;
       relocatedAddress -= pagedArea + pagedSegment; */
    Error_raise(Error_Criticality_fatalError, 
		"paged addressing not yet supported");
  }

  /* byte-wide or word-wide addressing */
  if (kind.elementsAreBytes) {
    if (!kind.slotWidthIsTwo) {
      relocatedValue = CodeSequence__addByteToByte((UINT8) relocatedAddress,
						   affectedCodeByte);
    } else {
      Boolean *affectedFlag = &(isSignificantList[infoIndex]);

      relocatedValue = CodeSequence__addWordToWord(relocatedAddress,
						   affectedCodeByte);

      if (kind.mostSignificantByteIsUsed) {
	CodeSequence__resetByteFlag(affectedFlag, true);
	relocatedValue = (relocatedValue >> 8);
      } else {
	CodeSequence__resetByteFlag(affectedFlag, false);
      }

      relocatedValue = (relocatedValue & 0xFF);
    }
  } else { /* relocated elements are words */
    /* a combination of word elements and most significant byte use
      is flagged as an 'r' error by the assembler, but it is
      processed here anyway. ??? WHY ??? */

    relocatedValue = CodeSequence__addWordToWord(relocatedAddress,
						 affectedCodeByte);

    if (kind.slotWidthIsTwo) {
      UINT8 byteValue;

      byteValue = (kind.mostSignificantByteIsUsed
		   ? CodeSequence__mostSignificantByte(relocatedValue)
		   : CodeSequence__leastSignificantByte(relocatedValue));

      CodeSequence__setCodeWordToByte(byteValue, affectedCodeByte);
    }
  }

  /* increment <offsetByRelaxation> whenever a byte relocation has
     happened with a width of 2 */
  if (kind.elementsAreBytes && kind.slotWidthIsTwo) {
    *offsetByRelaxation += 1;
  }

  /* check whether some relocation problem has happened and set
     <errorKind> and <errorRecord> accordingly */
  {
    Boolean msbIsNonzero = 
      (CodeSequence__mostSignificantByte(relocatedValue) != 0);
    *errorKind = CodeSequence__RelocError_none;

    if (!kind.dataIsSigned && kind.elementsAreBytes && msbIsNonzero) {
      *errorKind = CodeSequence__RelocError_uByteError;
    } else if (kind.isRelocatedPCRelative && kind.elementsAreBytes) {
      Target_Address r = relocatedValue & ~0x7F;
      if (r != (Target_Address) ~0x7F && r != 0) {
	*errorKind = CodeSequence__RelocError_pcrError;
      }
    } else if (kind.zeroPageIsReferenced
       && (msbIsNonzero || pagedArea != 0 || pagedSegment != 0)) {
      *errorKind = CodeSequence__RelocError_pageZeroError;
    } else if (kind.pageIsReferenced && msbIsNonzero) {
      *errorKind = CodeSequence__RelocError_pageError;
    }

    if (*errorKind != CodeSequence__RelocError_none) {
      errorRecord->relocationKind    = kind;
      errorRecord->referencedSegment = referencedSegment;
      errorRecord->referencedSymbol  = referencedSymbol;
      errorRecord->codeAddress       =
	codeSequenceBaseAddress + (infoIndex - *offsetByRelaxation) - 1;
      errorRecord->relocationValue   = relocation.value;
    }
  }
}

/*--------------------*/

static void CodeSequence__reportRelocationError (
			 in CodeSequence__RelocError errorKind,
			 in CodeSequence__ErrorRecord errorRecord)
  /** reports some relocation error found onto stderr and - when
      available - on the current map file; kind is given by
      <errorKind>, detailed information in <errorRecord> */
{
  if (errorKind != CodeSequence__RelocError_none) {
    String_Type errorMessage = String_make();
    Target_Address offset;
    Area_Segment segment;

    String_appendCharArray(&errorMessage,
				CodeSequence__errorMessageList[errorKind]);

    /* print symbol information in warning line (if applicable) */
    if (errorRecord.relocationKind.isSymbol) {
      String_Type st = String_make();
      String_appendCharArray(&errorMessage, " for symbol ");
      Symbol_getName(errorRecord.referencedSymbol, &st);
      String_append(&errorMessage, st);
      String_destroy(&st);
    }

    String_appendCharArray(&errorMessage, "\n");

    /* print header */
    String_appendCharArray(&errorMessage, 
			   "         file / module / area / offset\n");

    /* print information about referencing (current!) segment */
    String_appendCharArray(&errorMessage, "  Refby  ");
    CodeSequence__appendInformationLine(&errorMessage, Module_currentModule(),
					Area_currentSegment(),
					errorRecord.codeAddress);

    /* print information about definition and defining segment */
    if (errorRecord.relocationKind.isSymbol) {
      segment = Symbol_getSegment(errorRecord.referencedSymbol);
      offset  = Symbol_absoluteAddress(errorRecord.referencedSymbol);
    } else {
      segment = errorRecord.referencedSegment;
      offset  = errorRecord.relocationValue;
    }

    String_appendCharArray(&errorMessage, "  Defin  ");
    CodeSequence__appendInformationLine(&errorMessage,
					Area_getSegmentModule(segment),
					segment, offset);

    Error_raise(Error_Criticality_warning, 
		"%s", String_asCharPointer(errorMessage));

    /* also put error information to mapfile (if available) */
    if (MapFile_isOpen()) {
      MapFile_writeErrorMessage(errorMessage);
    }

    String_destroy(&errorMessage);
  }
}

/*--------------------*/

static void CodeSequence__resetByteFlag (out Boolean *flagList, 
					 in Boolean isLowByte)
  /** resets flag in <flagList> representing low or high byte;
      <isLowByte> tells the kind of byte to be reset where the
      endianness of the target has to be taken into account */
{
  UINT8 index;

  index = (isLowByte ? 0 : 1);
  
  if (Target_info.isBigEndian) {
    index = 1 - index;
  }

  flagList[index] = false;
}

/*--------------------*/

static void CodeSequence__setCodeWordToByte (in UINT8 value,
					     inout UINT8 *byteList)
  /** sets code word pointed to by <byteList> to <value> and clears
      msb (depending on endianness) */
{
  if (Target_info.isBigEndian) {
    byteList[0] = 0;
    byteList[1] = value;
  } else {
    byteList[0] = value;
    byteList[1] = 0;
  }
}

/*========================================*/
/*            EXPORTED ROUTINES           */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void CodeSequence_initialize (void)
{
}

/*--------------------*/

void CodeSequence_finalize (void)
{
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

void CodeSequence_makeKindFromInteger (out CodeSequence_RelocationKind *kind,
				       in UINT8 value)
{
  kind->mostSignificantByteIsUsed = ((value & 128) != 0);
  kind->pageIsReferenced          = ((value &  64) != 0);
  kind->zeroPageIsReferenced      = ((value &  32) != 0);
  kind->dataIsSigned              = ((value &  16) != 0);
  kind->slotWidthIsTwo            = ((value &   8) != 0);
  kind->isRelocatedPCRelative     = ((value &   4) != 0);
  kind->isSymbol                  = ((value &   2) != 0);
  kind->elementsAreBytes          = ((value &   1) != 0);
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void CodeSequence_relocate (inout CodeSequence_Type *sequence,
			    in UINT16 areaMode,
			    in CodeSequence_RelocationList *relocationList)
{
  Target_Address currentAddress;
  UINT16 i;
  Boolean isSignificantList[CodeSequence_maxLength];
  UINT16 offsetByRelaxation;  /* number of code bytes saved because
				 of smaller operands than expected */
  Area_Segment segment;
  String_Type segmentName = String_make();

  /* verify area mode */
  if (areaMode != 0) {
    Error_raise(Error_Criticality_fatalError, "bad area mode in R line");
  }

  /* check segment */
  segment = relocationList->segment;    

  if (segment == NULL) {
    Error_raise(Error_Criticality_fatalError, "bad segment in R line");
  }

  currentAddress = Area_getSegmentAddress(segment);
  Area_getSegmentName(segment, &segmentName);

  sequence->offsetAddress += currentAddress;
  offsetByRelaxation = 0;

  if (Target_info.getBankFromSegmentName == NULL) {
    sequence->romBank = 0;
  } else {
    sequence->romBank = Target_info.getBankFromSegmentName(segmentName);
  }

  /* set all code sequence bytes to 'significant' */
  for (i = 0;  i < sequence->length;  i++) {
    isSignificantList[i] = true;
  }

  /* loop over all relocations */
  for (i = 0;  i < relocationList->count;  i++) {
    CodeSequence__RelocError errorKind;
    CodeSequence__ErrorRecord errorRecord;
    
    CodeSequence__processOneRelocation(sequence->offsetAddress,
				       &offsetByRelaxation,
				       relocationList->list[i],
				       sequence->byteList,
				       isSignificantList,
				       &errorKind, &errorRecord);

    if (errorKind != CodeSequence__RelocError_none) {
      CodeSequence__reportRelocationError(errorKind, errorRecord);
    }
  }

  /* condense code sequence by relaxation i.e. throwing out all
     insignificant code bytes  */
  {
    UINT8 *byteList = sequence->byteList;
    UINT8 j = 0;
    UINT8 newSequenceLength = sequence->length;
    
    for (i = 0;  i < sequence->length;  i++) {
      if (!isSignificantList[i]) {
	newSequenceLength--;
      } else {
	if (i != j) {
	  byteList[j] = byteList[i];
	}
	j++;
      }
    }
    
    sequence->length = newSequenceLength;
  }
}


/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

UINT8 CodeSequence_convertToInteger (in CodeSequence_RelocationKind kind)
  /** converts relocation kind <kind> to external integer
      representation */
{
  UINT8 result = 0;

  result += (kind.mostSignificantByteIsUsed ? 128 : 0);
  result += (kind.pageIsReferenced          ?  64 : 0);
  result += (kind.zeroPageIsReferenced      ?  32 : 0);
  result += (kind.dataIsSigned              ?  16 : 0);
  result += (kind.slotWidthIsTwo            ?   8 : 0);
  result += (kind.isRelocatedPCRelative     ?   4 : 0);
  result += (kind.isSymbol                  ?   2 : 0);
  result += (kind.elementsAreBytes          ?   1 : 0);

  if ((result & 0x74) != 0) {
    result = result;
  }

  return result;
}


/*--------------------*/
// TODO: paged relocation and paged relocation error

/* /\*)Function	VOID	relp() */
/*  * */
/*  *	The function relp() evaluates a P line read by */
/*  *	the linker.  The P line data is combined with the */
/*  *	previous T line data to set the base page address */
/*  *	and test the paging boundary and length. */
/*  * */
/*  *		P Line  */
/*  * */
/*  *		P 0 0 nn nn n1 n2 xx xx  */
/*  * */
/*  * 	The  P  line provides the paging information to the linker as */
/*  *	specified by a .setdp directive.  The format of  the  relocation */
/*  *	information is identical to that of the R line.  The correspond- */
/*  *	ing T line has the following information:   */
/*  *		T xx xx aa aa bb bb  */
/*  * */
/*  * 	Where  aa aa is the area reference number which specifies the */
/*  *	selected page area and bb bb is the base address  of  the  page. */
/*  *	bb bb will require relocation processing if the 'n1 n2 xx xx' is */
/*  *	specified in the P line.  The linker will verify that  the  base */
/*  *	address is on a 256 byte boundary and that the page length of an */
/*  *	area defined with the PAG type is not larger than 256 bytes.   */
/*  * */
/*  *	local variable: */
/*  *		areax	**a		pointer to array of area pointers */
/*  *		int	aindex		area index */
/*  *		int	mode		relocation mode */
/*  *		Target_Address	relv		relocation value */
/*  *		int	rindex		symbol / area index */
/*  *		int	rtp		index into T data */
/*  *		sym	**s		pointer to array of symbol pointers */
/*  * */
/*  *	global variables: */
/*  *		head	*hp		pointer to the head structure */
/*  *		int	lkerr		error flag */
/*  *		sdp	sdp		base page structure */
/*  *		FILE	*stderr		standard error device */
/*  * */
/*  *	called functions: */
/*  *		Target_Address	CodeSequence__addWordToWord()		lkrloc.c */
/*  *		Target_Address	evword()	lkrloc.c */
/*  *		int	eval()		lkeval.c */
/*  *		int	fprintf()	c_library */
/*  *		int	more()		lklex.c */
/*  *		int	Symbol_absoluteAddress()	lksym.c */
/*  * */
/*  *	side effects: */
/*  *		The P and T lines are combined to set */
/*  *		the base page address and report any */
/*  *		paging errors. */
/*  * */
/*  *\/ */

/* VOID */
/* relp() */
/* { */
/* 	register int aindex, rindex; */
/* 	int mode, rtp; */
/* 	Target_Address relv; */
/* 	struct areax **a; */
/* 	struct sym **s; */

/* 	/\* */
/* 	 * Get area and symbol lists */
/* 	 *\/ */
/* 	a = hp->a_list; */
/* 	s = hp->s_list; */

/* 	/\* */
/* 	 * Verify Area Mode */
/* 	 *\/ */
/* 	if (eval() != (R_WORD | R_AREA) || eval()) { */
/* 		fprintf(stderr, "P input error\n"); */
/* 		lkerr++; */
/* 	} */

/* 	/\* */
/* 	 * Get area pointer */
/* 	 *\/ */
/* 	aindex = evword(); */
/* 	if (aindex >= hp->h_narea) { */
/* 		fprintf(stderr, "P area error\n"); */
/* 		lkerr++; */
/* 		return; */
/* 	} */

/* 	/\* */
/* 	 * Do remaining relocations */
/* 	 *\/ */
/* 	while (more()) { */
/* 		mode = eval(); */
/* 		rtp = eval(); */
/* 		rindex = evword(); */

/* 		/\* */
/* 		 * R_SYM or R_AREA references */
/* 		 *\/ */
/* 		if (mode & R_SYM) { */
/* 			if (rindex >= hp->h_nglob) { */
/* 				fprintf(stderr, "P symbol error\n"); */
/* 				lkerr++; */
/* 				return; */
/* 			} */
/* 			relv = Symbol_absoluteAddress(s[rindex]); */
/* 		} else { */
/* 			if (rindex >= hp->h_narea) { */
/* 				fprintf(stderr, "P area error\n"); */
/* 				lkerr++; */
/* 				return; */
/* 			} */
/* 			relv = a[rindex]->a_addr; */
/* 		} */
/* 		CodeSequence__addWordToWord(relv, rtp); */
/* 	} */

/* 	/\* */
/* 	 * Paged values */
/* 	 *\/ */
/* 	aindex = CodeSequence__addWordToWord(0,2); */
/* 	if (aindex >= hp->h_narea) { */
/* 		fprintf(stderr, "P area error\n"); */
/* 		lkerr++; */
/* 		return; */
/* 	} */
/* 	sdp.s_areax = a[aindex]; */
/* 	sdp.s_area = sdp.s_areax->a_bap; */
/* 	sdp.s_addr = CodeSequence__addWordToWord(0,4); */
/* 	if (sdp.s_area->a_addr & 0xFF || sdp.s_addr & 0xFF) */
/* 		relerp("Page Definition Boundary Error"); */
/* } */

/* /\*)Function	VOID	relerp(str) */
/*  * */
/*  *		char	*str		error string */
/*  * */
/*  *	The function relerp() outputs the paging error string to */
/*  *	stderr and to the map file (if it is open). */
/*  * */
/*  *	local variable: */
/*  *		none */
/*  * */
/*  *	global variables: */
/*  *		FILE	*mfp		handle for the map file */
/*  * */
/*  *	called functions: */
/*  *		VOID	erpdmp()	lkrloc.c */
/*  * */
/*  *	side effects: */
/*  *		Error message inserted into map file. */
/*  * */
/*  *\/ */

/* VOID */
/* relerp(str) */
/* char *str; */
/* { */
/* 	erpdmp(stderr, str); */
/* 	if (mfp) */
/* 		erpdmp(mfp, str); */
/* } */

/* /\*)Function	VOID	erpdmp(fptr, str) */
/*  * */
/*  *		FILE	*fptr		output file handle */
/*  *		char	*str		error string */
/*  * */
/*  *	The function erpdmp() outputs the error string str */
/*  *	to the device specified by fptr. */
/*  * */
/*  *	local variable: */
/*  *		head	*thp		pointer to head structure */
/*  * */
/*  *	global variables: */
/*  *		int	lkerr		error flag */
/*  *		sdp	sdp		base page structure */
/*  * */
/*  *	called functions: */
/*  *		int	fprintf()	c_library */
/*  *		VOID	prntval()	lkrloc.c */
/*  * */
/*  *	side effects: */
/*  *		Error reported. */
/*  * */
/*  *\/ */

/* VOID */
/* erpdmp(fptr, str) */
/* FILE *fptr; */
/* char *str; */
/* { */
/* 	register struct head *thp; */

/* 	thp = sdp.s_areax->a_bhp; */

/* 	/\* */
/* 	 * Print Error */
/* 	 *\/ */
/* 	fprintf(fptr, "\n?ASlink-Warning-%s\n", str); */
/* 	lkerr++; */

/* 	/\* */
/* 	 * Print PgDef Info */
/* 	 *\/ */
/* 	fprintf(fptr, */
/* 		"         file        module      pgarea      pgoffset\n"); */
/* 	fprintf(fptr, */
/* 		"  PgDef  %-8.8s    %-8.8s    %-8.8s    ", */
/* 			thp->h_lfile->f_idp, */
/* 			&thp->m_id[0], */
/* 			&sdp.s_area->a_id[0]); */
/* 	prntval(fptr, sdp.s_area->a_addr + sdp.s_addr); */
/* } */
