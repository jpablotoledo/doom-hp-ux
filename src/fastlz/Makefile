CC	= gcc
CFLAGS	= -O3 -Wall -pedantic -ansi
MAKELIB = ar
MAKELIBFLAGS = cru


OBJS	= fastlzc.o fastlzd.o fastlzintl.o


all:	libfastlz.a test_flz



libfastlz.a:	$(OBJS)
	$(MAKELIB) $(MAKELIBFLAGS) libfastlz.a $(OBJS)


test_flz:	test_flz.o libfastlz.a
	$(CC) -o test_flz test_flz.o libfastlz.a
	

clean:
	-rm *.o libfastlz.a


	
fastlzc.o:	fastlzc.c fastlz.h fastlzintl.h
	$(CC) -c $(CFLAGS) fastlzc.c

fastlzd.o:	fastlzd.c fastlz.h fastlzintl.h
	$(CC) -c $(CFLAGS) fastlzd.c

fastlzintl.o:	fastlzintl.c fastlzintl.h
	$(CC) -c $(CFLAGS) fastlzintl.c

test_flz.o:	test_flz.c fastlz.h
	$(CC) -c $(CFLAGS) test_flz.c
