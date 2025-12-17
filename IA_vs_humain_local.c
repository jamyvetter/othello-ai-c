#include "fonctions.h"
#include <stdio.h>
#include <stdbool.h>

// Programme main pour le jeu humain contre IA
// NOTE: copier ce programme et en faire la version utilisant le bitboard pour US1

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

	bool ia_a_les_blancs=false; // apres demande pr savoir quelle couleur joue l'IA: Noir=false; true= blanc
    int nb_coup_jouees=0; int Depth=4; //profondeur de recherche de l'IA

    while(!fin_de_partie(plateau)){
		if (peut_jouer(tour, plateau)) {
			
            if ((ia_a_les_blancs && tour) || (!ia_a_les_blancs && !tour)){ // c'est un NXOR mais flemmet d'opti
                if (nb_coup_jouees>50) Depth= 10;// profondeur max il calcule tout jusqu'au bout
                else if (nb_coup_jouees>45) Depth= 8; // augmente la profondeur en fin de partie
                else if (nb_coup_jouees>39) Depth= 6; 
                minimax(Depth,&x, &y,-INF-10,INF+10,tour,plateau);
            }
            else{
                do {
                    lire_coup_ia(&x, &y);
                } while (!coup_valide(x, y, tour, plateau));                
            }

			envoyer_coup_serveur(x,y);
			jouer_coup(x, y,tour, plateau);
            nb_coup_jouees++;
			afficher_plateau(plateau);					  // on affiche l'état du plateau a chaque tour
				
			score_joueur(&scoreB, &scoreN, plateau);	
			printf("Score B:%d N:%d nb_coups:%d\n", scoreB, scoreN,nb_coup_jouees); //on affiche le score actuel à chaque tour
		} else {
			printf("Le joueur %s ne peut pas jouer, on passe le tour.\n", tour ? "Blanc" : "Noir"); // pour le serveur, on sait pas encore si on doit jouer le coup je passe mon tour ou juste pas jouer
		}
			
		tour=!tour; // changement de joueur
	}	

	// partie finie
	gagnant(plateau);
	return 0;
}