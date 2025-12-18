#include "fonctions.h"
#include <stdio.h>
#include <stdbool.h>

// Programme main pour le jeu à 2 joueurs
int main(){
//Initialisation de l'othellier
char plateau[N][N];
initialiser_plateau(plateau);

bool tour=false; // qui peut jouer, true-> Blancs ; false-> Noirs
int x; int y; //stockage des coordonnées dans le tableau correspondant au coup qui va être joué


printf("=== Debut de la partie ===\n");
afficher_plateau(plateau);

//Initialisation des scores
int scoreB, scoreN;
score_joueur(&scoreB, &scoreN, plateau);
//printf("Score initial; Blancs: %d, Noirs: %d\n", scoreB, scoreN);  // enlevé car pas utile au début

		//char joueur; 
    while(!fin_de_partie(plateau)){
        //joueur= tour ? BLANCS : NOIRS;
		if (peut_jouer(tour, plateau)) {
			do {
				lire_coup(&x, &y, tour);
			} while (!coup_valide(x, y, tour, plateau));
			
			jouer_coup(x, y,tour, plateau);
			afficher_plateau(plateau);					  // on affiche l'état du plateau a chaque tour
				
			score_joueur(&scoreB, &scoreN, plateau);	
			printf("Score B:%d  N:%d\n", scoreB, scoreN); //on affiche le score actuel à chaque tour
		} else {
			printf("Le joueur %s ne peut pas jouer, on passe le tour.\n", tour ? "Blanc" : "Noir"); // pour le serveur, on sait pas encore si on doit jouer le coup je passe mon tour ou juste pas jouer
		}
			
		tour=!tour; // changement de joueur
	}	

	// partie finie
	gagnant(plateau);
	return 0;
}