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
CFILES = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(CFILES))

CC = gcc
CPPFLAGS = -std=c99 -O3
CFLAGS = -std=c99

DESTDIR = ~/.local/bin

build/ :
	mkdir -p build/

$(DESTDIR) : 
	mkdir -p $(DESTDIR)/

$(OBJS) : build/%.o : src/%.c build/ $(HEADERS)
	$(CC) $(CPPFLAGS) -c $< -o $@

bfcli : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o bfcli $(LDLIBS)

.DEFAULT_GOAL = all
.PHONY : all clean install remove

all : bfcli

clean :
	-rm -r build/
	-rm bfcli

install : bfcli $(DESTDIR)/
	cp bfcli $(DESTDIR)/

remove :
	-rm $(DESTDIR)/bfcli