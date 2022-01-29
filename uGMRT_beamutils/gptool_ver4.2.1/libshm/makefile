all:
	g++ -fPIC -c -g -w libshm.cpp -lX11 -lgcc -lm -lc -lstdc++
	#g++ -shared -o libshm.so libshm.o
	ar rc libshm.a libshm.o
	g++ -o fileToSHM -fPIC -g -w fileToSHM.cpp -L. -lshm
