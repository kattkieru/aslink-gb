#!/bin/make
# -- makefile for preparing, compiling and deploying the Revised
#    SDCCLinker executable

include ../_configure.mk
include _localconfigure.mk

#-----------------

all: deploy

deploy: process

process: prepare

#===============
#= PREPARATION =
#===============

#------------------
#- C source files -
#------------------

targetDirectories := \
  $(TARGET_SOURCE_DIR) $(TARGET_SOURCE_DIR)/platform

$(targetDirectories):
	$(MKDIR_PROGRAM) -p $@

targetFiles := \
    $(SUPPORTING_MODULE_NAME_LIST:%=$(TARGET_SOURCE_DIR)/%.h) \
    $(SUPPORTING_MODULE_NAME_LIST:%=$(TARGET_SOURCE_DIR)/%.c) \
    $(TARGET_SOURCE_DIR)/main.c

$(targetFiles): | $(targetDirectories)

$(targetFiles): $(TARGET_SOURCE_DIR)/% : %
	$(CP_PROGRAM) $< $@

prepare: $(targetFiles)

#==============
#= PROCESSING =
#==============

# object directories
objectDirectories := \
    $(TARGET_OBJECT_DIR) $(TARGET_OBJECT_DIR)/platform

$(objectDirectories):
	$(MKDIR_PROGRAM) -p $@

# object files requiring a special compilation
specialObjectFiles := $(TARGET_OBJECT_DIR)/mapfile.obj

# object files requiring a standard compilation
standardObjectFiles := $(SUPPORTING_MODULE_NAME_LIST:platform/=)
standardObjectFiles := $(standardObjectFiles:%=$(TARGET_OBJECT_DIR)/%.obj)
standardObjectFiles := \
    $(filter-out $(specialObjectFiles), $(standardObjectFiles))

mainObjectFile := $(TARGET_OBJECT_DIR)/main.obj

allObjectFiles := \
    $(mainObjectFile) $(standardObjectFiles) $(specialObjectFiles)

$(allObjectFiles): | $(objectDirectories)

$(standardObjectFiles) $(mainObjectFile): \
    $(TARGET_OBJECT_DIR)/%.obj: $(TARGET_SOURCE_DIR)/%.c
	$(standardCompileCommand)

$(specialObjectFiles): $(TARGET_OBJECT_DIR)/%.obj: $(TARGET_SOURCE_DIR)/%.c
	$(extendedCompileCommand)

# the main executable file
executableFile := $(TARGET_OBJECT_DIR)/main.exe

$(executableFile): $(allObjectFiles)
	@echo "### SDCC linker: linking into " $@ \
	&& $(CLINKER) $(CLINKFLAGS) /OUT:$@ $^

process: $(executableFile)

#-----------------------
#- processing commands -
#-----------------------

standardCompileCommand = \
    @echo "=== compiling" $< "===" \
    && $(CCOMP) $(CCOMPFLAGS) $< $(CCOMP_OUTFLAG)$@

extendedCompileCommand = \
    @echo "=== compiling" $< "[]===" \
    && $(CCOMP) $(EXTENDED_CCOMPFLAGS) $< $(CCOMP_OUTFLAG)$@


#==========
#= DEPLOY =
#==========

deploy:
	$(CP_PROGRAM) $(executableFile) $(TARGET_EXECUTABLE)


#==============
#= CLEANUP =
#==============

# -- cleaning up the temporary files generated
# -- for the documentation: the figures, logs and the interface
# -- description TeX files for all modules

clean:
	$(RM_PROGRAM) $(allObjectFiles)
	$(RM_PROGRAM) $(executableFile)
