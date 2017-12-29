/** Banking module --
    Implementation of module providing all services for code banking
    in the generic linker.

    Original version by Thomas Tensi, 2008-04
*/

#include "banking.h"

/*========================================*/

#include "area.h"
#include "error.h"
#include "file.h"
#include "globdefs.h"
#include "integermap.h"
#include "map.h"
#include "module.h"
#include "parser.h"
#include "string.h"
#include "stringlist.h"
#include "symbol.h"
#include "target.h"
#include "typedescriptor.h"

/*========================================*/

static Map_Type Banking__moduleNameToBankMap;
  /** map from module name to associated bank (filled from banking
      configuration file) */

/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static Target_Bank Banking__getBank (in Module_Type module);

/*--------------------*/

static void Banking__collectInterbankReferences (
			 out StringList_Type *jumpLabelNameList,
			 out StringList_Type *surrogateNameList,
			 out StringList_Type *symbolNameList,
			 out Map_Type *symbolIndexToLabelIndexMap,
			 inout Boolean *interbankReferenceIsFound)
  /** traverses all symbols and checks whether they are used in
      interbank references; each such symbol is split in the symbol
      table into a surrogate symbol and the original symbol, where the
      original references are redirected to the surrogate; the names
      of all those symbols are put into <symbolNameList>, the name of
      the surrogates in <surrogateNameList> and the names of the jump
      labels for bank switching in <jumpLabelNameList>;
      <symbolIndexToLabelIndexMap> maps a symbol index to the index of
      the jump label list where its target bank label occurs;
      <interbankReferenceIsFound> tells whether some interbank
      reference exists at all */
{
  Banking_Configuration *bankingConfiguration =
    Target_info.bankingConfiguration;
  IntegerMap_Type bankToLabelIndexMap =
    Map_make(TypeDescriptor_plainDataTypeDescriptor);
  List_Cursor moduleCursor;
  List_Type moduleList = List_make(Module_typeDescriptor);

  Map_clear(&bankToLabelIndexMap);

  /* traverse all modules to find interbank references */
  Module_getModuleList(&moduleList);

  for (moduleCursor = List_resetCursor(moduleList);
       moduleCursor != NULL;
       List_advanceCursor(&moduleCursor)) {
    Target_Bank currentBank;
    Module_Type module = List_getElementAtCursor(moduleCursor);
    List_Type moduleSymbolList = List_make(Symbol_typeDescriptor);
    List_Cursor symbolCursor;

    currentBank = Banking__getBank(module);
    Module_getSymbolList(module, &moduleSymbolList);

    /* traverse all symbols in the symbol list of module and check
       them for interbank references */
    for (symbolCursor = List_resetCursor(moduleSymbolList);
	 symbolCursor != NULL;
	 List_advanceCursor(&symbolCursor)) {
      Symbol_Type symbol = List_getElementAtCursor(symbolCursor);
      Area_Segment segment = Symbol_getSegment(symbol);
      
      if (Symbol_isSurrogate(symbol) || segment == NULL) {
	/* this is either a surrogate symbol or an absolute symbol
	   without segment ==> ignore */
      } else {
	Boolean isRelevant;
	String_Type symbolName = String_make();
	Module_Type targetModule = Area_getSegmentModule(segment);
	Target_Bank targetBank = Banking__getBank(targetModule);

	Symbol_getName(symbol, &symbolName);

	/* check whether call goes to different bank and this bank
	   might be swapped out */
	isRelevant = (currentBank != targetBank 
		      && targetBank != Target_undefinedBank);

	if (isRelevant) {
	  /* check whether the target symbol is a possible target for an
	     interbank call */
	  String_Type segmentName = String_make();
	  String_Type targetModuleName = String_make();

	  Module_getName(targetModule, &targetModuleName);
	  Area_getSegmentName(segment, &segmentName);

	  isRelevant = 
	    bankingConfiguration->ensureAsCallTarget(targetModuleName,
						     segmentName, symbolName);

	  String_destroy(&segmentName);
	  String_destroy(&targetModuleName);
	}

	if (isRelevant) {
	  /* this is an interbank reference */
	  SizeType labelIndex;
	  Symbol_Type surrogateSymbol;
	  String_Type surrogateSymbolName = String_make();
	  SizeType symbolIndex;
	  Object targetBankAsObject = (Object) targetBank;

	  *interbankReferenceIsFound = true;

	  /* generate a surrogate symbol by splitting existing symbol
	     into the referenced symbol (defined but not yet referenced)
	     and the surrogate symbol (referenced by modules but not yet
	     defined) */
	  bankingConfiguration->makeSurrogateSymbolName(&surrogateSymbolName,
							symbolName);
	  surrogateSymbol = Symbol_makeBySplit(symbol, surrogateSymbolName);

	  /* add symbol names to lists for later output */
	  StringList_append(symbolNameList, symbolName);
	  StringList_append(surrogateNameList, surrogateSymbolName);
	  symbolIndex = List_length(*symbolNameList);
	  labelIndex = IntegerMap_lookup(bankToLabelIndexMap,
					 targetBankAsObject);

	  if (labelIndex == IntegerMap_notFound) {
	    /* there is not yet a jump label for this target bank ==>
	       make it */
	    String_Type jumpLabelName = String_make();

	    labelIndex = List_length(*jumpLabelNameList);
	    bankingConfiguration->makeJumpLabelName(&jumpLabelName,
						    targetBank);
	    StringList_append(jumpLabelNameList, jumpLabelName);
	    IntegerMap_set(&bankToLabelIndexMap, targetBankAsObject,
			   labelIndex);
	    String_destroy(&jumpLabelName);
	  }


	  IntegerMap_set(symbolIndexToLabelIndexMap, (Object) symbolIndex,
			 labelIndex);

	  String_destroy(&surrogateSymbolName);
	}

	String_destroy(&symbolName);
      }
    }

    List_destroy(&moduleSymbolList);
  }

  Map_destroy(&bankToLabelIndexMap);
  List_destroy(&moduleList);
}

