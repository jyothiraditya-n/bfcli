 bfc demo/mandelbrot.bf -o man0.s -s
 bfc demo/mandelbrot.bf -o man1.s -sO1
 bfc demo/mandelbrot.bf -o man2.s -sO2
 bfc demo/mandelbrot.bf -o man3.s -sO3
 as man0.s -o man0.o
 as man1.s -o man1.o
 as man2.s -o man2.o
 as man3.s -o man3.o
 ld man0.o -o man0
 ld man1.o -o man1
 ld man2.o -o man2
 ld man3.o -o man3
 time ./man0
 time ./man1
 time ./man2
 time ./man3