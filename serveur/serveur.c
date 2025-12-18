#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "userNetwork.h"
#include "fonctions.h"


// Mode serveur

int nb_coup_analyses=0; // le nombre de position regardees avant chaque decision
int Depth=4; // la profondeur pour les appels de minimax
int nb_coup_jouees=0;// Le stade de la partie en cours

int main(int argc,char *argv[]) 
{
    // Stockage de notre plateau de jeu
    char plateau[N][N];
    initialiser_plateau(plateau); // même pas besoin mais ça fait pas de mal
    int x; int y; //variables pour le stockage des coordonnées dans le tableau correspondant au coup qui va être joué
    nb_coup_jouees=0; //compte le nombre de jeton poses pour avoir une idee de ou en est la partie

	game *g;
	int move; 

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

	// debut de partie
	while (g->state == PLAYING && !feof(stdin)) {
	 	if (g->myColor != g->currentPlayer) { // attente du coup de l'adversaire 
			if ((move=waitMoveOthello(g)) == 0 ) {
				printf("Game status %d: \t",g->state); 
				if (g->state == PLAYING) { 
					printf("Received move from server %d (x=%d,y=%d)\n",g->move,g->move%8,g->move/8);
                    // actualise notre plateau avec le coup de l'adversaire
                    if (g->move != 64){jouer_coup(g->move/8,g->move%8,g->currentPlayer,plateau); nb_coup_jouees++;}
				}
			}
		}
	 	else { 		// C'est au tour de cette machine, on doit décider d'un coup et l'envoyer au serveur
			g->move=65; // si scanf correct cette valeur est modifiée, sinon cela terminera la partie. 
			
            afficher_plateau(plateau);
			
            x=-1;y=-1;
            jouer_api_ia(&x,&y,&(g->myColor),plateau);
            //jouer_api_humain(&x,&y);
			
            traduction_x_y_vers_entier(x,y,&(g->move)); // modifie g->move pour qu'il mette le coup décidé dans le format attendu par le serveur
            printf("playing move %d (x=%d,y=%d)\n",g->move,g->move%8,g->move/8);
			doMoveOthello(g);	// envoie du coup à l'adversaire/ au serveur
            
            if (g->move != 64) {jouer_coup(g->move/8,g->move%8,g->myColor,plateau);// jouer le coup sur notre plateau si il y a bien eu un coup (pas de pass)
            nb_coup_jouees++;}
            afficher_plateau(plateau);// affiche le coup qu'on a joue
	   	}
		g->currentPlayer=!g->currentPlayer; 
	} 
	// fin de partie 
	printf("Final game status = %d\n",g->state); 
	freeGameOthello(g);
	return 0; 
}

/* A faire pour s'améliorer: 
-Interface utilisateur - serveur

-Mode 1v1 IAs pour les comparer et trouver les bons paramètres de coeff de w1,w2,w3...
- Pour la profondeur (Depth), il faut que le programme aille à une profondeur différente selon la phase du jeu( plus ou moins de coups à analyser) sans se faire timeout

-Ouvertures: implémenter une table d'ouverture pour le début de partie. -> pas le plus important il joue déjà bien au début


*/