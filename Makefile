release:
	gcc -DHIDDENDAT -std=gnu99 -Wall -pedantic -march=native -mtune=native -Ofast -fomit-frame-pointer -free -funsafe-math-optimizations src/Lab1_main.c -lm -o bin/LAB1_BMP
debug:
	gcc -DDEBUG -Wall -pedantic -g src/Lab1_main.c -lm -o bin/LAB1_BMP
clear:
	rm -vf *.dat
	rm -vf .*.dat
	rm -vf *.eps
	rm -vf test/lena_* test/baboon_* test/airplane_* test/peppers_*
