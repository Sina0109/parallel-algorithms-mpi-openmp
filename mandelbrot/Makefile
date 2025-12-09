CC = cc
RM = rm -f

all: mandelbrot mandelbrot-parallel

mandelbrot: mandelbrot.c rasterfile.h
	$(CC) -o mandelbrot $< -fopenmp

mandelbrot-parallel: mandelbrot-parallel.c rasterfile.h
	$(CC) -o mandelbrot-parallel $< -fopenmp

clean:
	$(RM) mandelbrot mandelbrot-parallel *.ras
