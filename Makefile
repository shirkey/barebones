# Makefile for Bare Bones interpreter
# $Id: Makefile 2 2008-02-11 02:48:07Z eric $
# Copyright 2008 Eric Smith <eric@brouhaha.com>

# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.  Note that I am not
# granting permission to redistribute or modify this program under the
# terms of any other version of the General Public License.

# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program (in the file "COPYING"); if not, see
# <http:#www.gnu.org/licenses/>.

CFLAGS = -g -Wall -Wextra

YACC = bison
YFLAGS = -d -v -t

LEX = flex
LFLAGS =
# Use -d flag for debug:
# LFLAGS = -d

LDFLAGS = -g
LOADLIBES = -lm

%.tab.c %.tab.h %.output: %.y
	$(YACC) $(YFLAGS) $<

barebones: barebones.o parser.tab.o scanner.o util.o

barebones.o: barebones.c barebones.h parser.tab.h util.h

util.o: util.c util.h

scanner.o: scanner.l barebones.h util.h parser.tab.h

parser.tab.o: parser.y barebones.h
