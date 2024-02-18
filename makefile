all: comserver comcli

comserver: comserver.c
	gcc -g -Wall -o comserver comserver.c

comcli: comcli.c
	gcc -g -Wall -o comcli comcli.c -lpthread

clean:
	rm -fr *~ comserver comcli