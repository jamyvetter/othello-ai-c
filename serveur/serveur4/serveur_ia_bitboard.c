// Programme pour les 1v1s othello (le tournoi sur le serveur)

// Version optimisée avec hashage zobrist et profondeur adaptative

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


/*Variables globales pour la gestion du temps ecoule*/
double limite_temps = 14; // secondes ----------------------------------A changer en 14-15s pr le tournoi, à tester ------------------
double debut;
bool temps_depasse =false;

position_connue opening_book[TAILLE_LIVRE_OUVERTURE];

// Mode serveur
int main(int argc,char *argv[]) 
{	
	//---------------
	//deja mis dans fonctions.c, ce sont les coeffs utilises
	//int coeffs_ia[10]={88, 15, 34 ,60 ,35, -2 ,27}; // les nouveaux après entrainement intensif 
//------------------------
    // Stockage de notre plateau de jeu
    Plateau plateau;
    initialiser_plateau_bitboard(&plateau);
    init_zobrist(); // la table de correspondance des hash
    uint64_t hash= calculer_hash_complet(plateau.noir, plateau.blanc,false); // le hash est de base à celui de la position initiale, avec trait aux noirs
	init_hash_ouvertures(opening_book);

    int nb_coup_jouees=0; //compte le nombre de jeton poses pour avoir une idee de ou en est la partie

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
	
	printf("I am player %s\n",(g->myColor==BLACK)?"Black":"White"); 
    //printf("%d\n",g->myColor);

	// Début de partie
	while (g->state == PLAYING && !feof(stdin)) {
	 	if (g->myColor != g->currentPlayer) { // attente du coup de l'adversaire 
			if ((move=waitMoveOthello(g)) == 0 ) {
				//printf("Game status %d: \t",g->state); 
				if (g->state == PLAYING) { 
					//printf("Received move from server %d (x=%d,y=%d)\n",g->move,g->move%8,g->move/8);
                    // actualise notre plateau avec le coup de l'adversaire
                    if (g->move != 64){
						jouer_coup_bitboard_hash(&plateau,&hash,g->move,!g->myColor); 
						nb_coup_jouees++;
					}
					else{hash ^= ZOBRIST_TOUR;}
				}
			}
		}
	 	else { 		// C'est au tour de cette machine, on doit décider d'un coup et l'envoyer au serveur
			g->move=64; // Par défaut à la valeur pass, si minimax ne trouve pas de coup ça restera à PASS=64
			
            //afficher_plateau_bitboard(plateau);
            
			//Determination du meilleur coup:
			if (nb_coup_jouees<6){ // on recherche dans la base d'ouverture seulement si on est au début de la partie (pour ne pas perdre du temps)
				debut=obtenir_temps(); temps_depasse=false; // Reset des variables globales (sauf limite_temps)
				g->move= recherche_ouverture(hash);
				// On fait aussi les verifications de colision, si le coup a l'air valide on peut prendre le temps d'être sûr qu'il est légal
				if (g->move == -1 || g->move < 0 || g->move > 63 || 
				(!(coups_possibles( (g->myColor==BLACK)?plateau.noir:plateau.blanc , (g->myColor==BLACK)?plateau.blanc:plateau.noir )& (1ULL << g->move) ) ) ) goto recherche_classique; // on appelle minimax car on ne connait pas la position
			}
			else{
				// On calcule à une profondeur supplémentaire jusqu'à se faire timeout
				debut=obtenir_temps(); temps_depasse=false; // Reset des variables globales (sauf limite_temps)
				recherche_classique:
				for (int d=1; d<20; d++){ //---- A changer, on peut partir de plus haut sans trop de problemes ou pas car comme ça on trouve directe les coups prioritaires à évaluer
					minimax_hash_temporel(d, &(g->move) ,-INF-10,INF+10, g->myColor, plateau,hash);
					if (temps_depasse) { // Si la variable globale a été changée pour cause de temps dépassé
						printf("on a pu aller à Depth=%d \n", d-1 );//On indique jusqu'où on est allé //A commenter pour le tournoi et pas perdre de temps
						break;
					}
				}
			}



			doMoveOthello(g);	// envoie du coup à l'adversaire/ au serveur
            envoyer_coup_serveur(g->move%8,g->move/8); // Affiche le coup choisi par l'IA
			//printf("Determine a profondeur: d=%d", Depth );

            if (g->move != 64) {
				jouer_coup_bitboard_hash(&plateau,&hash,g->move,g->myColor);// jouer le coup sur notre plateau et actualise le hash
            	nb_coup_jouees++;
			}
			else{
				hash ^= ZOBRIST_TOUR; // on doit hasher le changement de tour dans la position
			}
            afficher_plateau_bitboard(plateau);// affiche le coup qu'on a joue
	   	}
		g->currentPlayer=!g->currentPlayer; 
	} 
	
	// Fin de partie
	afficher_plateau_bitboard(plateau);
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




