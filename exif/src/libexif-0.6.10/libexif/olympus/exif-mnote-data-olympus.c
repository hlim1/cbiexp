/* exif-mnote-data-olympus.c
 *
 * Copyright � 2002, 2003 Lutz Mueller <lutz@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "exif-mnote-data-olympus.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libexif/exif-utils.h>
#include <libexif/exif-data.h>

#define DEBUG

static void
exif_mnote_data_olympus_clear (ExifMnoteDataOlympus *n)
{
	unsigned int i;

	if (!n) return;

	if (n->entries) {
		for (i = 0; i < n->count; i++)
			if (n->entries[i].data) {
				free (n->entries[i].data);
				n->entries[i].data = NULL;
			}
		free (n->entries);
		n->entries = NULL;
		n->count = 0;
	}
}

static void
exif_mnote_data_olympus_free (ExifMnoteData *n)
{
	if (!n) return;

	exif_mnote_data_olympus_clear ((ExifMnoteDataOlympus *) n);
}

static char *
exif_mnote_data_olympus_get_value (ExifMnoteData *d, unsigned int i, char *val, unsigned int maxlen)
{
	ExifMnoteDataOlympus *n = (ExifMnoteDataOlympus *) d;

	if (!d || !val) return NULL;
	if (i > n->count -1) return NULL;
	exif_log (d->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataOlympus",
		  "Querying value for tag '%s'...",
		  mnote_olympus_tag_get_name (n->entries[i].tag));
	return mnote_olympus_entry_get_value (&n->entries[i], val, maxlen);
}

static void
exif_mnote_data_olympus_save (ExifMnoteData *ne,
		unsigned char **buf, unsigned int *buf_size)
{
	ExifMnoteDataOlympus *n = (ExifMnoteDataOlympus *) ne;
	unsigned int i, o, s, doff, base = 0, o2 = 6;
	int datao = 0;

	if (!n || !buf || !buf_size) return;

	/*
	 * Allocate enough memory for all entries and the number of entries.
	 */
	*buf_size = 6 + 2 + 2 + n->count * 12;
	switch (n->version) {
	case 0: /* Olympus */
		*buf = malloc (*buf_size);
		if (!*buf) return;
		memset (*buf, 0, *buf_size);

		/* Write the header and the number of entries. */
		strcpy (*buf, "OLYMP");
		o2 += 2;
		datao = n->offset;
		break;
	case 1: /* Nikon v1 */
		base = MNOTE_NIKON1_TAG_BASE;
		*buf_size -= 8;
		/* Fall through */
	case 2: /* Nikon v2 */
		*buf_size += 8;
		*buf = malloc (*buf_size);
		if (!*buf) return;
		memset (*buf, 0, *buf_size);

		/* Write the header and the number of entries. */
		strcpy (*buf, "Nikon");
		(*buf)[6] = n->version;
		o2 += 2; *buf_size += 2;
		if (n->version == 2) {
			exif_set_short (*buf + 10, n->order, (ExifShort) ((n->order == EXIF_BYTE_ORDER_INTEL) ? 'II' : 'MM'));
			exif_set_short (*buf + 12, n->order, (ExifShort) 0x2A);
			exif_set_long (*buf + 14, n->order, (ExifShort) 8);
			o2 += 2 + 8;
		}
		datao = -10;
		break;
	}

	exif_set_short (*buf + o2, n->order, (ExifShort) n->count);
	o2 += 2;

	/* Save each entry */
	for (i = 0; i < n->count; i++) {
		o = o2 + i * 12;
		exif_set_short (*buf + o + 0, n->order,
				(ExifShort) (n->entries[i].tag - base));
		exif_set_short (*buf + o + 2, n->order,
				(ExifShort) n->entries[i].format);
		exif_set_long  (*buf + o + 4, n->order,
				n->entries[i].components);
		o += 8;
		s = exif_format_get_size (n->entries[i].format) *
						n->entries[i].components;
		if (s > 4) {
			doff = *buf_size;
			*buf_size += s;
			*buf = realloc (*buf, *buf_size);
			if (!*buf) return;
			exif_set_long (*buf + o, n->order, datao + doff);
		} else
			doff = o;

		/* Write the data. */
		if (n->entries[i].data) {
			memcpy (*buf + doff, n->entries[i].data, s);
		} else {
			/* Most certainly damaged input file */
			memset (*buf + doff, 0, s);
		}
	}
}

