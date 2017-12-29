======================================
Revised SDCC-Linker Distribution
======================================

This is the source suite for the Revised SDCC Linker for the
SDCC-C-Compiler.

-------------
Preconditions
-------------

Ensure that you have the following software available:

  - some C compiler for your platform,

  - an AWK program (e.g. GNU awk) and

  - LaTeX (if you want to generate the documentation).

------------------
Installation Steps
------------------

For installation you should proceed as follows:

  1. Edit the file "_configure.mk" for the Unix platform or the file
     "_configure.bat" for the Windows platform (see below for details).

  2. For Windows start the file "install.bat", for Unix start the file
     "install.sh".  The preparation and installation starts and will
     take a few minutes.

  3. When you have correctly configured everything, the linker
     executable and the documentation PDF are placed into the
     appropriate directories ready for use.  You can then use the
     linker with an SDCC compiler.

  4. To clean up temporary files run the script "cleanup.bat" for
     Windows or "clean.sh" for Unix.

----------------------------------
Setting up the configuration files
----------------------------------

The configuration file is a makefile for Unix and a batch file for
Windows.  Here you can set the directories for sources and target
files, the names of the programs in your environment etc.

Note that the source and documentation files are in principle copied
to some other place.  Normally you specify some directory for the
installation that has subdirectories for the include files, the
sources and the objects plus the final executable.

Hence the following variables have to be defined:

  - TARGET_ROOT_DIR:
    the parent directory of all target directories

  - TARGET_SOURCE_DIR:
    the root directory of the C include and src files; normally set to
    TARGET_ROOT_DIR/src

  - TARGET_OBJECT_DIR:
    the root directory of the generated object files; normally set to
    TARGET_ROOT_DIR/objects

  - TARGET_DOCUMENTATION_DIR:
    the root directory of documentation files; normally set to
    TARGET_ROOT_DIR/doc

  - TARGET_EXECUTABLE:
    the path name of the target SDCC linker program

  - TARGET_DOCUMENTATION:
    the path name of the target SDCC linker documentation

  - CP_PROGRAM:
    the path name of the copy program; "cp" on Unix and "copy" on
    Windows

  - GAWK_PROGRAM:
    the path name of the GAWK program

  - MKDIR_PROGRAM:
    the path name of the directory creation program (MKDIR)

  - MV_PROGRAM:
    the path name of the file movement program; "mv" on Unix and
    "move" on Windows

  - RM_PROGRAM: 
    the path name of the file deletion program; "rm" on Unix and "del"
    on Windows

  - TOOLS_DIR:
    the path name of the directory containing the specific tools for
    installation (like Shell or AWK scripts); typically it is the
    "tools" subdirectory of the main installation directory

  - C_PATH:
    the path name of the directory containing the C compiler

  - CCOMP:
    the path name of the C compiler; note that the flag settings are done
    in the local configuration of the source directory

  - CLINKER:
    the path name of the C linker; note that the flag settings are done
    in the local configuration of the source directory

  - PDFLATEX:
    the LaTeX program delivering a PDF file

  - METAPOST:
    the MetaPost program for handling the documentation diagrams

  - SUPPORTING_MODULE_NAME_LIST:
    the name list of all supporting modules (excluding main);
    typically you should not change that except when you are
    adding or replacing some modules

  - MODULE_NAME_LIST:
    the name list of all supporting modules (including main); the same
    comment as above applies here


