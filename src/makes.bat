@ECHO OFF
SET CFLAGS=/nologo /RTCsu /Z7 /RTC1 /RTCs /RTCu /Wall /WL /c

SET CFLAGS=%CFLAGS% /wd4100 /wd4255 /wd4267 /wd4296 /wd4305 /wd4306
SET CFLAGS=%CFLAGS% /wd4820 /wd4996
REM the following warnings are disabled
REM   4100: unreferenced formal parameter
REM   4255: bad prototype in standard include file...
REM*   4267: conversion to type of smaller size, possible loss of data
REM   4296: expression is always true
REM*   4305: truncation from TYPEX to TYPEY
REM*   4306: type cast conversion to type of greater size
REM   4820: 'bytes' bytes padding added after construct 'member_name'
REM   4996: deprecated unsafe string routines

SET EXTCFLAGS=%CFLAGS% /wd4028 /wd4090
REM the following warnings are disabled here
REM   4028: formal parameter X different from declaration
REM   4090: 'function' : different 'const' qualifiers

cl %CFLAGS% area.c
cl %CFLAGS% banking.c
cl %CFLAGS% codeoutput.c
cl %CFLAGS% codesequence.c
cl %CFLAGS% error.c
cl %CFLAGS% file.c
cl %CFLAGS% globdefs.c
cl %CFLAGS% integermap.c
cl %CFLAGS% library.c
cl %CFLAGS% list.c
cl %CFLAGS% listingupdater.c
cl %CFLAGS% main.c
cl %CFLAGS% module.c
cl %CFLAGS% map.c
cl %EXTCFLAGS% mapfile.c
cl %CFLAGS% multimap.c
cl %CFLAGS% noicemapfile.c
cl %CFLAGS% platform\gameboy.c
cl %CFLAGS% parser.c
cl %CFLAGS% scanner.c
cl %CFLAGS% set.c
cl %CFLAGS% string.c
cl %CFLAGS% stringlist.c
cl %CFLAGS% stringtable.c
cl %CFLAGS% symbol.c
cl %CFLAGS% target.c
cl %CFLAGS% typedescriptor.c

link /DEBUG main area banking codeoutput codesequence error file globdefs integermap library list listingupdater module map mapfile multimap noicemapfile gameboy parser scanner set string stringlist stringtable symbol target typedescriptor

REM DEL *.obj