static void
exif_mnote_data_olympus_load (ExifMnoteData *en,
			      const unsigned char *buf, unsigned int buf_size)
{
	ExifMnoteDataOlympus *n = (ExifMnoteDataOlympus *) en;
	ExifShort c;
	unsigned int i, s, o, o2 = 0, datao = 6, base = 0;

	if (!n || !buf) return;

	/* Start of interesting data */
	o2 = 6 + n->offset;

	/*
	 * Olympus headers start with "OLYMP" and need to have at least
	 * a size of 22 bytes (6 for 'OLYMP', 2 other bytes, 2 for the
	 * number of entries, and 12 for one entry.
	 *
	 * Nikon headers start with "Nikon" (6 bytes including '\0'), 
	 * version number (1 or 2).
	 * 
	 * Version 1 continues with 0, 1, 0, number_of_tags,
	 * or just with number_of_tags (models D1H, D1X...).
	 * 
	 * Version 2 continues with an unknown byte (0 or 10),
	 * two unknown bytes (0), "MM" or "II", another byte 0 and 
	 * lastly 0x2A.
	 */
	if (buf_size - n->offset < 22) return;
	if (!memcmp (buf + o2, "OLYMP", 5)) {
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataOlympus",
			"Parsing Olympus maker note...");

		/* The number of entries is at position 8. */
		n->version = 0;
		o2 += 8;

	} else if (!memcmp (buf + o2, "Nikon", 6)) {
		o2 += 6;
		exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteDataOlympus",
			"Parsing Nikon maker note (0x%02x, %02x, %02x, "
			"%02x, %02x, %02x, %02x, %02x, %02x)...",
			buf[o2 + 0], buf[o2 + 1], buf[o2 + 2], buf[o2 + 3], 
			buf[o2 + 4], buf[o2 + 5], buf[o2 + 6], buf[o2 + 7]); 

		/* The first byte is the version. */
		if (o2 >= buf_size) return;
		n->version = buf[o2];
		o2 += 1;

		/* Skip an unknown byte (00 or 0A). */
		o2 += 1;

		switch (n->version) {
		case 1:

			base = MNOTE_NIKON1_TAG_BASE;
			break;

		case 2:

			/* Skip 2 unknown bytes (00 00). */
			o2 += 2;

			/*
			 * Byte order. From here the data offset
			 * gets calculated.
			 */
			datao = o2;
			if (o2 >= buf_size) return;
			if (!strncmp (&buf[o2], "II", 2))
				n->order = EXIF_BYTE_ORDER_INTEL;
			else if (!strncmp (&buf[o2], "MM", 2))
				n->order = EXIF_BYTE_ORDER_MOTOROLA;
			else {
				exif_log (en->log, EXIF_LOG_CODE_DEBUG,
					"ExifMnoteDatalympus", "Unknown "
					"byte order '%c%c'", buf[o2],
					buf[o2 + 1]);
				return;
			}
			o2 += 2;

			/* Skip 2 unknown bytes (00 2A). */
			o2 += 2;

			/* Go to where the number of entries is. */
			if (o2 >= buf_size) return;
			o2 = datao + exif_get_long (buf + o2, n->order);
			break;

		default:
			exif_log (en->log, EXIF_LOG_CODE_DEBUG,
				"ExifMnoteDataOlympus", "Unknown version "
				"number %i.", n->version);
			return;
		}
	} else if (!memcmp (buf + o2, "\0\x1b", 2)) {
		n->version = 2;
	} else {
		return;
	}

	/* Number of entries */
	if (o2 >= buf_size) return;
	c = exif_get_short (buf + o2, n->order);
	o2 += 2;

	/* Read the number of entries and remove old ones. */
	exif_mnote_data_olympus_clear (n);

	n->entries = malloc (sizeof (MnoteOlympusEntry) * c);
	if (!n->entries) return;
	memset (n->entries, 0, sizeof (MnoteOlympusEntry) * c);

	/* Parse the entries */
	for (i = 0; i < c; i++) {
	    o = o2 + 12 * i;
	    if (o + 12 > buf_size) return;

	    n->count = i + 1;
	    n->entries[i].tag        = exif_get_short (buf + o, n->order) + base;
	    n->entries[i].format     = exif_get_short (buf + o + 2, n->order);
	    n->entries[i].components = exif_get_long (buf + o + 4, n->order);
	    n->entries[i].order      = n->order;

	    exif_log (en->log, EXIF_LOG_CODE_DEBUG, "ExifMnoteOlympus",
		      "Loading entry 0x%x ('%s')...", n->entries[i].tag,
		      mnote_olympus_tag_get_name (n->entries[i].tag));

	    /*
	     * Size? If bigger than 4 bytes, the actual data is not
	     * in the entry but somewhere else (offset).
	     */
	    s = exif_format_get_size (n->entries[i].format) *
		   			 n->entries[i].components;
	    if (!s) continue;
	    o += 8;
	    if (s > 4) o = exif_get_long (buf + o, n->order) + datao;
	    if (o + s > buf_size) continue;

	    /* Sanity check */
	    n->entries[i].data = malloc (s);
	    if (!n->entries[i].data) continue;
	    n->entries[i].size = s;
	    memcpy (n->entries[i].data, buf + o, s);
	}
}

static unsigned int
exif_mnote_data_olympus_count (ExifMnoteData *n)
{
	return n ? ((ExifMnoteDataOlympus *) n)->count : 0;
}

