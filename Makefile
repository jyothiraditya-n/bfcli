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

CFILES = $(shell find src/ -name "*.c")
OBJS = $(patsubst %.c,%.o,$(CFILES))
LIBS = libClame/libClame.a

DBFFILES = $(wildcard demo/*.bf)
DSFILES = $(patsubst demo/%.bf,%.s,$(DBFFILES))
DOBJS = $(patsubst demo/%.bf,%.o,$(DBFFILES))
DEMOS = $(patsubst demo/%.bf,%,$(DBFFILES))

CC = gcc
AS = as
LD = ld

CPPFLAGS = -Wall -Wextra -Werror -std=c99 -O3 -I libClame/inc/
CFLAGS = -std=c99 -s
LDLIBS += -L libClame/ -lClame

DLFLAGS += -s

DESTDIR ?= ~/.local/bin

files = $(wildcard bfcli)
files += $(foreach obj,$(OBJS),$(wildcard $(obj)))

files += $(foreach file,$(DSFILES),$(wildcard $(file)))
files += $(foreach obj,$(DOBJS),$(wildcard $(obj)))
files += $(foreach demo,$(DEMOS),$(wildcard $(demo)))

CLEAN = $(foreach file,$(files),rm $(file);)

$(DESTDIR) : 
	mkdir -p $(DESTDIR)/

$(OBJS) : %.o : %.c $(HEADERS)
	$(CC) $(CPPFLAGS) -c $< -o $@

libClame/libClame.a :
	+cd libClame; $(MAKE) libClame.a

bfcli : $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(OBJS) -o bfcli $(LDLIBS)

$(DSFILES) : %.s : demo/%.bf bfcli
	./bfcli -sOs $< -o $@

kingdom.s : demo/kingdom.bf bfcli
	./bfcli -sO2 demo/kingdom.bf -o kingdom.s
	# BUGBUG -O3 causes it to not work correctly.

$(DOBJS) : %.o : %.s
	$(AS) $(DSFLAGS) $< -o $@

$(DEMOS) : % : %.o
	$(LD) $(DLFLAGS) $< -o $@

.DEFAULT_GOAL = all
.PHONY : all clean demos global install remove
.PHONY : _demos _translate_demos

all : bfcli

clean :
	cd libClame; make clean
	$(CLEAN)

demos : bfcli
	+make -j1 _translate_demos
	+make _demos

global : install
	-sudo rm /bin/bfcli /bin/bf /bin/bfc
	sudo ln -s $(DESTDIR)/bfcli /bin/bfcli
	sudo ln -s $(DESTDIR)/bfcli /bin/bf
	sudo ln -s $(DESTDIR)/bfcli /bin/bfc

install : bfcli $(DESTDIR)/
	cp bfcli $(DESTDIR)/

remove :
	-rm $(DESTDIR)/bfcli

_demos : $(DEMOS)

_translate_demos : $(DSFILES)