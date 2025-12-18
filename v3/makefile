othello:main.o fonctions.o
	gcc main.o fonctions.o -o othello -Wall -Wextra
fonctions.o: fonctions.c fonctions.h
	gcc -c fonctions.c -o fonctions.o -Wall -Wextra
main.o: main.c fonctions.h
	gcc -c main.c -o main.o -Wall -Wextra