/*--------------------*/

static Target_Bank Banking__getBank (in Module_Type module)
  /** returns associated bank for <module> or <undefinedBank> if none
      exists */
{
  //TODO: make this more intelligent by an auxiliary map
  // static Map_Type moduleToBankMap;
  String_Type moduleName = String_make();
  String_Type upperCaseModuleName = String_make();
  Target_Bank result;

  Module_getName(module, &moduleName);
  String_convertToUpperCase(moduleName, &upperCaseModuleName);
  result = Banking_getModuleBank(upperCaseModuleName);
  String_destroy(&upperCaseModuleName);
  String_destroy(&moduleName);
  return result;
}

/*--------------------*/

static void Banking__relocateBankedSegments (void)
  /* loops over all segments in <genericBankedCodeAreaName> and
     relocates them to the correct banked area */
{
  Banking_Configuration *bankingConfiguration =
    Target_info.bankingConfiguration;
  String_Type genericBankedCodeAreaName =
    bankingConfiguration->genericBankedCodeAreaName;
  Area_Type genericBankedArea;

  Area_lookup(&genericBankedArea, genericBankedCodeAreaName);

  if (genericBankedArea == NULL) {
    Error_raise(Error_Criticality_warning, 
		"no banked segments found for area %s",
		String_asCharPointer(genericBankedCodeAreaName));
  } else {
    Area_AttributeSet attributeSet = Area_getAttributes(genericBankedArea);
    String_Type bankedCodeAreaName = String_make();
    Banking_NameConstructionProc makeBankedCodeAreaName =
      bankingConfiguration->makeBankedCodeAreaName;
    List_Cursor segmentCursor;
    Area_SegmentList segmentList = List_make(Area_segmentTypeDescriptor);

    Area_getListOfSegments(genericBankedArea, &segmentList);

    for (segmentCursor = List_resetCursor(segmentList);  segmentCursor != NULL;
	 List_advanceCursor(&segmentCursor)) {
      Area_Segment segment = List_getElementAtCursor(segmentCursor);
      Module_Type segmentModule = Area_getSegmentModule(segment);
      Target_Bank currentBank = Banking__getBank(segmentModule);
      Area_Type bankedArea;

      /* move this segment to another area defined by the bank of the
	 associated module */
      makeBankedCodeAreaName(&bankedCodeAreaName, currentBank);
      bankedArea = Area_make(bankedCodeAreaName, attributeSet);
      Area_setSegmentArea(&segment, bankedArea);
    }

    Area_clearListOfSegments(&genericBankedArea);
    String_destroy(&bankedCodeAreaName);
  }
}

/*--------------------*/

static void Banking__setBankForModuleName (in String_Type moduleName,
					   in long bank)
  /** sets bank for module with <moduleName> to <bank> */
{
  String_Type upperCaseModuleName = String_make();
  Object bankAsObject = (Object) bank;

  String_convertToUpperCase(moduleName, &upperCaseModuleName);
  Map_set(&Banking__moduleNameToBankMap, upperCaseModuleName, bankAsObject);
}

/*--------------------*/

