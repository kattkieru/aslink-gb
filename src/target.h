/** Target module --
    This module provides all services for specifying target specific
    configuration information within the SDCC linker.

    Original version by Thomas Tensi, 2006-08
*/

#ifndef __TARGET_H
#define __TARGET_H

/*========================================*/

#include "banking.h"
#include "globdefs.h"
#include "string.h"
#include "stringlist.h"

/*========================================*/

typedef UINT16 Target_Address;
  /** target addresses are 16 bit */

#define Target_undefinedBank (-1)
  /** undefined value for a bank number */

typedef int Target_Bank;
  /** bank number type */


typedef Target_Bank (*Target_BankAnalysisProc) (in String_Type segmentName);
  /** type for routines parsing the current segment with <segmentName>
      of emitted code for ROM bank switching */


typedef UINT8 (*Target_CodeQueryProc)(in Target_Bank bank,
				      in Target_Address address);
  /** type for routines returning the associated emitted code byte for
      <bank> and <address> */


typedef void (*Target_CommandLineHandleProc)(in String_Type mainFileNamePrefix,
				     in StringList_Type argumentList,
				     inout Boolean optionIsHandledList[]);
  /** type for routines parsing the command line options in
      <argumentList> for options relevant for this platform; all
      options where <optionIsHandledList> is true have already been
      processed before; when an option is used by that routine,
      <optionIsHandledList> is also set to true; <mainFileNamePrefix>
      tells the name of the main file passed as parameter without
      extension */


typedef void (*Target_UsageInfoProc)(out String_Type *st);
  /** type for routines returning a string with an indented line list
      (separated by newlines) with platform specific options as a
      usage info */


typedef void (*Target_InitializationProc)(void);
  /** type for routines setting up module internal data */


typedef void (*Target_FinalizationProc)(void);
  /** type for routines cleaning up module internal data */


typedef struct {
  Boolean isBigEndian;
  Boolean isCaseSensitive;
  Target_BankAnalysisProc getBankFromSegmentName;
  Target_CodeQueryProc getCodeByte;
  Target_UsageInfoProc giveUsageInfo;
  Target_CommandLineHandleProc handleCommandLineOptions;
  Target_InitializationProc initialize;
  Target_FinalizationProc finalize;
  Banking_Configuration *bankingConfiguration;
} Target_Type;
/** type to tell several properties of target platform like
    endianness, case sensitivity of names, banking configuration,
    callback routines for rom bank switching, querying for bytes in
    the emitted code, command line option parsing, giving usage
    information for target specific options and setting up and tearing
    down the platform specific data; each of those routines may be
    NULL when it is not used in this target platform */


extern Target_Type Target_info;
  /** variable containing the information about the current target
      platform */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Target_initialize (void);
  /** sets up module internal data */

/*--------------------*/

void Target_finalize (void);
  /** cleans up module internal data */


/*--------------------*/
/* CONFIGURATION      */ 
/*--------------------*/

void Target_setInfo (in String_Type platformName);
  /** sets <info> for platform specified by <platformName> */

#endif /* __TARGET_H */
