bin_PROGRAMS = xrtailab

xrtailab_SOURCES = \
	Fl_Led.h \
	Fl_Led.cpp \
	Fl_Led_Window.h \
	Fl_Led_Window.cpp \
	Fl_Meter.h \
	Fl_Meter.cpp \
	Fl_Meter_Window.h \
	Fl_Meter_Window.cpp \
	Fl_Scope.h \
	Fl_Scope.cpp \
	Fl_Scope_Window.h \
	Fl_Scope_Window.cpp \
	Fl_Synchronoscope.h \
	Fl_Synchronoscope.cpp \
	Fl_Synchronoscope_Window.h \
	Fl_Synchronoscope_Window.cpp \
	xrtailab.cpp

efltk_cflags = $(shell @EFLTK_DIR@/bin/efltk-config --cxxflags)
efltk_ldflags = $(shell @EFLTK_DIR@/bin/efltk-config --ldflags --use-gl --multithread)

xrtailab_CPPFLAGS = \
	@RTAI_USER_CFLAGS@ \
	-I$(top_srcdir)/rtai-core/include \
	-I../rtai-core/include \
	$(efltk_cflags) \
	@GL_CFLAGS@

xrtailab_LDFLAGS = \
	$(efltk_ldflags) \
	@GL_LIBS@

SUBDIRS = scilab matlab

EXTRA_DIST = icons
