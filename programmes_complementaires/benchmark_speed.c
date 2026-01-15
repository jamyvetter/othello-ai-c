#include "fonctions.h"
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdlib.h>

// Programme pour évaluer quelle profondeur est la bonne pr chaque moment, mais bientot obsolete

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
    //int coeffs_ia[10]={21, 10, 72, 88, 23, 38, 80}; // les gens choisies pour l'IA
    int coeffs_ia[10]={18 ,43, 89, 93 ,6 ,2 ,40};
    int echantillon=2;

    int profondeur_maximale[60]={10,9,9,8,8,8,8,7,7,7,7,7,7,7,7,7,7,6,7,7,8,8,8,8,8,7,8,7,8,7,8,6,6,8,8,8,8,10,8,10,8,10,9,10,10,12,20,20,20,20,20,20,20,20,20,20,20,20,20,20,}; // Les profondeurs pour chaque coup qui n'ont pas encore timeout

    int profondeur_adapatee[60]; // Les profondeurs relatives au test
    for (int g=0; g<30; g++) profondeur_adapatee[g]=20; 
    for (int g=30; g<60; g++) profondeur_adapatee[g]=20; 
    double limite_temps = 0.1; // temps max autorise de reflexion par l'IA

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

                    double temps_mesure= 0;
                    int depth_test=4; // on prend une certaine marge mais on commence pas à 1 
                    

                    if (nb_coup_jouees<= 50){
                        int test_possible=1;
                        while(test_possible){
                            double debut_m= obtenir_temps();
                            minimax_bitboard_ia(depth_test,&p,-INF-10,INF+10,tour,coeffs_ia,plateau);
                            double fin_m= obtenir_temps();
                            temps_mesure= fin_m - debut_m;//printf("Temps execution : %.3f secondes\n", temps_mesure);// printf("nb_coup_jouees :%d \n",nb_coup_jouees);
                            if (temps_mesure> limite_temps || (depth_test>= 64- nb_coup_jouees) || depth_test> profondeur_maximale[nb_coup_jouees] ) {
                                // on garde la profondeur la plus basse car depth_test a pris trop de temps
                                if (profondeur_adapatee[nb_coup_jouees]>=depth_test) profondeur_adapatee[nb_coup_jouees]= depth_test-1;
                                test_possible=0;
                            // printf("break\n");
                            }
                            depth_test++;
                        }                        
                    }
                    else{
                        minimax_bitboard_ia(1,&p,-INF-10,INF+10,tour,coeffs_ia,plateau);
                    }




                }
                else{ // JOUEUR 2
                    //minimax_bitboard_ia(1,&p,-INF-10,INF+10,tour,coeffs_ia,plateau);
                    ordi_random_bitboard(&p,tour,plateau);            
                }
                //printf("va jouer joue le coup %d\n",p);
                jouer_coup_bitboard(&plateau,p,tour); //printf("coup jopue\n");
                //afficher_plateau_bitboard(plateau);
                nb_coup_jouees++;
                //printf("nb_coups_jouees: %d\n",nb_coup_jouees);
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
    
    //printf("winrate IA: %d %% ; taux_draw=%d %%\n", victoires_IA/(echantillon/100),egalites/(echantillon/100) );
    //printf("profondeur:%d, nombre de parties: %d \n", Depth, echantillon);

    printf("Noeuds vus: %llu MegaNodes per sec: %f \n",nb_noeuds_vus, nb_noeuds_vus/ (1000000*(fin -debut)));

    for (int i=40; i<46; i++) printf("nb_coups_jouees:%d Profondeur possible:%d\n",i,profondeur_adapatee[i]);

    for (int i=0; i<60; i++) printf("%d,",profondeur_adapatee[i]);
	return 0;
}

/*
Compiler avec gcc .\benchmark_vitesse.c .\fonctions.c -o benchmark_vitesse -Wall -Wextra
.\benchmark_vitesse

Resultats: entre 1.8 et 1.9 MNDs
*/