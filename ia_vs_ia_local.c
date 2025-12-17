#include "fonctions.h"
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

// Programme main pour un benchmark de l'IA en version tableau de char

int main(){


srand(time(NULL)); // Intialisation du random, faire qu'une seule fois sinon bug

//Initialisation de l'othellier
char plateau[N][N];
initialiser_plateau(plateau);

bool tour=false; // qui peut jouer, true-> Blancs ; false-> Noirs
int x; int y; //stockage des coordonnées dans le tableau correspondant au coup qui va être joué


//Variables de score
int scoreB, scoreN;

int victoires_IA=0; // le nombre de victoires de l'IA testee
int egalites=0;
	bool ia_a_les_blancs=false; // apres demande pr savoir quelle couleur joue l'IA: Noir=false; true= blanc
    int nb_coup_jouees=0; int Depth=4; //profondeur de recherche de l'IA
    int echantillon=1000;
    clock_t debut=clock();
    for (int nb_parties=0; nb_parties<echantillon; nb_parties++){
        Depth=2;  // reset la profondeur au debut de la partie
        nb_coup_jouees=0;
        initialiser_plateau(plateau);
        ia_a_les_blancs=!ia_a_les_blancs; // change le cote de l'IA
        tour=false;
        while(!fin_de_partie(plateau)){
            if (peut_jouer(tour, plateau)) { // sinon on lui passe le tour
                
                if ((ia_a_les_blancs && tour) || (!ia_a_les_blancs && !tour)){ // c'est un NXOR mais flemme d'opti
                    if (nb_coup_jouees>50) Depth=10;// profondeur max il calcule tout jusqu'au bout
                    else if (nb_coup_jouees>45) Depth= 8; // augmente la profondeur en fin de partie
                    else if (nb_coup_jouees>39) Depth= 6; 
                    minimax(1,&x, &y,-INF-10,INF+10,tour,plateau); 
                    //ordi_random(&x, &y,tour,plateau); // si on veut jouer random vs random
            }
                else{
                    ordi_random(&x, &y,tour,plateau);            
                }
                jouer_coup(x, y,tour, plateau);
                nb_coup_jouees++;
            } 
                
            tour=!tour; // changement de joueur
        }	
        if (ia_a_les_blancs) score_joueur(&scoreB, &scoreN, plateau);
        else score_joueur(&scoreN, &scoreB, plateau);
        if (scoreB> scoreN ) victoires_IA++;
        else if (scoreB == scoreN) egalites++;

    }

    clock_t fin=clock();
    //---Fin des tests, affichage du gagnant
    double temps_ecoule = (double)(fin -debut)/ CLOCKS_PER_SEC;
    printf("Temps execution : %f secondes\n", temps_ecoule);
    printf("winrate IA: %d %% ; taux_draw=%d %%\n", victoires_IA/(echantillon/100),egalites/(echantillon/100) );
    printf("profondeur:%d, nombre de parties: %d \n", Depth, echantillon);


	return 0;
}

// Avec 1000 parties:
// L'IA version 1 a un winrate contre random de 81-84% et taux de nulles: 2-3% à profondeur 1; temps_execution=  1.137 s
// Profondeur 3: 89-91%, 1-2% de nulles (ou probablement moins vu que division par 10 et arrondis) temps_execution= 30.3 s



/*
Nouveaux test: les temps d'execution char vs bitboard sont tres similaires avec avantage pr char !!! 
Bon pas strictement la meme fonction d'evaluation peut etre

*/