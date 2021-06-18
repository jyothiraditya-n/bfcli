# Bfcli: The Interactive Brainfuck Command-Line Interpreter
```
Copyright (C) 2021 Jyothiraditya Nellakra
Version 6.1: Cherrybreeze

bfcli@data:0$
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

- The command `*` to print a memory listing of the values around the current
pointer to aid with debugging of code.

- The disabling of `STDIN` buffering when running Brainfuck code and
alternative behaviour for ^C, either exiting the program if called at the main
prompt, clearing the line of code if called at the secondary prompt, or
stopping the execution of the running Brainfuck program.

- Integration with `libClame` to provide command-line argument parsing, which
is used for providing program information with command line switches,
specifying if colour output should be enabled, and pre-emptively declaring the
file to be loaded into memory. This library is also used as the backbone for
the command-line itself.

- Colour support for highlighting important information on the screen.

- Loading valid Brainfuck files at the prompt, performing a file listing with
`%` and executing them with `@`.

## Building, Running and Installing the Program from Source

You will need to have an up-to-date version of GNU C Compiler and GNU Make. You
can compile the code by running `make` at the command-line after cd'ing into
the source code folder.

Then, to start the compiled program, you can then run `./bfcli`. Some example
Brainfuck code is provided in the `demo` folder if you wish to try to them
out.

The following are the command-line arguments that this program accepts:

```
Usage: ./bfcli [ARGS]

Valid arguments are:
  -a, --about         prints the licence and about dialogue.
  -h, --help          prints the help dialogue.
  -v, --version       prints the program version.

  -c, --colour        (default) enables colour output.
  -m, --monochrome    disables colour output.

  -f, --file FILE     loads the file FILE into memory.
```

Finally, to install the code, you can run `make install`. This will install it
to `~/.local/bin` where `~` is your user's home folder. If you wish to change
this location, you can specify a new one with `DESTDIR=<location> make install`.
However, you may need to run the command with elevated privileges if installing
to a system folder like `/bin`.

## Contributing

If you happen to find any bugs, or want to contribute some code of your own,
feel free to create an issue or pull request.

Additionally, if you wish to contact me, feel free to mail me at
[jyothiraditya.n@gmail.com](mailto:jyothiraditya.n@gmail.com).

---

Happy Coding!