static void Banking__writeStubFile (in String_Type stubFileName,
				    in StringList_Type jumpLabelNameList,
				    in StringList_Type surrogateNameList,
				    in StringList_Type symbolNameList,
				    in Map_Type symbolIndexToLabelIndexMap)
  /** generates a temporary object file with all the banking
      definitions; the names of all external symbols are given as
      <symbolNameList>, the name of the surrogates as
      <surrogateNameList> and the names of the jump labels for bank
      switching as <jumpLabelNameList>; <symbolIndexToLabelIndexMap>
      is a map from symbol index to the index of the jump label list
      where its target bank label occurs */
{
  char *procName = "Banking__writeStubFile";
  Boolean precondition;

  {
    SizeType surrogateCount = List_length(surrogateNameList);
    SizeType symbolCount    = List_length(symbolNameList);
    SizeType jumpLabelCount = List_length(jumpLabelNameList);

    precondition = (PRE(jumpLabelCount > 0, procName, "no jump labels")
		    && PRE(surrogateCount == symbolCount, procName,
			   "no matching surrogates for external symbols"));
  }

  if (precondition) {
    Boolean fileIsOpen;
    File_Type stubCodeFile;

    fileIsOpen = File_open(&stubCodeFile, stubFileName, File_Mode_write);

    if (!fileIsOpen) {
      Error_raise(Error_Criticality_fatalError, 
		  "cannot create stub code file %s", 
		  String_asCharPointer(stubFileName));
    } else {
      Banking_Configuration *bankingConfiguration
	= Target_info.bankingConfiguration;
      UINT8 offsetPerTrampolineCall =
	bankingConfiguration->offsetPerTrampolineCall;
      List_Cursor stringCursor;
      SizeType totalSymbolCount = (List_length(jumpLabelNameList)
		  		   + 2 * List_length(surrogateNameList));

      /* write radix and header line */
      File_writeCharArray(&stubCodeFile, "X\nH ");
      File_writeCharArray(&stubCodeFile, "1 areas ");
      File_writeHex(&stubCodeFile, totalSymbolCount, 4);
      File_writeCharArray(&stubCodeFile, " global symbols\n");

      /* write module line */
      File_writeCharArray(&stubCodeFile, "M generatedBanking\n");

      /* write symbol lines for all external referenced symbols i.e. the
	 jump labels and the symbol names */
      {
	StringList_Type externalNameList = StringList_make();
	List_copy(&externalNameList, jumpLabelNameList);
	List_concatenate(&externalNameList, symbolNameList);

	for (stringCursor = List_resetCursor(externalNameList);
	     stringCursor != NULL;
	     List_advanceCursor(&stringCursor)) {
	  String_Type externalName = List_getElementAtCursor(stringCursor);
	  File_writeCharArray(&stubCodeFile, "S ");
	  File_writeString(&stubCodeFile, externalName);
	  File_writeCharArray(&stubCodeFile, " Ref0000\n");
	}

	List_destroy(&externalNameList);
      }

      /* write information about nonbanked code area containing the
	 definitions of the surrogate symbols */
      {
	UINT16 jumpTableSize = (List_length(surrogateNameList)
				* offsetPerTrampolineCall);

	File_writeCharArray(&stubCodeFile, "A ");
	File_writeString(&stubCodeFile,
			 bankingConfiguration->nonbankedCodeAreaName);
	File_writeCharArray(&stubCodeFile, " size ");
	File_writeHex(&stubCodeFile, jumpTableSize, 4);
	File_writeCharArray(&stubCodeFile, " flags 0\n");
      }

      /* write all defined symbols i.e. the surrogate symbol names and
	 their offsets (starting with zero) */
      {
	int offsetInSegment = 0;

	for (stringCursor = List_resetCursor(surrogateNameList);
	     stringCursor != NULL;
	     List_advanceCursor(&stringCursor)) {
	  String_Type surrogateSymbolName =
	    List_getElementAtCursor(stringCursor);

	  File_writeCharArray(&stubCodeFile, "S ");
	  File_writeString(&stubCodeFile, surrogateSymbolName);
	  File_writeCharArray(&stubCodeFile, " Def");
	  File_writeHex(&stubCodeFile, offsetInSegment, 4);
	  File_writeChar(&stubCodeFile, '\n');
	  offsetInSegment += offsetPerTrampolineCall;
	}
      }

      /* finally write the trampoline call code sequences */
      {
	int callCount = List_length(symbolNameList);
	String_Type codeSequence = String_make();
	int targetSymbolIndex = List_length(jumpLabelNameList);
	int surrogateSymbolIndex = 0;
	int offsetInSegment = 0;
	int i;

	for (i = 1;  i <= callCount;  i++) {
	  /* write code for each trampoline call */
 
	  /* all external symbols are in area 0 */
	  UINT16 referencedAreaIndex = 0;
	  UINT16 jumpLabelSymbolIndex = 
	    IntegerMap_lookup(symbolIndexToLabelIndexMap, (Object) i);

	  bankingConfiguration->makeTrampolineCallCode(offsetInSegment,
						       referencedAreaIndex,
						       targetSymbolIndex,
						       jumpLabelSymbolIndex,
						       &codeSequence);

	  File_writeString(&stubCodeFile, codeSequence);

	  targetSymbolIndex++;
	  surrogateSymbolIndex++;
	  offsetInSegment += offsetPerTrampolineCall;
	}

	String_destroy(&codeSequence);
      }

      /* we're done */
      File_close(&stubCodeFile);
    }
  }
}

