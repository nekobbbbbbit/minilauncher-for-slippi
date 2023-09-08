CC=cc
CFLAGS=`pkg-config --cflags efl ecore elementary` `curl-config --cflags`
LDFLAGS=`pkg-config --libs efl ecore elementary` `curl-config --libs` -lcjson
OBJS=main.o replay.o home.o

minilauncher4slippi: $(OBJS)
	$(CC) -o minilauncher4slippi $(OBJS) $(LDFLAGS)

clean:
	rm minilauncher4slippi *.o
