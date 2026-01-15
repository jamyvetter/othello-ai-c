# Variables de compilation
CC = gcc
RM = del /Q # Utilise 'del /Q' (suppression silencieuse) de Windows
CFLAGS = -Wall -Wextra # Options de compilation : Avertissements

#--- Cibles par defaut doit etre en premier---

all: main_ia_entrainement IA_vs_humain_local ia_vs_ia_local ia_vs_ia_local_bitboard deux_joueurs_local
# Cibles principales (executables)
main_ia_entrainement: main_ia_entrainement.o algo_genetique_etu.o fonctions.o
	$(CC) $^ -o $@ $(LDFLAGS)

IA_vs_humain_local:IA_vs_humain_local.o fonctions.o
	$(CC) $^ -o $@ $(LDFLAGS)

ia_vs_ia_local:ia_vs_ia_local.o fonctions.o
	$(CC) $^ -o $@ $(LDFLAGS)

ia_vs_ia_local_bitboard:ia_vs_ia_local_bitboard.o fonctions.o
	$(CC) $^ -o $@ $(LDFLAGS)

deux_joueurs_local:deux_joueurs_local.o fonctions.o
	$(CC) $^ -o $@ $(LDFLAGS)


# Compilation des fichiers sources (.c vers .o)
fonctions.o: fonctions.c fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

main_ia_entrainement.o: main_ia_entrainement.c fonctions.h algoGenetique.h
	$(CC) $(CFLAGS) -c $< -o $@

algo_genetique_etu.o: algoGenetique_etu.c algoGenetique.h fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

IA_vs_humain_local.o: IA_vs_humain_local.c fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

ia_vs_ia_local.o: ia_vs_ia_local.c fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

ia_vs_ia_local_bitboard.o: ia_vs_ia_local_bitboard.c fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

deux_joueurs_local.o: deux_joueurs_local.c fonctions.h
	$(CC) $(CFLAGS) -c $< -o $@

# Cible de nettoyage
clean:
	-$(RM) *.o
	-$(RM) main_ia_entrainement.exe
	-$(RM) IA_vs_humain_local.exe
	-$(RM) deux_joueurs_local.exe
	-$(RM) ia_vs_ia_local.exe
	-$(RM) ia_vs_ia_local_bitboard.exe
