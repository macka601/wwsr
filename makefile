CC=gcc 

CFLAGS = -I. -lm -lusb -lpq -I /usr/include/postgresql/ -lcurl -Werror=unused-but-set-variable -Werror=unused-parameter -Werror=unused-variable

DEPS = wwsr.h config.h wwsr_usb.h logger.h weatherProcessing.h database.h common.h wunderground.h

OBJ = wwsr.o config.o wwsr_usb.o logger.o weatherProcessing.o database.o wunderground.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
wwsr: $(OBJ) 
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f wwsr *.o

