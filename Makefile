# Bfcli: The Interactive Brainfuck Command-Line Interpreter
# Copyright (C) 2021 Jyothiraditya Nellakra
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.

HEADERS = $(wildcard src/*.h)
HEADERS += $(wildcard libClame/inc/*.h)

CFILES = $(wildcard src/*.c)
OBJS = $(patsubst %.c,%.o,$(CFILES))
LIBS = libClame/libClame.a

DBFFILES = $(wildcard demo/*.bf)
DCFILES = $(patsubst demo/%.bf,%.c,$(DBFFILES))
DEMOS = $(patsubst %.c,%,$(DCFILES))

EBFFILES = $(wildcard extra/*.bf)
ECFILES = $(patsubst extra/%.bf,%.c,$(EBFFILES))
EXTRAS = $(patsubst %.c,%,$(ECFILES))

CC = gcc
CPPFLAGS = -Wall -Wextra -Werror -std=c99 -O3 -I libClame/inc/
DCFLAGS = -Wall -Wextra -Werror -std=c89 -O3
ECFLAGS = -Wall -Wextra -Werror -std=gnu89 -O0
CFLAGS = -std=c99
LDLIBS += -L libClame/ -lClame

DESTDIR = ~/.local/bin

files = $(wildcard bfcli)
files += $(foreach obj,$(OBJS),$(wildcard $(obj)))
files += $(foreach file,$(DCFILES),$(wildcard $(file)))
files += $(foreach demo,$(DEMOS),$(wildcard $(demo)))
files += $(foreach file,$(ECFILES),$(wildcard $(file)))
files += $(foreach extra,$(EXTRAS),$(wildcard $(extra)))
CLEAN = $(foreach file,$(files),rm $(file);)

$(DESTDIR) : 
	mkdir -p $(DESTDIR)/

$(OBJS) : %.o : %.c $(HEADERS)
	$(CC) $(CPPFLAGS) -c $< -o $@

libClame/libClame.a :
	+cd libClame; $(MAKE) libClame.a

bfcli : $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(OBJS) -o bfcli $(LDLIBS)

$(DCFILES) : %.c : demo/%.bf bfcli
	./bfcli -t $< -o $@

$(DEMOS) : % : %.c
	$(CC) $(DCFLAGS) $< -o $@

$(ECFILES) : %.c : extra/%.bf bfcli
	./bfcli -xt $< -o $@

$(EXTRAS) : % : %.c
	$(CC) $(ECFLAGS) $< -o $@

.DEFAULT_GOAL = all
.PHONY : all bf clean demos extras install remove
.PHONY : _demos _transpile_demos
.PHONY : _specials _transpile_specials

all : bfcli

bf : install
	-sudo rm /bin/bf
	sudo ln -s $(DESTDIR)/bfcli /bin/bf

clean :
	cd libClame; make clean
	$(CLEAN)

demos : bfcli
	+make -j1 _transpile_demos
	+make _demos

extras : bfcli
	+make -j1 _transpile_demos _transpile_specials
	+make _demos _specials

install : bfcli $(DESTDIR)/
	cp bfcli $(DESTDIR)/

remove :
	-rm $(DESTDIR)/bfcli

_demos : $(DEMOS)

_transpile_demos : $(DCFILES)

_specials : $(EXTRAS)

_transpile_specials : $(ECFILES)