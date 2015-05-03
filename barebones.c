// Bare Bones interpreter
// $Id: barebones.c 14 2008-02-19 19:34:41Z eric $
// Copyright 2008 Eric Smith <eric@brouhaha.com>

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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "barebones.h"
#include "parser.tab.h"

struct var_t
{
  struct var_t *next;
  char *name;
  bool init;
  uintmax_t val;
};

int stmt_line;  // line number of currently executing statement, for
		// error reporting purposes

bool init_to_zero = true;  // if false, reference to uninitialized variables
                           // (other than in a clear statement) will result
                           // in a run time error


void usage (FILE *f)
{
  fprintf (f, "%s [options]  [initializers...] src-file:\n", progname);
  fprintf (f, "options:\n");
  fprintf (f, "  -u            report uninitialized variables\n");
  fprintf (f, "  -O            optimize\n");
  fprintf (f, "initializers:\n");
  fprintf (f, "  var=value     e.g. X=37\n");
}


void error (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "error on line %d: ", stmt_line);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, "\n");
  exit (2);
}


// linked list of variables
// unsorted because it is assumed that there will only be a small number
// of variables, so the cost of looking them up by a linear search of a
// list at "compile time" will be negligible.
var_t *var_head = NULL;


var_t *find_var (char *name)
{
  var_t *var;

  // linear search for named variable
  for (var = var_head; var; var = var->next)
    {
      if (strcasecmp (name, var->name) == 0)
	return var;
    }

  // if not found, create
  var = alloc (sizeof (var_t));
  var->name = strdup (name);
  if (init_to_zero)
    {
      var->val = 0;
      var->init = true;
    }
  else
    {
      var->init = false;
    }
  var->next = var_head;
  var_head = var;
  return var;
}

void set_var (var_t *var, uintmax_t val)
{
  var->val = val;
  var->init = true;
}

static void define_var (char *s)
{
  var_t *var;
  char *p = strchr (s, '=');
  char *name = newstrn (s, p - s);
  intmax_t val;
  p++;

  val = a_to_intmax (p);
  if (val < 0)
    fatal (2, "negative values are not permitted");

  var = find_var (name);
  set_var (var, val);
}


void print_vars (bool show_uninitialized)
{
  var_t *var;

  for (var = var_head; var; var = var->next)
    {
      if ((! var->init) && (! show_uninitialized))
	continue;
      printf ("%s: ", var->name);
      if (var->init)
	printf ("%" PRIuMAX, var->val);
      else
	printf ("uninitialized");
      printf ("\n");
    }
}


void check_var_init (var_t *var)
{
  if (! var->init)
    error ("unitialized variable %s", var->name);
}


void execute_stmt (stmt_t *stmt)
{
  stmt_line = stmt->line;
  switch (stmt->type)
    {
    case CLEAR_STMT:
      stmt->var->val = 0;
      stmt->var->init = true;
      break;
    case INCR_STMT:
      check_var_init (stmt->var);
      if (stmt->var->val == UINTMAX_MAX)
	fatal (2, "overflow");
      stmt->var->val++;
      break;
    case DECR_STMT:
      check_var_init (stmt->var);
      if (stmt->var->val)
	stmt->var->val--;
      break;
    case WHILE_STMT:
      check_var_init (stmt->var);
      while (stmt->var->val)
	execute_stmt_list (stmt->stmt_list);
      break;
    case COPY_STMT:
      check_var_init (stmt->var);
      stmt->dest->val = stmt->var->val;
      stmt->dest->init = true;
      break;
    case ADD_CLEAR_STMT:
      check_var_init (stmt->var);
      check_var_init (stmt->dest);
      stmt->dest->val += stmt->var->val;
      stmt->var->val = 0;
      break;
    }
}


void execute_stmt_list (stmt_t *list)
{
  while (list)
    {
      execute_stmt (list);
      list = list->next;
    }
}


stmt_t *new_stmt (stmt_type_t type, var_t *var)
{
  stmt_t *stmt = alloc (sizeof (stmt_t));
  stmt->type = type;
  stmt->var = var;
  return stmt;
}


void add_stmt_to_list (stmt_t *head, stmt_t *stmt)
{
  if (head->tail)
    head->tail->next = stmt;
  else
    head->next = stmt;
  head->tail = stmt;
}


extern FILE *yyin;
extern int yydebug;
int yyparse (void);

bool parse_prog (char *fn)
{
  int status;
  yyin = fopen (fn, "r");
  if (! yyin)
    fatal (1, "can't read program\n");
#if 0
  yydebug = 1;
#endif
  status = yyparse ();
  fclose (yyin);
  return status == 0;
}


void optimize_stmt_list (stmt_t *list);


void optimize_while_stmt (stmt_t *stmt)
{
  bool match = false;
  var_t *loop_var = stmt->var;
  stmt_t *first_stmt = stmt->stmt_list;
  stmt_t *second_stmt = first_stmt->next;
  var_t *src;
  var_t *dest;
  if (first_stmt && second_stmt && ! second_stmt->next)
    {
      if ((first_stmt->type == INCR_STMT) &&
	  (first_stmt->var != loop_var) &&
	  (second_stmt->type == DECR_STMT) &&
	  (second_stmt->var == loop_var))
	{
	  match = true;
	  src = second_stmt->var;
	  dest = first_stmt->var;
	}
      else if ((second_stmt->type == INCR_STMT) &&
	       (second_stmt->var != loop_var) &&
	       (first_stmt->type == DECR_STMT) &&
	       (first_stmt->var == loop_var))
	{
	  match = true;
	  src = first_stmt->var;
	  dest = second_stmt->var;
	}
    }

  if (match)
    {
      stmt->type = ADD_CLEAR_STMT;
      stmt->var = src;
      stmt->dest = dest;
    }
  else
    optimize_stmt_list (stmt->stmt_list);
}


void optimize_stmt_list (stmt_t *list)
{
  while (list)
    {
      if (list->type == WHILE_STMT)
	optimize_while_stmt (list);
      list = list->next;
    }
}


stmt_t *main_prog = NULL;


int main (int argc, char *argv [])
{
  bool opt_flag = false;
  progname = argv [0];

  while (--argc)
    {
      argv++;
      if (argv [0][0] == '-')
	{
	  if (strcmp (argv [0], "-u") == 0)
	    init_to_zero = false;
	  else if (strcmp (argv [0], "-O") == 0)
	    opt_flag = true;
	  else
	    fatal (1, "unrecognized option '%s'", argv [0]);
	}
      else if (strchr (argv [0], '='))
	define_var (argv [0]);
      else if (main_prog)
	fatal (1, "only one program may be specified");
      else if (! parse_prog (argv [0]))
	fatal (2, "parse failed");
      else
	{
	  ;  // everything OK!
	}
    }

  if (! main_prog)
    fatal (1, "no program found");

  if (opt_flag)
    optimize_stmt_list (main_prog);

  printf ("initial values of variables:\n");
  print_vars (false);

  execute_stmt_list (main_prog);

  printf ("final values of variables:\n");
  print_vars (true);

  exit (0);
}
