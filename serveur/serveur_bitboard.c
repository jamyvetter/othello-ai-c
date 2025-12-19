// Global pour le bitoboard , pour le serveur
//ligne d'execution: ...
// A faire avant : export LD_LIBRARY_PATH=.


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "userNetwork.h"
#include "../fonctions.h"

// Mode serveur
int main(int argc,char *argv[]) 
{
	double pire_temps= 0.0; int stade_jeu;
	clock_t debut; clock_t fin;
	int coeffs_ia[10]={18 ,43, 89, 93 ,6 ,2 ,40};
	int worst_diff=0;

    // Stockage de notre plateau de jeu
    Plateau plateau;
    initialiser_plateau_bitboard(&plateau);
    int move_choisi;
    int nb_coup_jouees=0; //compte le nombre de jeton poses pour avoir une idee de ou en est la partie
	int Depth= 10; //profondeur de base de l'IA
	game *g;
	int move; // utliser par prof

	if ((g=allocateGameOthello()) == NULL ) { return(EXIT_FAILURE); }

	if (argc > 3 ) { 
		g->userId=atoi(argv[3]); 
		g->address=argv[2]; 
	}
	else { fprintf(stderr,"usage : %s <userPasswd> <ip ad> <userId>\n",argv[0]); exit(-1); }

	g->port=8080; //par exemple

	if (registerGameOthello(g,argv[1]) < 0 ) { exit(-1); }	// test de l'authentification auprès du serveur 

	if (startGameOthello(g) < 0 ) { // cet appel est bloquant en attendant un adversaire 
		fprintf(stderr,"error Starting Game\n"); 

		return (EXIT_FAILURE); 
	}
	printf("I am player %s\n",(g->myColor==BLACK)?"Black":"White"); 
    printf("%d\n",g->myColor);
	// debut de partie
	while (g->state == PLAYING && !feof(stdin)) {
	 	if (g->myColor != g->currentPlayer) { // attente du coup de l'adversaire 
			if ((move=waitMoveOthello(g)) == 0 ) {
				printf("Game status %d: \t",g->state); 
				if (g->state == PLAYING) { 
					printf("Received move from server %d (x=%d,y=%d)\n",g->move,g->move%8,g->move/8);
                    // actualise notre plateau avec le coup de l'adversaire
                    if (g->move != 64){jouer_coup_bitboard(&plateau,g->move,!g->myColor); nb_coup_jouees++;}
				}
			}
		}
	 	else { 		// C'est au tour de cette machine, on doit décider d'un coup et l'envoyer au serveur
			g->move=65; // si scanf correct cette valeur est modifiée, sinon cela terminera la partie. 
			
            afficher_plateau_bitboard(plateau);
            
			move_choisi=64;

            if (nb_coup_jouees>42) Depth=18;// profondeur max il calcule tout jusqu'au bout
            else if (nb_coup_jouees>35) Depth= 8; // augmente la profondeur en fin de partie
            else if (nb_coup_jouees>25) Depth= 6; 
			

			struct timeval tv1,tv2;
			struct timezone tz;
			int diff1,diff2;
			gettimeofday(&tv1,NULL);

			debut=clock();

			minimax_bitboard_ia(9,&move_choisi,-INF-10,INF+10,g->myColor,coeffs_ia,plateau); 	

			gettimeofday(&tv2,NULL);
			
			
			diff1=(tv2.tv_sec-tv1.tv_sec);
			diff2=(tv2.tv_usec-tv1.tv_usec)/1000;
			printf("durée=%d sec %d usecn \n",diff1,diff2);
			if (diff1 > worst_diff) worst_diff= diff1;
			fin=clock();
    		double temps_ecoule = (double)(fin -debut)/ CLOCKS_PER_SEC;
			if (pire_temps< temps_ecoule){ pire_temps=temps_ecoule; stade_jeu=nb_coup_jouees; }
			g->move=move_choisi;
//NOTE ENLEVER AFTER

			printf("playing move %d (x=%d,y=%d)\n",g->move,g->move%8,g->move/8);
			doMoveOthello(g);	// envoie du coup à l'adversaire/ au serveur
            
            if (g->move != 64) {jouer_coup_bitboard(&plateau,g->move,g->myColor);// jouer le coup sur notre plateau
            nb_coup_jouees++;}
            afficher_plateau_bitboard(plateau);// affiche le coup qu'on a joue
	   	}
		g->currentPlayer=!g->currentPlayer; 
	} 
	// fin de partie 
	printf("Final game status = %d\n",g->state); 
	printf("I was player %s\n",(g->myColor==BLACK)?"Black":"White"); 

    printf("Pire temps execution : %f secondes pour nb_coups_jouees:%d \n", pire_temps,stade_jeu);
	printf("worst diff %d",worst_diff);
    int scoreB=__builtin_popcountll(plateau.blanc);int scoreN=__builtin_popcountll(plateau.noir); 
    if ((g->myColor) && (scoreB> scoreN )) printf("on a tabassé ce prof \n");
    else if ((!g->myColor) && (scoreB< scoreN ) ) printf("on a tabassé ce prof \n");
    else if (scoreB == scoreN) printf("Nulle \n");
	else printf("quelle indignité \n");
	printf("Score B: %d;  Score N:%d \n",scoreB, scoreN);


	freeGameOthello(g);
	return 0; 
}


