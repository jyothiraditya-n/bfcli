# Bfcli: The Interactive Command-Line Brainfuck Interpreter

```
Usage: ./bfcli
(Don't supply any arguments.)

Interactive Brainfuck interpreter; exit with ^C.
Line buffer size: 4096 chars Code buffer size: 65536 chars
Memory size: 655360 chars

Extended Brainfuck commands:
  ?: Prints the help and copyright disclaimer to the console.
  /: Clears the memory and moves the pointer to 0.
  *: Prints memory values around the current pointer value.
```

## Building / Running It
Run the following commands to build and run it:

```
$ make
$ ./bfcli
```
