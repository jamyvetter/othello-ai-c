CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -L. -lothello

# Cibles
EXEC = serveur_ia_bitboard
OBJ = fonctions.o

all: $(EXEC)

# L'exécutable dépend de l'objet fonctions.o
$(EXEC): serveur_ia_bitboard.c $(OBJ) fonctions.h
	$(CC) $(CFLAGS) -o $(EXEC) serveur_ia_bitboard.c $(OBJ) $(LDFLAGS)

# Compilation de l'objet fonctions.o
$(OBJ): fonctions.c fonctions.h
	$(CC) $(CFLAGS) -c fonctions.c -o $(OBJ)

clean:
	rm -f $(OBJ) $(EXEC)
