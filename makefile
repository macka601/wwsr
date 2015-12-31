CC=gcc 

CFLAGS = -I. -lm -lusb -lpq -I /usr/include/postgresql/ -lcurl 

DEPS = wwsr.h usbFunctions.h logger.h weatherProcessing.h database.h common.h wunderground.h

OBJ = wwsr.o usbFunctions.o logger.o weatherProcessing.o database.o wunderground.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
wwsr: $(OBJ) 
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f wwsr *.o

#gcc wwsr3.3.c -o wwsr3.3 -lm -lusb -lpq -I /usr/include/postgresql/

