#! /bin/bash

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

print_about() {
	echo ""
	echo "  Bfcli: The Interactive Brainfuck Command-Line Interpreter"
	echo "  Copyright (C) 2021-2022 Jyothiraditya Nellakra"
	echo "  Version 10.5: A Friend's Rice Noodles"
	echo ""
	echo "  This program is free software: you can redistribute it and/or modify"
	echo "  it under the terms of the GNU General Public License as published by"
	echo "  the Free Software Foundation, either version 3 of the License, or"
	echo "  (at your option) any later version."
	echo ""
	echo "  This program is distributed in the hope that it will be useful,"
	echo "  but WITHOUT ANY WARRANTY; without even the implied warranty of"
	echo "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the"
	echo "  GNU General Public License for more details."
	echo ""
	echo "  You should have received a copy of the GNU General Public License"
	echo "  along with this program. If not, see <https://www.gnu.org/licenses/>."
	echo ""
}

print_help() {
	echo ""
	echo "  Usage: $(basename "$0") [OPTION] [ARGS]"
	echo ""
	echo "  Valid values for OPTION are:"
	echo ""
	echo "    -a, --about  Display the about dialogue."
	echo "    -h, --help   Display this help dialogue."
	echo ""
	echo "    -o, --output OUT  Specify the output file for the executable"
	echo ""
	echo "  Note: ARGS are supplied directly to \`bfcli -to -'; don't re-specify the output"
	echo "        manually or it'll error out. Since the output is piped into \`cc -xc -',"
	echo "        it must be C-compatible. Hence, use the \`-x' flag, not \`-s'."
	echo ""
	echo "  Happy coding! :)"
	echo ""
}

args="$@"
outfile="a.out"

if [ "$#" -le 2 ]; then
	print_help
	exit 1
fi

case "$1" in
"-a" | "--about")
	print_about
	exit 0;
;;

"-h" | "--help")
	print_help
	exit 0
;;

"-o" | "--output")
	if [ "$#" -le 4 ]; then
		print_help
		exit 1
	fi

	args="${@:3}"
	outfile="$2"
;;
esac

(set -x; bfcli -to - ${args[@]} | cc -xc - -o "$outfile")