/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Banking_initialize (void)
{
  Banking__moduleNameToBankMap = Map_make(String_typeDescriptor);
}

/*--------------------*/

void Banking_finalize (void)
{
  Map_destroy(&Banking__moduleNameToBankMap);
}

/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Banking_adaptAreaNameWhenBanked (in Module_Type module,
				      inout String_Type *areaName)
{
  if (Banking_isActive()) {
    Banking_Configuration *bankingConfiguration =
      Target_info.bankingConfiguration;
    String_Type genericBankedCodeAreaName =
      bankingConfiguration->genericBankedCodeAreaName;

    if (String_isEqual(*areaName, genericBankedCodeAreaName)) {
      Target_Bank currentBank = Banking__getBank(module);
      Banking_NameConstructionProc makeBankedCodeAreaName =
	bankingConfiguration->makeBankedCodeAreaName;

      makeBankedCodeAreaName(areaName, currentBank);
    }
  }
}

/*--------------------*/

Boolean Banking_isActive (void)
{
  return (Target_info.bankingConfiguration != NULL);
}

/*--------------------*/

Target_Bank Banking_getModuleBank (in String_Type moduleName)
{
  Object bankPtr = Map_lookup(Banking__moduleNameToBankMap, moduleName);
  Target_Bank result = Target_undefinedBank;

  if (bankPtr != NULL) {
    result = (Target_Bank) bankPtr;
  }

  return result;
}

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Banking_readConfigurationFile (in String_Type fileName)
{
  Boolean fileIsOpen;
  File_Type configurationFile;
  char commentChar = ';';

  fileIsOpen = File_open(&configurationFile, fileName, File_Mode_read);

  if (!fileIsOpen) {
    Error_raise(Error_Criticality_fatalError,
		"cannot open banking configuration file %s", 
		String_asCharPointer(fileName));
  } else {
    /* read lines from the configuration file specifying the mapping
       from module names to banks */
    String_Type currentLine = String_make();
    StringList_Type lineList = StringList_make();
    Boolean isDone = false;

    while (!isDone) {
      File_readLine(&configurationFile, &currentLine);

      if (String_length(currentLine) == 0) {
	isDone = true;
      } else if (String_getCharacter(currentLine, 1) == commentChar) {
	/* ignore comment line */
      } else {
	/* strip off newline and append to line list */
	SizeType length = String_length(currentLine);
	String_deleteCharacters(&currentLine, length, 1);
	StringList_append(&lineList, currentLine);
      }
    }

    Parser_setMappingFromList(lineList, Banking__setBankForModuleName);

    List_destroy(&lineList);
    String_destroy(&currentLine);
  }
}

/*--------------------*/

Boolean Banking_resolveInterbankReferences (inout StringList_Type *fileList)
{
  Boolean interbankReferenceIsFound = false;

  if (Banking_isActive()) {
    Map_Type symbolIndexToLabelIndexMap =
      Map_make(TypeDescriptor_plainDataTypeDescriptor);
    StringList_Type jumpLabelNameList = StringList_make();
    StringList_Type surrogateNameList = StringList_make();
    StringList_Type symbolNameList = StringList_make();
    
    /* loop over all segments belonging to <genericBankedCodeAreaName>
       and relocate them to the correct banked area */
    Banking__relocateBankedSegments();
    
    /* find all interbank reference symbols and split them; collect
       symbol, surrogate and label names in lists */
    Banking__collectInterbankReferences(&jumpLabelNameList, &surrogateNameList,
					&symbolNameList,
					&symbolIndexToLabelIndexMap,
					&interbankReferenceIsFound);

    if (interbankReferenceIsFound) {
      /* generate a temporary object file with all the banking
	 definitions */
      String_Type stubFileName = String_makeFromCharArray("c:/tmp/xxx.o");
      Banking__writeStubFile(stubFileName, jumpLabelNameList,
			     surrogateNameList, symbolNameList,
			     symbolIndexToLabelIndexMap);

      Parser_parseObjectFile(true, stubFileName);
      StringList_append(fileList, stubFileName);
      String_destroy(&stubFileName);
    }

    List_destroy(&symbolNameList);
    List_destroy(&surrogateNameList);
    List_destroy(&jumpLabelNameList);
    Map_destroy(&symbolIndexToLabelIndexMap);
  }

  return interbankReferenceIsFound;
}
