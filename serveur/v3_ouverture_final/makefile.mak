CC = gcc
CFLAGS = -Wall -Wextra

all: fonctions.o serveur_ia_bitboard

fonctions.o: fonctions.c fonctions.h
	$(CC) $(CFLAGS) -c fonctions.o fonctions.c

serveur_ia_bitboard: serveur_ia_bitboard.c fonctions.h #../fonctions.o
	$(CC) $(CFLAGS) -L. -o serveur_ia_bitboard serveur_ia_bitboard.c fonctions.o -lothello
