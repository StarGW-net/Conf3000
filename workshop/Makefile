conf3: slave14.o tailer11.o 
	cc -o conf3 slave14.o tailer11.o 
	chmod 7111 conf3
	mv -f conf3 ../conf3

fmat11.o: fmat11.c
	cc -c fmat11.c

slave14.o: slave14.c stevelib.h packet1.h gubbings.h user.h fmat11.h chan.h 
	cc -c slave14.c

crap: crap.c
	 cc -c crap.c
	cc -o crap crap.o tailer11.o

tailer11.o: tailer11.o gubbings.h user.h
	cc -c tailer11.c

server2: server2.c packet1.h gubbings.h slave14.o
	cc server2.c -o server
	chmod 0100 server
	mv -f server ../server

doalt: doalt.c gubbings.h
	cc doalt.c -o doalt
	chmod 0711 doalt
	mv doalt ../doalt

marvin: marv.c gubbings.h
	cc marv.c -o marvin
	chmod 7111 marvin
	mv -f marvin ../marvin

robot: robot.c gubbings.h
	cc robot.c -o robot
	chmod 7111 robot
	mv -f robot ../robot

clean:
	/bin/rm -f *.o
	/bin/rm -f *~
