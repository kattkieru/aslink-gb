#!/bin/make
# -- file containing the local settings for the Revised SDCC Linker
# -- source overriding the global settings

#-- the C compiler flags
CCOMPFLAGS := /nologo /RTCsu /Z7 /RTC1 /RTCs /RTCu /Wall /WL /c \
              /I$(C_PATH)/include /I$(TARGET_INCLUDE_DIR) \
              /wd4100 /wd4255 /wd4267 /wd4296 /wd4305 /wd4306 \
              /wd4820 /wd4996

# the following warnings are disabled
#   4100: unreferenced formal parameter
#   4255: bad prototype in standard include file...
#   4296: expression is always true
#   4820: 'bytes' bytes padding added after construct 'member_name'
#   4996: deprecated unsafe string routines

EXTENDED_CCOMPFLAGS := $(CCOMPFLAGS) /wd4028 /wd4090
# the following warnings are disabled here
#   4028: formal parameter X different from declaration
#   4090: 'function' : different 'const' qualifiers

# -- the C compiler flag for specifying the output object file
CCOMP_OUTFLAG := /Fo

CLINKFLAGS := /DEBUG /NOLOGO /LIBPATH:$(C_PATH)/lib
