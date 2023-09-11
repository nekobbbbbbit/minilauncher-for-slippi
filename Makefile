CC=cc
CFLAGS=-g `pkg-config --cflags efl ecore elementary libusb-1.0` `sdl2-config --cflags` `curl-config --cflags`
LDFLAGS=`pkg-config --libs efl ecore elementary libusb-1.0` `sdl2-config --libs` `curl-config --libs` -lcjson
OBJS=main.o replay.o home.o input.o

minilauncher4slippi: $(OBJS)
	$(CC) -o minilauncher4slippi $(OBJS) $(LDFLAGS)

clean:
	rm minilauncher4slippi *.o
