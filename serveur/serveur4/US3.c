// US3: Jouer les coups d'un joueur humain en utilisant l'API réseau
// A faire avant : export LD_LIBRARY_PATH=.

// BONUS: L'ia suggère au joueur le coup à jouer en réflechissant rapidement

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "userNetwork.h"
#include "fonctions.h"


/*Variables globales pour la gestion du temps ecoule*/
double limite_temps = 1; // secondes ----------------------------------Temps de réflexion pour l'aide IA ------------------
double debut;
bool temps_depasse =false;
position_connue opening_book[TAILLE_LIVRE_OUVERTURE];


int main(int argc,char *argv[]) 
{
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
    afficher_plateau_bitboard(plateau);
	// Début de partie
	while (g->state == PLAYING && !feof(stdin)) {
	 	if (g->myColor != g->currentPlayer) { // attente du coup de l'adversaire 
			if ((move=waitMoveOthello(g)) == 0 ) {
				printf("Game status %d: \t",g->state); 
				if (g->state == PLAYING) { 
					printf("Received move from server %d (x=%d,y=%d)\n",g->move,g->move%8,g->move/8);
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
			
            afficher_plateau_bitboard(plateau);
            
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
				for (int d=1; d<20; d++){ //------------------------------------------------------------ A changer, on peut partir de plus haut sans trop de problemes
					//if (minimax_hash_temporel(d, &(g->move) ,-INF-10,INF+10, g->myColor,  coeffs_ia, plateau,hash)==678 ) break; // Minimax n'a pas eu le temps de finir son execution, on stoppe les appels à minimax
					minimax_hash_temporel(d, &(g->move) ,-INF-10,INF+10, g->myColor, plateau,hash);
					if (temps_depasse) {
						printf("on a pu aller à Depth=%d \n", d-1 );// A commenter pour le tournoi et pas perdre de temps
						break;
					}
					d++;
				}
			}
		printf("L IA pense que le bon coup est:");
            envoyer_coup_serveur(g->move/8,g->move%8); // Affiche le coup choisi par l'IA (ici US3 donc c'est juste une petite aide)

            // Choix du coup de l'humain
            if ( peut_jouer_bb( (g->myColor==BLACK)?plateau.noir:plateau.blanc , (g->myColor==BLACK)?plateau.blanc:plateau.noir ) ){ // Sinon le coup reste 64=pass
                do {
                    printf("Entre un coup valide sous format: ex:f5 \n"); printf("Tu joues les: %s\n",(g->myColor==BLACK)?"Black":"White"); // On rappelle au joueur qui il est
                    char colonne; int ligne;
                    scanf(" %c%d", &colonne,&ligne); //On me un espace pour éviter le bug de double demande (sinon la solution alternative est while(getchar\n))
                    g->move= (colonne - 'a') + (ligne - 1)*8 ;
		    printf("coup compris: %d\n",g->move);
                } while ( g->move<0 || g->move>63 || !est_coup_legal(plateau,g->myColor,g->move));
            }
            else{
                printf("Tu ne peux pas jouer, on passe ton tour\n");
            }

			doMoveOthello(g);	// envoie du coup à l'adversaire/ au serveur
            

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


