#!/bin/make
# -- file containing the global settings for the SDCCLinker generation
#
# naming convention for variables:
#  - globally visible names are upper-case and underscore-separated
#  - local names are lower-case, camel-cased

SHELL:=/bin/sh
.SUFFIXES:

#---------------------------
#- target root directories -
#---------------------------

# the root of the file system (for convenience)
ROOT_DIR:=C:

#-- the root of the complete installation target directories
TARGET_ROOT_DIR:=$(ROOT_DIR)/temp/SDCCLinker

#-- the root directory of C include and src files
TARGET_SOURCE_DIR:=$(TARGET_ROOT_DIR)/src

#-- the root directory of object files
TARGET_OBJECT_DIR:=$(TARGET_ROOT_DIR)/objects

# -- the directory of documentation files
TARGET_DOCUMENTATION_DIR:=$(TARGET_ROOT_DIR)/doc

#----------------
#- target files -
#----------------

#-- the path name of the target SDCC linker program
TARGET_EXECUTABLE:=$(TARGET_ROOT_DIR)/rev_sdcc_linker.exe
#-- the path name of the target SDCC linker documentation
TARGET_DOCUMENTATION:=$(TARGET_ROOT_DIR)/rev_sdcc_linker-documentation.pdf

#--------------------
#- support programs -
#--------------------

programDirectory:=$(ROOT_DIR)/Programme
binDirectory:=$(programDirectory)/Cygwin/bin

#-- the path name of the copy program (CP)
CP_PROGRAM:=$(binDirectory)/cp.exe

#-- the path name of the GAWK program
GAWK_PROGRAM:=$(binDirectory)/gawk.exe

#-- the path name of the directory creation program (MKDIR)
MKDIR_PROGRAM:=$(binDirectory)/mkdir.exe

#-- the path name of the file movement program (MV)
MV_PROGRAM:=$(binDirectory)/mv.exe

#-- the path name of the file deletion program (RM)
RM_PROGRAM:=$(binDirectory)/rm.exe -f

#-- the directory of the specific development tools
currentFilePath:=$(strip $(dir $(lastword $(MAKEFILE_LIST))))
TOOLS_DIR:=$(currentFilePath)tools

#-- the C compiler and linker
C_PATH:=$(programDirectory)/Programmierung/Microsoft_Visual_C++
CCOMP:=$(C_PATH)/bin/cl
CLINKER:=$(C_PATH)/bin/link

#-- the LaTeX and MetaPost programs
texPath:=$(programDirectory)/Buero/TeX/MiKTeX_2.8/miktex/bin
PDFLATEX:=$(texPath)/pdflatex
METAPOST:=$(texPath)/mp

#---------------------------
#- configuration variables -
#---------------------------

#-- the name list of all supporting modules (excluding main) --
SUPPORTING_MODULE_NAME_LIST:=area banking codeoutput codesequence error file \
                             globdefs integermap library list listingupdater \
                             map mapfile module multimap noicemapfile parser \
                             scanner set string stringlist stringtable symbol \
                             target typedescriptor platform/gameboy

#-- the name list of all supporting modules (including main) --
MODULE_NAME_LIST:=$(SUPPORTING_MODULE_NAME_LIST) main
