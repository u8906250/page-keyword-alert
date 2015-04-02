CFLAGS = -O2 -Wall -mwindows\
	 -I../gtk+-bundle_3.6.4-20130921_win32/include/gtk-3.0\
	 -I../gtk+-bundle_3.6.4-20130921_win32/include/glib-2.0\
	 -I../gtk+-bundle_3.6.4-20130921_win32/lib/glib-2.0/include\
	 -I../gtk+-bundle_3.6.4-20130921_win32/include/pango-1.0\
	 -I../gtk+-bundle_3.6.4-20130921_win32/include/cairo\
	 -I../gtk+-bundle_3.6.4-20130921_win32/include/gdk-pixbuf-2.0\
	 -I../gtk+-bundle_3.6.4-20130921_win32/include/atk-1.0\
	 -I./lib
LDFLAGS = -L../gtk+-bundle_3.6.4-20130921_win32/lib -lgtk-3 -lole32 -latk-1.0 -lgio-2.0 -lcairo -lpango-1.0 -lglib-2.0 -lgobject-2.0 -L./lib/curl -lcurl -lz -lws2_32

all: page-keyword-alert

page-keyword-alert: main.c
	$(CC) $(CFLAGS) -o bin/page-keyword-alert $^ $(LDFLAGS)

clean:
	rm -f bin/page-keyword-alert bin/page-keyword-alert.exe *~


