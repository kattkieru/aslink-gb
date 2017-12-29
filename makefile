#!/bin/make
# -- setup of the Revised SDCC Linker

all: 
	make -C src all
	make -C doc all

clean:
	make -C src clean
	make -C doc clean
