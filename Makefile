
all:	marketSim.c
	gcc -O3 marketSim.c -lpthread -o marketSim

clean:
	rm -f marketSim
 
