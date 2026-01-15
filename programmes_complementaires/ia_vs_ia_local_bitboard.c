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
int echantillon=130;

clock_t debut=clock();
double plus_grand_temps_minimax= (double)(clock() -debut)/ CLOCKS_PER_SEC; // explicite, mesure performance, sera très petit au debut normalement
double plus_grand_temps_minimax_2= (double)(clock() -debut)/ CLOCKS_PER_SEC;


struct timespec start_p, end_p;
clock_gettime(CLOCK_MONOTONIC, &start_p);


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
                    
                    clock_t avant_minimax=clock();
                    struct timespec start, end;
                    clock_gettime(CLOCK_MONOTONIC, &start);
                    minimax_bitboard_ia(Depth,&p,-INF-10,INF+10,tour,coeffs_ia,plateau); // test en profoneur 1 pour evaluer fonction evaluation
                    clock_t apres_minimax=clock();
                    double temps_minimax = (double)(apres_minimax -avant_minimax)/ CLOCKS_PER_SEC;
                    if (temps_minimax > plus_grand_temps_minimax) {
                        plus_grand_temps_minimax_2= temps_minimax;
                       // printf("ouiPlus long temps d'execution actuel : %f s pour nb_coups %d\n", plus_grand_temps_minimax_2,nb_coup_jouees);
                    }
                    clock_gettime(CLOCK_MONOTONIC, &end);

                    // Formule de calcul :
                    double diff = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
                    if (diff > plus_grand_temps_minimax) {
                        plus_grand_temps_minimax= diff;
                        //printf("Plus long temps d'execution actuel : %f s pour nb_coups %d\n", plus_grand_temps_minimax,nb_coup_jouees);
                    }

                    //ordi_random_bitboard(&p,tour,plateau); 
                }
                else{ // JOUEUR 2
                    //minimax_bitboard_ia(1,&p,-INF-10,INF+10,tour,coeffs_ia,plateau);
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

    clock_gettime(CLOCK_MONOTONIC, &end_p);
    double temps_programme_complet = (end_p.tv_sec - start_p.tv_sec) + (end_p.tv_nsec - start_p.tv_nsec) * 1e-9;

    clock_t fin=clock();
    double temps_ecoule = (double)(fin -debut)/ CLOCKS_PER_SEC;



    printf("Plus long temps d'execution : %f secondes\n", plus_grand_temps_minimax); printf("Plus long temps d'execution de clk : %f secondes\n", plus_grand_temps_minimax_2);
    printf("Temps execution : %f secondes\n", temps_ecoule);
    printf("Temps execution ameliore : %f secondes\n", temps_programme_complet);
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
/* 17/12: Apres resolution de certains bugs sur fonction d'evaluation... TADA!!
Temps execution : 18.449000 secondes
Victoires IA: 95003 / 100000 draw: 965 
 winrate IA: 95 % ; taux_draw=0 %
profondeur:1, nombre de parties: 100000

*/

/* TEST RAPIDITE IA: mise a profondeur 9 toute la partie
Plus long temps d'execution actuel : 1.008000 s pour nb_coups 12
Plus long temps d'execution actuel : 1.159000 s pour nb_coups 18
Plus long temps d'execution actuel : 1.269000 s pour nb_coups 20
Plus long temps d'execution actuel : 7.300000 s pour nb_coups 22
Plus long temps d'execution actuel : 8.130000 s pour nb_coups 15
Plus long temps d'execution : 8.130000 secondes
Temps execution : 134.181000 secondes
Victoires IA: 10 / 10 draw: 0

-> On voit que vers les 15-20 premiers coups jouees, il faut mette une profondeur plus basse que pour le reste

Echantillon de 100 tests, test de jusqu'à combien de coups on peut calculer en fin de partie:
On voit aussi que sur un test; si nb_coups_jouees >= 45 il joue en moins de 15s 
Attention toutefois, il est possible que ces resutats soient fausses par le fait qu'on joue contre random qui ne joue pas bien et se laisse peu d'options donc moins de coup à calculer

! Par contre pour nb_coups_jouees=42, Pas le temps de tout calculer; il a pris 23s sur un test

*/