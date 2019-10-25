program: alloc.c
	gcc -Wall -o alloc.out alloc.c

debug: alloc.c
	gcc -Wall -g -o alloc.out alloc.c

run: alloc.out
	./alloc.out

memrun: alloc.out
	valgrind --tool=helgrind ./alloc.out

clean:
	rm -f *.out
