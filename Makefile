all:	src/flash_bash.c
	gcc -Wall -pthread -o flash_bash src/flash_bash.c -lpigpio -lrt

clean:
	rm -rf flash_bash flash_bash.dSYM/