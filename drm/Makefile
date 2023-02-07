CFLAGS=`pkg-config --cflags libdrm`
LFLAGS=`pkg-config --libs libdrm`
CFLAGS+=-Wall -O3 

all: test_rec

%.o: %.c
	gcc -c $< -o $@ $(CFLAGS)

test_rec: test_rec.o drm_fb.o
	gcc -o test_rec test_rec.o drm_fb.o $(LFLAGS)

clean:
	rm test_rec *.o
