cmake_minimum_required( VERSION 3.4 )
project( aslink-gb )

set( ASLINK_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/../ )

set( SOURCES
	src/area.c
	src/banking.c
	src/codeoutput.c
	src/codesequence.c
	src/error.c
	src/file.c
	src/globdefs.c
	src/integermap.c
	src/library.c
	src/list.c
	src/listingupdater.c
	src/main.c
	src/module.c
	src/map.c
	src/mapfile.c
	src/multimap.c
	src/noicemapfile.c
	src/platform/gameboy.c
	src/parser.c
	src/scanner.c
	src/set.c
	src/string.c
	src/stringlist.c
	src/stringtable.c
	src/symbol.c
	src/target.c
	src/typedescriptor.c
)

if( UNIX )
	set( CFLAGS "-O3 -fPIC -mavx -m64 -g -std=c90" )
else()
	## I'm going by Dr. Tensi's settings for now
	set( CFLAGS "/nologo /O2 /Zi /RTC1 /Wall" )
	set( CFLAGS "${CFLAGS} /wd4100 /wd4255 /wd4267 /wd4296 /wd4305 /wd4306 /wd4820 /wd4996" )
endif()

add_executable( aslink ${SOURCES} )
install(
	TARGETS aslink
	DESTINATION ${ASLINK_PREFIX}/bin
)
