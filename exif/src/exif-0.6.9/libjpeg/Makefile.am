INCLUDES =		\
	-I$(top_srcdir)	\
	$(LIBEXIF_CFLAGS)

noinst_LTLIBRARIES = libjpeg.la
libjpeg_la_SOURCES =			\
	jpeg-data.c jpeg-data.h		\
	jpeg-marker.c jpeg-marker.h
libjpeg_la_LIBADD = $(LIBEXIF_LIBS)
