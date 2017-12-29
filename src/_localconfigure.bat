@ECHO OFF
REM -- file containing the local settings for the SDCC linker build
REM -- overriding the global settings

REM -- the C compiler flags
SET CCOMPFLAGS=/nologo /RTCsu /Z7 /RTC1 /RTCs /RTCu /Wall /WL /c
SET CCOMPFLAGS=%CCOMPFLAGS% /I%C_PATH%\include /I%TARGET_INCLUDE_DIR%
SET CCOMPFLAGS=%CCOMPFLAGS% /wd4100 /wd4255 /wd4267 /wd4296 /wd4305 /wd4306
SET CCOMPFLAGS=%CCOMPFLAGS% /wd4820 /wd4996
  REM the following warnings are disabled
  REM   4100: unreferenced formal parameter
  REM   4255: bad prototype in standard include file...
  REM   4296: expression is always true
  REM   4820: 'bytes' bytes padding added after construct 'member_name'
  REM   4996: deprecated unsafe string routines

SET EXTENDED_CCOMPFLAGS=%CCOMPFLAGS% /wd4028 /wd4090
  REM the following warnings are disabled here
  REM   4028: formal parameter X different from declaration
  REM   4090: 'function' : different 'const' qualifiers

REM -- the C compiler flag for specifying the output object file
SET CCOMP_OUTFLAG=/Fo

SET CLINKFLAGS=/DEBUG /NOLOGO /LIBPATH:%C_PATH%\lib
