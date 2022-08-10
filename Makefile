build:
	mpicxx -fopenmp -c main.c -o main.o -lm
	mpicxx -fopenmp -c functions.c -o functions.o -lm
	nvcc -I./inc -c cuda.cu -o cuda.o
	mpicxx -fopenmp -o main  main.o functions.o cuda.o  /usr/local/cuda/lib64/libcudart_static.a -ldl -lrt

clean:
	rm -f *.o ./main

run:
	mpiexec -np 2 ./main > output.txt 
	
runOn2:
	mpiexec -np 2 -machinefile  mf  -map-by  node  ./main > output.txt                
