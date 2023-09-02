CC=cc
CFLAGS=`pkg-config --cflags efl ecore elementary`
LDFLAGS=`pkg-config --libs efl ecore elementary`

minilauncher4slippi: main.o replay.o
	$(CC) -o minilauncher4slippi main.o replay.o $(LDFLAGS)

clean:
	rm minilauncher4slippi *.o
