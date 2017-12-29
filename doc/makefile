#!/bin/make
# -- makefile for preparing, typesetting and deploying the Revised
#    SDCCLinker documentation

include ../_configure.mk
include _localconfigure.mk

#-----------------

all: deploy

deploy: process

process: prepare

#===============
#= PREPARATION =
#===============

#-----------------------
#- documentation files -
#-----------------------

targetFiles:= \
    $(TARGET_DOCUMENTATION_DIR)/linker_documentation.tex \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures.mp \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/ModuleView.mp

$(DOCUMENTATION_FIGURE_DIRECTORY):
	$(MKDIR_PROGRAM) -p $@

$(targetFiles): | $(DOCUMENTATION_FIGURE_DIRECTORY)

$(targetFiles): $(TARGET_DOCUMENTATION_DIR)/%: %
	$(CP_PROGRAM) $< $@

prepare: $(targetFiles)

#==============
#= PROCESSING =
#==============

mainFileStem:=linker_documentation

# all picture files in the figures directory
pictures:= \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-1.mps  \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-2.mps  \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-3.mps  \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-4.mps  \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-5.mps  \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-6.mps  \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-7.mps  \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-8.mps  \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-9.mps  \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-10.mps \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-11.mps \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-12.mps \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-13.mps \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-14.mps \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-15.mps \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-16.mps \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-17.mps \
    $(DOCUMENTATION_FIGURE_DIRECTORY)/linkerFigures-18.mps

$(pictures): | $(DOCUMENTATION_FIGURE_DIRECTORY)

$(pictures):
	cd $(DOCUMENTATION_FIGURE_DIRECTORY) \
	&& $(METAPOST) -quiet linkerFigures

# all TeX include files describing the interfaces
texFiles:=$(SUPPORTING_MODULE_NAME_LIST:%=$(TARGET_DOCUMENTATION_DIR)/%.tex)
texFiles:=$(filter-out $(TARGET_DOCUMENTATION_DIR)/platform/%, $(texFiles))
allTexFiles:=$(texFiles) $(TARGET_DOCUMENTATION_DIR)/gameboy.tex

$(texFiles): $(TARGET_DOCUMENTATION_DIR)/%.tex: $(TARGET_SOURCE_DIR)/%.h
	$(makeTexFromIncludeCommand) $< >$@

$(TARGET_DOCUMENTATION_DIR)/gameboy.tex: \
    $(TARGET_SOURCE_DIR)/platform/gameboy.h
	$(makeTexFromIncludeCommand) $< >$@

# the main pdf documentation file
# we have to run LaTeX twice for correct labels
pdfFile:=$(TARGET_DOCUMENTATION_DIR)/$(mainFileStem).pdf

$(pdfFile): $(pictures) $(allTexFiles) \
    $(TARGET_DOCUMENTATION_DIR)/$(mainFileStem).tex
	cd $(TARGET_DOCUMENTATION_DIR) \
	&& $(PDFLATEX) -quiet $(mainFileStem) \
	&& $(PDFLATEX) -quiet $(mainFileStem)

process: $(pdfFile)

#-----------------------
#- processing commands -
#-----------------------

makeTexFromIncludeCommand:=$(GAWK_PROGRAM) -f $(TOOLS_DIR)/cIncludeToLatex.awk


#==========
#= DEPLOY =
#==========

deploy:
	$(CP_PROGRAM) $(pdfFile) $(TARGET_DOCUMENTATION)


#==============
#= CLEANUP =
#==============

# -- cleaning up the temporary files generated
# -- for the documentation: the figures, logs and the interface
# -- description TeX files for all modules

clean:
	$(RM_PROGRAM) $(DOCUMENTATION_FIGURE_DIRECTORY)/*.mps
	$(RM_PROGRAM) $(DOCUMENTATION_FIGURE_DIRECTORY)/*.log
	$(RM_PROGRAM) $(DOCUMENTATION_FIGURE_DIRECTORY)/*.mpx
	cd $(TARGET_DOCUMENTATION_DIR) && $(RM_PROGRAM) $(allTexFiles)
	$(RM_PROGRAM) $(TARGET_DOCUMENTATION_DIR)/$(mainFileStem).aux
	$(RM_PROGRAM) $(TARGET_DOCUMENTATION_DIR)/$(mainFileStem).log
	$(RM_PROGRAM) $(TARGET_DOCUMENTATION_DIR)/$(mainFileStem).pdf
