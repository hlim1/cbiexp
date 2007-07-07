/* unixcrypt.c: functions to simulate old "unix crypt" program */
/* $Id: unixcrypt.c,v 1.1 2004/04/27 20:30:06 liblit Exp $ */

/* WARNING: do not use this software for encryption! The encryption
   provided by this program has been broken and is not secure. Only
   use this software to decrypt existing data. */

#define _XOPEN_SOURCE
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <config.h>    /* generated by configure */
#include "unixcrypt.h"
#include "io.h"

#ifndef HAVE_LIBCRYPT    /* use the dropin replacement for crypt(3) */
  #include "unixcrypt3.h"
  #define crypt crypt_replacement
#endif

/* The library function crypt(3) is not well-defined if the salt
   characters are not within the range [0-9A-Za-z./]. In fact, this
   function behaves differently on the reference SunOS system than on
   a glibc system. Since crypt(1) invokes crypt(3) with arbitrary salt
   characters, we need to reproduce the behavior of Sun's crypt(3).
   Here we only call crypt(3) with legal salt values, so this should
   be portable. */

#define ascii_to_bin_sun(c) ((c)>'Z'?(c-59):(c)>'9'?((c)-53):(c)-'.')
#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

char *crypt_sun(const char *key, const char *salt) {
  char salt1[2];
  static char result[13];
  char *p;
  int tmp;

  tmp = 0x3f & ascii_to_bin_sun(salt[0]);
  salt1[0] = bin_to_ascii(tmp);
  tmp = 0x3f & ascii_to_bin_sun(salt[1]);
  salt1[1] = bin_to_ascii(tmp);

  p = crypt(key, salt1);
  strncpy(result, p, 13);

  result[0] = salt[0];
  result[1] = salt[1];

  return result;
}

/* initialize state of encryption engine */

void unixcrypt_init(unixcrypt_state *st, char *key) {
  signed char buf[16];
  int i, j, k, acc, tmp, v;
  char *p;

  memset(buf, 0, 16);
  strncpy(buf, key, 8);

  buf[8] = buf[0];
  buf[9] = buf[1];
  p = crypt_sun(buf, &buf[8]);

  strncpy(buf, p, 13);

  acc = 0x7b;

  for (i=0; i<13; i++) {
    acc = i + acc * buf[i];
  }

  for (i = 0; i < 0x100; i++) {
    st->box1[i] = i;
    st->box2[i] = 0;
  }

  for (i = 0; i < 0x100; i++) {
    acc *= 5;
    acc += buf[i % 13];

    v = acc - 0xfff1 * (acc / 0xfff1);

    j = (v & 0xff) % (0x100-i);
    k = 0xff - i;

    tmp = st->box1[k];
    st->box1[k] = st->box1[j];
    st->box1[j] = tmp;

    if (st->box2[k]==0) {
      j = ((v >> 8) & 0xff) % k;

      while (st->box2[j] != 0) {
	j = (j+1) % k;
      }

      st->box2[k] = j;
      st->box2[j] = k;
    }
  }

  /* calculate box3, the inverse of box1 */
  for (i = 0; i < 0x100; i++) {
    st->box3[st->box1[i] & 0xff] = i;
  }

  /* initialize character count */
  st->j = st->k = 0;
}

/* encrypt/decrypt one character */

int unixcrypt_char(unixcrypt_state *st, int c) {
  c += st->k;
  c = st->box1[c & 0xff];
  c += st->j;
  c = st->box2[c & 0xff];
  c -= st->j;
  c = st->box3[c & 0xff];
  c -= st->k;

  st->k++;
  if (st->k == 0x100) {
    st->j++;
    st->k = 0;
    if (st->j == 0x100) {
      st->j = 0;
    }
  }
  return c;
}

/* unixcrypt: decrypt stream/file using the old unix "crypt" format. */

int unixcrypt(reader *r, writer *w, char *keyword) {
  unixcrypt_state uc_state;
  int c;
  int st;

  /* initialize state of unix crypt */
  unixcrypt_init(&uc_state, keyword);

  /* decrypt */
  c = r->bgetc(r);
  while (c != EOF) {
    c = unixcrypt_char(&uc_state, c);
    st = w->bputc(c, w);
    if (st<0) return -1;
    c = r->bgetc(r);
  }
  if (errno) 
    return -1;

  return w->beof(w);
}

/* specialize to file */

int unixcrypt_file(int fd, char *filename, char *keyword) {
  readwriter *rw = new_file_readwriter(fd, filename);
  reader *r = (reader *)rw;
  writer *w = (writer *)rw;
  int st = unixcrypt(r, w, keyword);
  free(rw);
  return st;
}

/* specialize to stream */

int unixcrypt_streams(FILE *fin, FILE *fout, char *keyword) {
  reader *r = new_stream_reader(fin);
  writer *w = new_stream_writer(fout);
  int st = unixcrypt(r, w, keyword);
  free(r);
  free(w);
  return st;
}

