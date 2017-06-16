a.out: main.c my_print.o
	gcc -m32 main.c my_print.o
my_print.o: my_print.asm
	nasm -f elf32 my_print.asm

run:
	./a.out

clean:
	rm my_print.o a.out