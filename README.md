# Bfcli: The Interactive Brainfuck Command-Line Interpreter

```
Usage: ./bfcli
(Don't supply any arguments.)

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
```

## Building / Running It
Run the following commands to build and run it:

```
$ make
$ ./bfcli
```

Run the following commands to install it locally and run it:

```
$ make
$ make install
$ bfcli
```

Alternatively, specify an installation folder and run it:

```
$ make
# DESTDIR=/bin make install
$ bfcli
```
