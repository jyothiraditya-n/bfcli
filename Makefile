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

BFFILES = $(wildcard demo/*.bf)
DEMOS = $(patsubst demo/%.bf,%,$(BFFILES))

CC = gcc
CPPFLAGS = -Wall -Wextra -Werror -std=c99 -O3 -I libClame/inc/
DCFLAGS = -Wall -Wextra -Werror -std=c89 -Ofast -xc
CFLAGS = -std=c99
LDLIBS += -L libClame/ -lClame

DESTDIR = ~/.local/bin

files = $(foreach obj,$(OBJS),$(wildcard $(obj)))
files += $(wildcard bfcli) $(foreach demo,$(DEMOS),$(wildcard $(demo)))
CLEAN = $(foreach file,$(files),rm $(file);)

$(DESTDIR) : 
	mkdir -p $(DESTDIR)/

$(OBJS) : %.o : %.c $(HEADERS)
	$(CC) $(CPPFLAGS) -c $< -o $@

libClame/libClame.a :
	+cd libClame; $(MAKE) libClame.a

bfcli : $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(OBJS) -o bfcli $(LDLIBS)

$(DEMOS) : % : demo/%.bf bfcli
	./bfcli -t $< | $(CC) $(DCFLAGS) - -o $@

.DEFAULT_GOAL = all
.PHONY : all bf clean demos install remove

all : bfcli

bf : install
	-sudo rm /bin/bf
	sudo ln -s $(DESTDIR)/bfcli /bin/bf

clean :
	cd libClame; make clean
	$(CLEAN)

demos : $(DEMOS)

install : bfcli $(DESTDIR)/
	cp bfcli $(DESTDIR)/

remove :
	-rm $(DESTDIR)/bfcli