static unsigned int
exif_mnote_data_olympus_get_id (ExifMnoteData *d, unsigned int n)
{
	ExifMnoteDataOlympus *note = (ExifMnoteDataOlympus *) d;

	if (!note) return 0;
	if (note->count <= n) return 0;
	return note->entries[n].tag;
}

static const char *
exif_mnote_data_olympus_get_name (ExifMnoteData *d, unsigned int i)
{
	ExifMnoteDataOlympus *n = (ExifMnoteDataOlympus *) d;

	if (!n) return NULL;
	if (i >= n->count) return NULL;
	return mnote_olympus_tag_get_title (n->entries[i].tag);
}

static const char *
exif_mnote_data_olympus_get_title (ExifMnoteData *d, unsigned int i)
{
	ExifMnoteDataOlympus *n = (ExifMnoteDataOlympus *) d;
	
	if (!n) return NULL;
	if (i >= n->count) return NULL;
        return mnote_olympus_tag_get_title (n->entries[i].tag);
}

static const char *
exif_mnote_data_olympus_get_description (ExifMnoteData *d, unsigned int i)
{
	ExifMnoteDataOlympus *n = (ExifMnoteDataOlympus *) d;
	
	if (!n) return NULL;
	if (i >= n->count) return NULL;
        return mnote_olympus_tag_get_title (n->entries[i].tag);
}

static void
exif_mnote_data_olympus_set_byte_order (ExifMnoteData *d, ExifByteOrder o)
{
	ExifByteOrder o_orig;
	ExifMnoteDataOlympus *n = (ExifMnoteDataOlympus *) d;
	unsigned int i, fs;
	ExifShort s;
	ExifLong l;
	ExifSLong sl;
	ExifRational r;
	ExifSRational sr;

	if (!n) return;

	o_orig = n->order;
	n->order = o;
	for (i = 0; i < n->count; i++) {
		n->entries[i].order = o;
		fs = exif_format_get_size (n->entries[i].format);
		switch (n->entries[i].format) {
		case EXIF_FORMAT_SHORT:
			for (i = 0; i < n->entries[i].components; i++) {
				s = exif_get_short (n->entries[i].data + (i*fs),
						o_orig);
				exif_set_short (n->entries[i].data + (i * fs),
						o, s);
			}
			break;
		case EXIF_FORMAT_LONG:
			for (i = 0; i < n->entries[i].components; i++) {
				l = exif_get_long (n->entries[i].data + (i*fs),
						o_orig);
				exif_set_long (n->entries[i].data + (i * fs),
						o, l);
			}
			break;
		case EXIF_FORMAT_RATIONAL:
			for (i = 0; i < n->entries[i].components; i++) {
				r = exif_get_rational (n->entries[i].data +
						(i * fs), o_orig);
				exif_set_rational (n->entries[i].data +
						(i * fs), o, r);
			}
			break;
		case EXIF_FORMAT_SLONG:
			for (i = 0; i < n->entries[i].components; i++) {
				sl = exif_get_slong (n->entries[i].data +
						(i * fs), o_orig);
				exif_set_slong (n->entries[i].data +
						(i * fs), o, sl);
			}
			break;
		case EXIF_FORMAT_SRATIONAL:
			for (i = 0; i < n->entries[i].components; i++) {
				sr = exif_get_srational (n->entries[i].data +
						(i * fs), o_orig);
				exif_set_srational (n->entries[i].data +
						(i * fs), o, sr);
			}
			break;
		case EXIF_FORMAT_UNDEFINED:
		case EXIF_FORMAT_BYTE:
		case EXIF_FORMAT_ASCII:
		default:
			/* Nothing here. */
			break;
		}
	}
}

static void
exif_mnote_data_olympus_set_offset (ExifMnoteData *n, unsigned int o)
{
	if (n) ((ExifMnoteDataOlympus *) n)->offset = o;
}

ExifMnoteData *
exif_mnote_data_olympus_new (void)
{
	ExifMnoteData *d;

	d = calloc (1, sizeof (ExifMnoteDataOlympus));
	if (!d) return NULL;

	exif_mnote_data_construct (d);

	/* Set up function pointers */
	d->methods.free            = exif_mnote_data_olympus_free;
	d->methods.set_byte_order  = exif_mnote_data_olympus_set_byte_order;
	d->methods.set_offset      = exif_mnote_data_olympus_set_offset;
	d->methods.load            = exif_mnote_data_olympus_load;
	d->methods.save            = exif_mnote_data_olympus_save;
	d->methods.count           = exif_mnote_data_olympus_count;
	d->methods.get_id          = exif_mnote_data_olympus_get_id;
	d->methods.get_name        = exif_mnote_data_olympus_get_name;
	d->methods.get_title       = exif_mnote_data_olympus_get_title;
	d->methods.get_description = exif_mnote_data_olympus_get_description;
	d->methods.get_value       = exif_mnote_data_olympus_get_value;

	return d;
}
