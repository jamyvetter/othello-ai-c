#include "fonctions.h"
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdlib.h>

// Programme pour évaluer la vitesse d'éxecution de l'IA (nodes/sec)

double obtenir_temps(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}


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


// RAPPEL COEFFs: nb_coup_legaux, nb_ jetons, poids_pions // et pareil mais en fin de partie
// ---------------------- hof ias
//int coeffs_ia[10]={18 ,43, 89, 93 ,6 ,2 ,40}; // 95 % D1, D3:95% , 97% D4
//int coeffs_ia[10]={84 ,38, 89, 98 ,32 ,3 ,36}; // 95% aussi 
// ---------------------- hof ias


//int coeffs_ia[10]={91, 40, 13, 89, 9 ,4, 14}; // 93%

int coeffs_ia[10]={88, 15, 34 ,60 ,35, -2 ,27}; // 93 % ça va , D2 = 97% ,D3= 98%, D4= 99% -> CETTE IA a l'air meilleur sur le long terme
// et en plus elle gagne plus contre codingame alors tout va bien


int echantillon=50;



    double debut= obtenir_temps();

    for (int nb_parties=0; nb_parties<echantillon; nb_parties++){
        nb_coup_jouees=0;
        Depth=1; // reset la profondeur au debut de la partie
        initialiser_plateau_bitboard(&plateau);
        ia_a_les_blancs=!ia_a_les_blancs; // change le cote de l'IA
        tour=false;
        while(!fin_de_partie_bitboard(plateau)){
            if (peut_jouer_bitboard(plateau,tour)) { // sinon on lui passe le tour
                //printf("peut jopue\n");
                if ((ia_a_les_blancs && tour) || (!ia_a_les_blancs && !tour)){ // c'est un NXOR mais flemme d'opti
                    if (nb_coup_jouees>=45) Depth=20;// profondeur max il calcule tout jusqu'au bout
                    else if (nb_coup_jouees>30) Depth= 1; // augmente la profondeur en fin de partie
                    else if (nb_coup_jouees>15) Depth= 1; 
                    
                    //Calcul du temps maximum de reponse de l'IA

                    minimax_bitboard_ia(7,&p,-INF-10,INF+10,tour,coeffs_ia,plateau); // test en profoneur 1 pour evaluer fonction evaluation
                    //ordi_greedy_bitboard(&p,tour,plateau);
                }
                else{ // JOUEUR 2
                    //minimax_bitboard_ia(1,&p,-INF-10,INF+10,tour,coeffs_ia,plateau);
                    //ordi_random_bitboard(&p,tour,plateau);         
                    ordi_greedy_bitboard(&p,tour,plateau);     
                }
                //printf("va jouer joue le coup %d\n",p);
                jouer_coup_bitboard(&plateau,p,tour); //printf("coup jopue\n");
                //afficher_plateau_bitboard(plateau);
                nb_coup_jouees++;
                //printf("nb_coups:%d  %d \n",nb_coup_jouees,ia_a_les_blancs);
            } 
                
            tour=!tour; // changement de joueur
        }	
       scoreB=__builtin_popcountll(plateau.blanc);scoreN=__builtin_popcountll(plateau.noir); 
       if ((ia_a_les_blancs) && (scoreB> scoreN )) victoires_IA++;
       else if ((!ia_a_les_blancs) && (scoreB< scoreN ) ) victoires_IA++;
       else if (scoreB == scoreN) egalites++;

    }

  //  clock_gettime(CLOCK_MONOTONIC, &end_p);
  //  double temps_programme_complet = (end_p.tv_sec - start_p.tv_sec) + (end_p.tv_nsec - start_p.tv_nsec) * 1e-9;

   // clock_t fin=clock();
   // double temps_ecoule = (double)(fin -debut)/ CLOCKS_PER_SEC;

    double fin= obtenir_temps();

   // printf("Plus long temps d'execution : %f secondes\n", plus_grand_temps_minimax); printf("Plus long temps d'execution de clk : %f secondes\n", plus_grand_temps_minimax_2);
    printf("Temps execution : %.3f secondes\n", fin -debut);
    //printf("Temps execution ameliore : %f secondes\n", temps_programme_complet);
    printf("Victoires IA: %d / %d draw: %d \n ",victoires_IA,echantillon,egalites);
    printf("winrate IA: %d %% ; taux_draw=%d %%\n", victoires_IA/(echantillon/100),egalites/(echantillon/100) );
    printf("profondeur:%d, nombre de parties: %d \n", Depth, echantillon);

    printf("Noeuds vus: %llu MegaNodes per sec: %f ",nb_noeuds_vus, nb_noeuds_vus/ (1000000*(fin -debut)));

	return 0;
}

/*
Compiler avec gcc .\benchmark_vitesse.c .\fonctions.c -o benchmark_vitesse -Wall -Wextra
.\benchmark_vitesse

Resultats: entre 1.8 et 1.9 MNDs -> 1.8 - 1.9
*/