#include "fonctions.h"
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

// Programme main pour un benchmark de l'IA, version bitboard

int main(){


srand(time(NULL)); // Intialisation du random, faire qu'une seule fois sinon bug

//Initialisation de l'othellier
Plateau plateau;
initialiser_plateau_bitboard(&plateau);


bool tour=false; // qui peut jouer, true-> Blancs ; false-> Noirs
int p; // valeur du coup a jouer, 0<=p<=63 (ou 64 si pass mais later ça)


//Variables de score
int scoreB, scoreN;

int victoires_IA=0; // le nombre de victoires de l'IA testee
int egalites=0;
bool ia_a_les_blancs=false; // apres demande pr savoir quelle couleur joue l'IA: Noir=false; true= blanc
int nb_coup_jouees=0; int Depth=4; //profondeur de recherche de l'IA
//int coeffs_ia[10]={21, 10, 72, 88, 23, 38, 80}; // les gens choisies pour l'IA
int coeffs_ia[10]={18 ,43, 89, 93 ,6 ,2 ,40};
int echantillon=1000;
clock_t debut=clock();

    for (int nb_parties=0; nb_parties<echantillon; nb_parties++){
        nb_coup_jouees=0;
        Depth=2; // reset la profondeur au debut de la partie
        initialiser_plateau_bitboard(&plateau);
        ia_a_les_blancs=!ia_a_les_blancs; // change le cote de l'IA
        tour=false;
        while(!fin_de_partie_bitboard(plateau)){
            if (peut_jouer_bitboard(plateau,tour)) { // sinon on lui passe le tour
                //printf("peut jopue\n");
                if ((ia_a_les_blancs && tour) || (!ia_a_les_blancs && !tour)){ // c'est un NXOR mais flemme d'opti
                   // /if (nb_coup_jouees>50) Depth=10;// profondeur max il calcule tout jusqu'au bout
                    //else if (nb_coup_jouees>45) Depth= 8; // augmente la profondeur en fin de partie
                    //else if (nb_coup_jouees>39) Depth= 6; 
                    minimax_bitboard_ia(Depth,&p,-INF-10,INF+10,tour,coeffs_ia,plateau); // test en profoneur 1 pour evaluer fonction evaluation
                    //ordi_random_bitboard(&p,tour,plateau); 
                }
                else{
                    ordi_random_bitboard(&p,tour,plateau);            
                }
                //printf("va jouer joue le coup %d\n",p);
                jouer_coup_bitboard(&plateau,p,tour); //printf("coup jopue\n");
                //afficher_plateau_bitboard(plateau);
                nb_coup_jouees++;
            } 
                
            tour=!tour; // changement de joueur
        }	
       scoreB=__builtin_popcountll(plateau.blanc);scoreN=__builtin_popcountll(plateau.noir); 
       if ((ia_a_les_blancs) && (scoreB> scoreN )) victoires_IA++;
       else if ((!ia_a_les_blancs) && (scoreB< scoreN ) ) victoires_IA++;
       else if (scoreB == scoreN) egalites++;

    }
    clock_t fin=clock();
    //---Fin des tests, affichage du gagnant
    double temps_ecoule = (double)(fin -debut)/ CLOCKS_PER_SEC;
    printf("Temps execution : %f secondes\n", temps_ecoule);
    printf("Victoires IA: %d / %d draw: %d \n ",victoires_IA,echantillon,egalites);
    printf("winrate IA: %d %% ; taux_draw=%d %%\n", victoires_IA/(echantillon/100),egalites/(echantillon/100) );
    printf("profondeur:%d, nombre de parties: %d \n", Depth, echantillon);

	return 0;
}

/*
Conclusion premier test: bitboard 0.5 s plus rapide sur 10000 parties jouees aleatoirement vs plateau normal
Et on a pas encore optimisé le peut_jouer
*/



/*
A upgrade: la fonction d'eval gagne 75% du temps ce qui est peu, meme apres modif meilleur tableau.
-> upgrade, maintenant gange 89-92% du temps a profondeur 1 après entrainement. En plus celle-là utilise la fonctionnalité de changer ses genes
selon la phase du jeu

*/