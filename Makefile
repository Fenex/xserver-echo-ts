name=echots
src=./src/
dest=./build/
vendor=./vendor/vblibc/

all: prebuild main kbrd vblibc_string
	gcc -g -Wall $(dest)main.o $(dest)kbrd.o $(dest)vblibc_string.o -o $(dest)$(name) -lxcb

prebuild:
	@mkdir -p $(dest)

main: $(src)main.c
	gcc -g -c -Wall $(src)main.c -o $(dest)main.o

kbrd: $(src)kbrd.c
	gcc -g -c -Wall $(src)kbrd.c -o $(dest)kbrd.o

vblibc_string: $(vendor)$(src)vblibc_string.c
	gcc -g -c -Wall $(vendor)$(src)vblibc_string.c -o $(dest)vblibc_string.o

clean:
	rm -rf $(dest)*
