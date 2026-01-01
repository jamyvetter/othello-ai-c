// Programme pour les 1v1s othello (le tournoi sur le serveur)

// IMPORTANT: ce n'est pas la meilleure version, juste une version de backup. La profondeur n'est pas dutout optimisée

//ligne d'execution: makefile sur github

// A faire avant : export LD_LIBRARY_PATH=.


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "userNetwork.h"
#include "fonctions.h"

// Mode serveur
int main(int argc,char *argv[]) 
{
		
	int coeffs_ia[10]={18 ,43, 89, 93 ,6 ,2 ,40};

    // Stockage de notre plateau de jeu
    Plateau plateau;
    initialiser_plateau_bitboard(&plateau);
    int move_choisi;
    int nb_coup_jouees=0; //compte le nombre de jeton poses pour avoir une idee de ou en est la partie
	int Depth= 8; //profondeur de base de l'IA
	game *g;
	int move; // utlisé par prof

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
	
	//printf("I am player %s\n",(g->myColor==BLACK)?"Black":"White"); 
    //printf("%d\n",g->myColor);

	// Début de partie
	while (g->state == PLAYING && !feof(stdin)) {
	 	if (g->myColor != g->currentPlayer) { // attente du coup de l'adversaire 
			if ((move=waitMoveOthello(g)) == 0 ) {
				//printf("Game status %d: \t",g->state); 
				if (g->state == PLAYING) { 
					//printf("Received move from server %d (x=%d,y=%d)\n",g->move,g->move%8,g->move/8);
                    // actualise notre plateau avec le coup de l'adversaire
                    if (g->move != 64){jouer_coup_bitboard(&plateau,g->move,!g->myColor); nb_coup_jouees++;}
				}
			}
		}
	 	else { 		// C'est au tour de cette machine, on doit décider d'un coup et l'envoyer au serveur
			g->move=65; // si scanf correct cette valeur est modifiée, sinon cela terminera la partie. 
			
            //afficher_plateau_bitboard(plateau);
            
			// On calcule à une profondeur adaptée selon l'avancée de la partie
			if (nb_coup_jouees>42) Depth=18;// profondeur max il calcule tout jusqu'au bout
            else if (nb_coup_jouees>35) Depth= 8; // augmente la profondeur en fin de partie
            else if (nb_coup_jouees>25) Depth= 6; 
			
			minimax_bitboard_ia(Depth,&(g->move),-INF-10,INF+10,g->myColor,coeffs_ia,plateau); 	

			//printf("playing move %d (x=%d,y=%d)\n",g->move,g->move%8,g->move/8);
			doMoveOthello(g);	// envoie du coup à l'adversaire/ au serveur
            
            if (g->move != 64) {
				jouer_coup_bitboard(&plateau,g->move,g->myColor);// jouer le coup sur notre plateau
            	nb_coup_jouees++;
			}
            //afficher_plateau_bitboard(plateau);// affiche le coup qu'on a joue
	   	}
		g->currentPlayer=!g->currentPlayer; 
	} 
	
	// Fin de partie
	printf("Final game status = %d\n",g->state); 
    
	int scoreB=__builtin_popcountll(plateau.blanc);int scoreN=__builtin_popcountll(plateau.noir); 
    if ((g->myColor) && (scoreB> scoreN )) printf("C'est une victoire avec les blancs! \n");
    else if ((!g->myColor) && (scoreB< scoreN ) ) printf("C'est une victoire avec les noirs! \n");
    else if (scoreB == scoreN) printf("Nulle \n");
	else printf("C'est une défaite... :( \n");
	printf("Score B: %d;  Score N:%d \n",scoreB, scoreN);

	freeGameOthello(g);
	return 0; 
}