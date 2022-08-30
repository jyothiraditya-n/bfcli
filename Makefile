# Bfcli: The Interactive Brainfuck Command-Line Interpreter
# Copyright (C) 2021-2022 Jyothiraditya Nellakra
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

CPPFLAGS = -Wall -Wextra -std=gnu99 -O3 -I libClame/inc/
CFLAGS = -std=gnu99 -s
LDLIBS += -L libClame/ -lClame -lpthread

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

specials += kingdom.s euler.s hanoi.s mandelbrot.s

$(filter-out $(specials),$(DSFILES)) : %.s : demo/%.bf bfcli
	./bfcli -sOa $< -o $@

kingdom.s : demo/kingdom.bf bfcli
	./bfcli -sOp demo/kingdom.bf -o kingdom.s

euler.s : demo/euler.bf bfcli
	./bfcli -sOs demo/euler.bf -o euler.s

hanoi.s : demo/hanoi.bf bfcli
	./bfcli -sOs demo/hanoi.bf -o hanoi.s

mandelbrot.s : demo/mandelbrot.bf bfcli
	./bfcli -sOs demo/mandelbrot.bf -o mandelbrot.s

$(DOBJS) : %.o : %.s
	$(AS) $(DSFLAGS) $< -o $@

$(DEMOS) : % : %.o
	$(LD) $(DLFLAGS) $< -o $@

.DEFAULT_GOAL = all
.PHONY : all clean demos global install remove
.PHONY : _demos _translate_demos

all : bfcli

clean :
	cd libClame; $(MAKE) clean
	$(CLEAN)

demos : bfcli
	+$(MAKE) -j1 _translate_demos
	+$(MAKE) _demos

global : install
	-sudo rm /bin/bfcli /bin/bf /bin/bfc
	sudo ln -s $(DESTDIR)/bfcli /bin/bfcli
	sudo ln -s $(DESTDIR)/bfcli /bin/bf
	sudo ln -s $(DESTDIR)/bfc /bin/bfc

install : $(DESTDIR)/ bfcli script/bfc.sh
	cp bfcli $(DESTDIR)/
	cp script/bfc.sh $(DESTDIR)/bfc

remove :
	-rm $(DESTDIR)/bfcli $(SCRIPTS)

_demos : $(DEMOS)

_translate_demos : $(DSFILES)