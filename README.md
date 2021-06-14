# Bfcli: The Interactive Brainfuck Command-Line Interpreter

```
Usage: bfcli [ARGS]

Valid arguments are:
  -a, --about         prints the licence and about dialogue.
  -h, --help          prints the help dialogue.

  -c, --colour                (default) enables colour output.
  -m, --monochrome    disables colour output.

  -f, --file FILE     loads the file FILE into memory.

Interactive Brainfuck interpreter; exit with ^C.

Line buffer size: 4096 chars
Code buffer size: 65536 chars
Memory size: 655360 chars

Extended Brainfuck commands:
  ? Prints the help and copyright disclaimer to the console.
  / Clears the memory and moves the pointer to 0.
  * Prints memory values around the current pointer value.
  
  { Begins a block of code.
  } Ends a block of code.

Note: Once a block of code has been started, code will not be
      executed until the block has been ended.

  @ Execute code from the loaded file.
  % Print code from the loaded file.

Note: The above two commands perform no action if no file is loaded.
      In order to load a file when Bfcli is running, type the file
      name at the main prompt.

Happy coding! :)
```

## Building / Running It
Run the following commands to build and run it:

```
$ make
$ ./bfcli
```

Or run the following commands to install it locally and run it:

```
$ make
$ make install
$ bfcli
```

Or specify an installation folder and run it:

```
$ make
# DESTDIR=/bin make install
$ bfcli
```
