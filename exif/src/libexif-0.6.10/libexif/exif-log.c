/* exif-log.c
 *
 * Copyright � 2004 Lutz M�ller <lutz@users.sourceforge.net>
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
#include <libexif/exif-log.h>

#include <stdlib.h>
#include <string.h>

struct _ExifLog {
	unsigned int ref_count;

	ExifLogFunc func;
	void *data;
};

ExifLog *
exif_log_new (void)
{
	ExifLog *log;

	log = malloc (sizeof (ExifLog));
	if (!log) return NULL;
	memset (log, 0, sizeof (ExifLog));
	log->ref_count = 1;

	return log;
}

void
exif_log_ref (ExifLog *log)
{
	if (!log) return;
	log->ref_count++;
}

void
exif_log_unref (ExifLog *log)
{
	if (!log) return;
	if (log->ref_count > 0) log->ref_count--;
	if (!log->ref_count) exif_log_free (log);
}

void
exif_log_free (ExifLog *log)
{
	if (!log) return;
	memset (log, 0, sizeof (ExifLog));
	free (log);
}

void
exif_log_set_func (ExifLog *log, ExifLogFunc func, void *data)
{
	if (!log) return;
	log->func = func;
	log->data = data;
}

void
exif_log (ExifLog *log, ExifLogCode code, const char *domain,
	  const char *format, ...)
{
	va_list args;

	va_start (args, format);
	exif_logv (log, code, domain, format, args);
	va_end (args);
}

void
exif_logv (ExifLog *log, ExifLogCode code, const char *domain,
	   const char *format, va_list args)
{
	if (!log) return;
	if (!log->func) return;
	log->func (log, code, domain, format, args, log->data);
}
