CFLAGS = -g -Wall -O3 -o
CLIBS = -lrt -lpthread
MONITORES =  sensores.c socket.c tela.c
HOST = localhost
PORT = 4545

run: compile
	echo $@	
	@ ./main.exe $(HOST) $(PORT)

compile: clean
	echo $@	
	@  gcc main.c $(MONITORES) $(CFLAGS) main.exe $(CLIBS)

clean: 
	echo $@	
	@ rm -f *.out *.txt