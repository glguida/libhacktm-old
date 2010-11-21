CFLAGS+= -g  -O3 
LIB = hacktm
LIBNAME = lib${LIB}.a

all: ${LIBNAME} tests

${LIBNAME}: fdr.o components.o htmnode.o
	$(AR) rs $@ $^ 

clean: tests_clean
	-rm *.o *.a

tests: test0 test1 test3
test0: ${LIBNAME} tests/test0.c
	$(CC) $(CFLAGS) -I. -L. tests/$@.c -lhacktm -o $@
test1: ${LIBNAME} tests/test1.c
	$(CC) $(CFLAGS) -I. -L. tests/$@.c -lhacktm -lncurses -o $@
test3: ${LIBNAME} tests/test3.c
	$(CC) $(CFLAGS) -I. -L. tests/$@.c -lhacktm -o $@

# This test requires PortAudio and single-precision FFTW3 libraries.
test_audio: ${LIBNAME} tests/test_audio.c
	$(CC) $(CFLAGS) -I . -L. tests/$@.c -lhacktm -lportaudio -lm -lfftw3f -lncurses -o $@

tests_clean:
	-rm test0 test1 test3 test_audio

