all: client.exe server.exe

%.exe: %.o
	gcc $< -o $@

%.o: %.c
	gcc -c -g -Wall $< -o $@

clean:
	rm -f *.o *.exe