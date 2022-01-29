all:
	g++ -c -g -w gptool.cpp -fopenmp -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE=1 -D_LARGEFILE_SOURCE=1 
	gfortran -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE=1 -D_LARGEFILE_SOURCE=1 -o gptool gptool.o -L`pwd` -lshm  -fopenmp -lcpgplot -lpgplot -lX11 -lgcc -lm -lc -lstdc++
	rm -rf *.o
