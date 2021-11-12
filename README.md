# Bfcli: The Interactive Brainfuck Command-Line Interpreter
```
Copyright (C) 2021 Jyothiraditya Nellakra
Version 8.1: The Pursuit of Esotericism

bfcli:0%
```

---

I made this program as a challenge to myself in incorporating all the features
I felt would be needed to make an interactive programming tool for command-line
use. In this sense, the choice of the programming language being Brainfuck was
mainly inspired by its relative simplicity; each command is a character and the
only grammar to parse is to ensure the number of brackets match up.

Nevertheless, incorporating necessary functionality was rather difficult, and
despite my intention to make this program easy to port across multiple
platforms, I realised early on that many of my plans would require libraries
beyond the cstdlib. Hence, this program was written with an increasing emphasis
on UNIX and Linux as development continued.

The following is a non-exhaustive list of the functionality I added:

- A prompt that allows you enter a line of Brainfuck to be run immediately,
accompanied by a secondary prompt for continuing the line, invoked when the
original line contained unmatched brackets or an unclosed brace.

- The use of `!` and `;` to hint to the interpreter that the user is not yet
finished writing a block of code and that the user is done writing a block of
code, respectively. (Notably, the block still isn't actually finished until all
of the matching brackets have been written out.)

- The command `?` to pull up a help dialogue for both the Brainfuck programming
language as well as the functionality of this program at the main prompt.

- The command `/` to clear the memory and reset the pointer to zero.

- The commands `*` and `&` to print a memory listing of the values around the
current pointer, and produce a complete memory dump to aid with debugging of
code.

- The command `$` compiles the program to object code and then disassembles it.
This makes it easier to read the logic than pure brainfuck since it replaces
the endless stream of pluses, minuses and arrows with opcodes and numeric
values.

- The configurable disabling of `STDIN` buffering when running Brainfuck code
and alternative behaviour for ^C, either exiting the program if called at the
main prompt, clearing the line of code if called at the secondary prompt, or
stopping the execution of the running Brainfuck program.

- Integration with `libClame` to provide command-line argument parsing, which
is used for providing program information with command line switches,
specifying if colour output should be enabled, and pre-emptively declaring the
file to be loaded into memory. This library is also used as the backbone for
the command-line itself.

- Colour support for highlighting important information on the screen, as well
as a monochrome, non-ANSI, and printer-friendly 'minimal' mode.

- Loading valid Brainfuck files at the prompt, performing code buffer editing
with `%` and executing them with `@`. (Again, thanks to intergration with
`libClame`.)

- Transpiling Brainfuck code to C source code with output to either STDOUT or a
custom output file, as specified on the command line, as well as partial
compilation to Amd64 assembly to boost compilation performance while offering
similar performance to GCC's `-O3`.

## Building, Running and Installing the Program from Source

You will need to have an up-to-date version of GNU C Compiler and GNU Make. You
can compile the code by running `make` at the command-line after cd'ing into
the source code folder.

Then, to start the compiled program, you can then run `./bfcli`. Some example
Brainfuck code is provided in the `demo` folder if you wish to try to them
out.

The following are the command-line arguments that this program accepts:

```
Usage: bf [ARGS] [FILE]

Valid arguments are:
  -a, --about       Prints the licence and about dialogue.
  -h, --help        Prints the help dialogue.
  -v, --version     Prints the program version.

  -c, --colour      Enables colour output.
  -n, --no-ansi     Disables the use of ANSI escape sequences.
  -m, --minimal     Disables Brainfuck extensions.

Note: Colour support and use of ANSI escape sequences is enabled
      by default.

Note: When in Minimal Mode, it's you and the original Brainfuck
      language, and that's it. All of the extensions of interactive
      mode are disabled.

  -f, --file FILE   Loads the file FILE into memory.
  -l, --length LEN  Sets the shell's code buffer length to LEN.

Note: If a file is specified without -f, it is run immediately and
      the program exits as soon as the execution of the file
      terminates. Use -f if you want the interactive prompt.

Note: If a file is specified with -f, the code buffer's length is
      set to LEN plus the file's length.

  -o, --output OUT  Sets the output file for the transpiled C
                    code and the memory dump to OUT.

  -r, --ram SIZE    Sets the total memory size for the compiled
                    program to SIZE.

  -s, --safe-code   Generates code that won't segfault if < or
                    > are used out-of-bounds. (The pointer wraps
                    around.)

  -t, --transpile   Transpiles the file to C source code, ouputs
                    the result to OUT and exits.

  -x, --assembly    Generates assembly code intermixed with the C
                    output. This option affords both high performance
                    and fast compile times, however it only works on
                    amd64-based computers.

  -d, --direct-inp  Don't buffer the standard input. Send characters
                    to Brainfuck code without waiting for a newline.

Note: If no output file is specified, the transpiled code is output
      to STDOUT. Code generated with -s may be both slower to compile
      and execute, so only use it when necessary.

Happy coding! :)
```

