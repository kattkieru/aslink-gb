@ECHO OFF
REM -- file containing the global settings for the SDCCLinker C environment
REM
REM naming convention:
REM  - globally visible names are upper-case and underscore-separated
REM  - local names are lower-case, camel-cased

REM ---------------------------
REM - target root directories -
REM ---------------------------

REM -- the root of the complete installation target directories
SET TARGET_ROOT_DIR=C:\temp\SDCCLinker

REM -- the directory of C include files
SET TARGET_INCLUDE_DIR=%TARGET_ROOT_DIR%\src

REM -- the directory of C src files
SET TARGET_SOURCE_DIR=%TARGET_ROOT_DIR%\src

REM -- the directory of object files
SET TARGET_OBJECT_DIR=%TARGET_ROOT_DIR%\objects

REM -- the directory of documentation files
SET TARGET_DOCUMENTATION_DIR=%TARGET_ROOT_DIR%\doc

REM ----------------
REM - target files -
REM ----------------

REM -- the path name of the target SDCC linker program
SET TARGET_EXECUTABLE=%TARGET_ROOT_DIR%\rev_sdcc_linker.exe
REM -- the path name of the target SDCC linker documentation
SET TARGET_DOCUMENTATION=%TARGET_ROOT_DIR%\rev_sdcc_linker-documentation.pdf

REM --------------------
REM - support programs -
REM --------------------

REM -- the directory of the specific development tools
SET configureScriptDirectory=%~dp0
SET configureScriptDirectory=%configureScriptDirectory:~0,-1%
SET TOOLS_DIR=%configureScriptDirectory%\tools

SET programDirectory=c:\Programme

REM -- the path name of the copy program (CP)
SET CP_PROGRAM=CALL %TOOLS_DIR%\copyQuiet.bat

REM -- the path name of the GAWK program
SET binDirectory=%programDirectory%\Cygwin\bin
SET GAWK_PROGRAM=%binDirectory%\gawk.exe

REM -- the path name of the directory creation program (MKDIR)
SET MKDIR_PROGRAM=MKDIR

REM -- the path name of the file movement program (MV)
SET MV_PROGRAM=MOVE

REM -- the path name of the file deletion program (RM)
SET RM_PROGRAM=CALL %TOOLS_DIR%\deleteQuiet.bat

REM -- the C compiler and linker
SET C_PATH=%programDirectory%\Programmierung\Microsoft_Visual_C++
SET CCOMP=%C_PATH%\bin\cl
SET CLINKER=%C_PATH%\bin\link

REM -- the LaTeX and MetaPost programs
SET texPath=%programDirectory%\Buero\TeX\MiKTeX_2.8\miktex\bin
SET PDFLATEX=%texPath%\pdflatex
SET METAPOST=%texPath%\mp

REM ---------------------------------
REM - misc. configuration variables -
REM ---------------------------------

REM -- the name list of all supporting modules (excluding main) --
SET SUPPORTING_MODULE_NAME_LIST=area banking codeoutput codesequence error file
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% globdefs
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% integermap
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% library list
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% listingupdater
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% map mapfile
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% module multimap
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% noicemapfile
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% parser scanner
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% set string
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% stringlist
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% stringtable
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% symbol target
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% typedescriptor
SET SUPPORTING_MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% platform\gameboy

REM -- the name list of all supporting modules (including main) --
SET MODULE_NAME_LIST=%SUPPORTING_MODULE_NAME_LIST% main
