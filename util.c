// Utility functions
// $Id: util.c 2 2008-02-11 02:48:07Z eric $
// Copyright 1995, 2004, 2005, 2006, 2008 Eric Smith <eric@brouhaha.com>

// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3 as
// published by the Free Software Foundation.  Note that I am not
// granting permission to redistribute or modify this program under the
// terms of any other version of the General Public License.

// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program (in the file "COPYING"); if not, see
// <http://www.gnu.org/licenses/>.


#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"


char *progname;


// generate fatal error message to stderr, doesn't return
void fatal (int ret, char *format, ...)
{
  va_list ap;

  if (format)
    {
      fprintf (stderr, "fatal error: ");
      va_start (ap, format);
      vfprintf (stderr, format, ap);
      va_end (ap);
      fprintf (stderr, "\n");
    }
  if (ret == 1)
    {
      fprintf (stderr, "usage:\n");
      usage (stderr);
    }
  exit (ret);
}


void *alloc (size_t size)
{
  void *p;

  p = calloc (1, size);
  if (! p)
    fatal (2, "Memory allocation failed\n");
  return (p);
}


char *newstr (char *orig)
{
  int len;
  char *r;

  len = strlen (orig);
  r = (char *) alloc (len + 1);
  memcpy (r, orig, len + 1);
  return (r);
}


char *newstrn (char *orig, int max_len)
{
  int len;
  char *r;

  len = strlen (orig);
  if (len > max_len)
    len = max_len;
  r = (char *) alloc (len + 1);
  memcpy (r, orig, len);
  return (r);
}

int keyword (char *string, keyword_t *table)
{
  while (table->name)
    {
      if (strcasecmp (string, table->name) == 0)
	return table->value;
      table++;
    }
  return 0;
}

intmax_t a_to_intmax (char *s)
{
  intmax_t val;
  int cc;

  int i = sscanf (s, "%" SCNiMAX "%n", & val, & cc);
  if ((i != 1) || (cc != strlen (s)))
    fatal (2, "can't interpret '%s' as an integer", s);

  return val;
}

