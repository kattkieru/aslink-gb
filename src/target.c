/** Target Configuration module --
    Implementation of module providing all services for specifying
    target specific configuration information within the SDCC linker.

    Original version by Thomas Tensi, 2007-02
*/

#include "target.h"

/*========================================*/

#include "codeoutput.h"
#include "globdefs.h"
#include "platform/gameboy.h"
#include "string.h"
#include "stringlist.h"

/*========================================*/

Target_Type Target_info;
  /** variable containing the information about the current target
      platform */


/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Target_initialize (void)
{
  Target_info = Gameboy_targetInfo;
}

/*--------------------*/

void Target_finalize (void)
{
}


/*--------------------*/
/* CONFIGURATION      */ 
/*--------------------*/

void Target_setInfo (in String_Type platformName)
{
}
