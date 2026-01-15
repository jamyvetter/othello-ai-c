CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -L. -lothello

# Cibles
EXEC = US5
OBJ = fonctions.o

all: $(EXEC)

# L'exécutable dépend de l'objet fonctions.o
$(EXEC): US5.c $(OBJ) fonctions.h
	$(CC) $(CFLAGS) -o $(EXEC) US5.c $(OBJ) $(LDFLAGS)

# Compilation de l'objet fonctions.o
$(OBJ): fonctions.c fonctions.h
	$(CC) $(CFLAGS) -c fonctions.c -o $(OBJ)

clean:
	rm -f $(OBJ) $(EXEC)