TL;DR:

```
Usage: bf [ARGS] [FILE]

Valid arguments are:

  -a, --about      | -h, --help       | -v, --version
  -c, --colour     | -m, --minimal    | -n, --no-ansi
  -d, --direct-inp |

  -f, --file FILE  | -l, --length LEN | -r, --ram SIZE
  -x, --assemble   | -s, --safe-code  | -t, --transpile
  -o, --output OUT |

Happy coding! :)
```

The following is the help dialogue for the interactive functionality:

```
Interactive Brainfuck interpreter; exit with ^C.

Brainfuck commands:
  > Increments the data pointer.
  < Decrements the data pointer.

  + Increments the value at the data pointer.
  - Decrements the value at the data pointer.

Note: Data values are modulo-256 unsigned integers, meaning
      0 - 1 = 255, and 255 + 1 = 0.

  . Outputs the value at the data pointer as an ASCII character.
  , Inputs an ASCII character and stores its value at the data
    pointer.

  [ (Open bracket) begins a loop.
  ] (Close brace) ends a loop.

Note: Loops run while the value at the data pointer is non-zero.

Extended Brainfuck commands:
  ? Prints the help and copyright disclaimer to the console.
  / Clears the memory and moves the pointer to 0.
  * Prints memory values around the current pointer value.
  & Prints all memory values.

Note: When ANSI support is enabled, & pauses at the end of the
      first screen of text and displays a the prompt ':'. Here,
      you can type any key to advance the memory dump by one
      line, enter to advance it by one page, or tab to complete
      the rest of the dump without pausing again.

Note: When an output file is specified with -o, the memory dump
      is redirected to that file instead of being displayed on
      the console.

Note: Extended Brainfuck commands are disabled when executing file
      code, and will simply be ignored. This is done for
      compatibility with vanilla Brainfuck programs.

  ! Indicates to wait for more code before execution.
  ; Indicates to stop waiting for more code before execution.

Note: The above two commands can be placed anywhere in a line and
      and will function correctly, but they may prove most useful
      at the ends of lines while typing long sections of code.

Note: The interpreter will still wait for more code if the current
      code contains unmatched brackets.

  @ Executes code from the code buffer.
  % Edits code in the code buffer.
  $ Disassembles the object code generated for the code buffer.

Note: In order to load a file when Bfcli is running, type the file
      name at the main prompt. When files are loaded, they are put
      into the code buffer.

Note: Buffer Editing functionality is disabled when the use of ANSI
      escape sequences is disabled. % will simply print the contents
      of the code buffer in this case.
```

Finally, to install the code, you can run `make install`. This will install it
to `~/.local/bin` where `~` is your user's home folder. If you wish to change
this location, you can specify a new one with `DESTDIR=<location> make install`.
However, you may need to run the command with elevated privileges if installing
to a system folder like `/bin`.

Although, if you want to also use this as the default Brainfuck interpreter on
your system, you can run `make bf` to symlink `(your install location)/bfcli`
to `/bin/bfcli`.

## Contributing

If you happen to find any bugs, or want to contribute some code of your own,
feel free to create an issue or pull request.

Additionally, if you wish to contact me, feel free to mail me at
[jyothiraditya.n@gmail.com](mailto:jyothiraditya.n@gmail.com).

---

Happy Coding!
