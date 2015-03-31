AUTOMAKE_OPTIONS=foreign

bin_PROGRAMS    = pagekeywordalert

pagekeywordalert_SOURCES	= winmain.c\
				  resource.rc

pagekeywordalert_CFLAGS	= -Wall -O2 -mwindows\
			  -I../lib

pagekeywordalert_LDADD	= ../lib/curl/libcurl.a\
			  -lz\
			  -lvfw32 -lws2_32 -lcomdlg32 -lgdi32 -lcomctl32 -lshlwapi -liconv

.rc.o:
	windres resource.rc -o resource.o
