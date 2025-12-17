#include "fonctions.h"
#include <stdio.h>
#include <stdbool.h>

// Programme main pour le jeu à 2 joueurs
// Inclut le bitboard, on observe les memes resultats pour le plateau normal et le bitboard

int coeffs_ia[10]={1,23,5};

int main(){

//Initialisation de l'othellier
Plateau othellier;
initialiser_plateau_bitboard(&othellier);

char plateau[N][N];
initialiser_plateau(plateau);

bool tour=false; // qui peut jouer, true-> Blancs ; false-> Noirs
int x; int y; //stockage des coordonnées dans le tableau correspondant au coup qui va être joué
int p;

printf("=== Debut de la partie ===\n");
afficher_plateau_bitboard(othellier);
afficher_plateau(plateau);

//Initialisation des scores
int scoreB, scoreN;
score_joueur(&scoreB, &scoreN, plateau);

    while(!fin_de_partie(plateau)){
		if (peut_jouer(tour, plateau)) {
			
			printf("coups possibles\n");
			if (tour) afficher_bitboard(coups_possibles(othellier.blanc, othellier.noir));
			else afficher_bitboard(coups_possibles( othellier.noir,othellier.blanc));		
			
			printf("C'est au tour de Mr.%s : \n",tour ? "Blanc" : "Noir");			
			do {
				lire_coup_ia(&x, &y);
			} while (!coup_valide(x, y, tour, plateau));
			traduction_x_y_vers_entier(x,y, &p);
			jouer_coup(x, y,tour, plateau); jouer_coup_bitboard(&othellier, p,tour);
			afficher_plateau(plateau);					  // on affiche l'état du plateau a chaque tour
			printf("bb\n");afficher_plateau_bitboard(othellier);
			

			
			//score_joueur(&scoreB, &scoreN, plateau);	
		//	printf("Score B:%d  N:%d\n", scoreB, scoreN); //on affiche le score actuel à chaque tour
		//	printf( "Score B:%d  N:%d\n", __builtin_popcountll(othellier.blanc), __builtin_popcountll(othellier.noir)); //on affiche le score actuel à chaque tour
			printf("evaluation_plateau: %d bitboard:%d \n",evaluation2(plateau), evaluation_bitboard(othellier,coeffs_ia));
			
			// ! pour les tests inverser tour car le coup a deja ete joue
			printf("minimax 0 (eval_bboard): %d  \n",minimax_bitboard_ia(0,&p,-INF-10,INF+10,!tour,coeffs_ia,othellier));
			printf("minimax_bb 1: %d  minimax_classique 1: %d \n", minimax_bitboard_ia(5,&p,-INF-10,INF+10,!tour,coeffs_ia,othellier), minimax(5, &x, &y,-INF-10,INF+10,!tour, plateau) );
			printf("coup de p:%d p/8:%d , p%%8:%d \n",p,p/8,p%8);


		} else {
			printf("Le joueur %s ne peut pas jouer, on passe le tour.\n", tour ? "Blanc" : "Noir");
		}
			
		tour=!tour; // changement de joueur
	}	

	// partie finie
	gagnant(plateau);
	return 0;


/*
    printf("--- AVANT LE COUP ---\n");
    printf("Noir (X) :\n");
    afficher_bitboard(othellier.noir);
    printf("blanc (O) :\n");
    afficher_bitboard(othellier.blanc);
	printf("Aefef :\n");
	afficher_plateau_bitboard(othellier);

    // Noir joue en F5 (index 37, ou 4*8 + 5)
    // Cela devrait prendre le pion blanc en E5
    printf("Je joue en index 37 (F5)...\n");
    jouer_coup_bitboard(&othellier, 37);

    printf("--- APRES LE COUP ---\n");
    printf("MOI (X) :\n");
    afficher_bitboard(othellier.noir); // On devrait voir le pion joué + le pion pris
    printf("ADV (O) :\n");
    afficher_bitboard(othellier.blanc); // Le pion pris doit avoir disparu
	afficher_plateau_bitboard(othellier);*/
}
// version d'avant de deux_jouers_local, on remet apres avoir debuggue