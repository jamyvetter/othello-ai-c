#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "userNetwork.h"
#include "fonctions.h"

// Programme US4: permet de faire jouer un humain contre une IA en local (qui joue selon un temps donné)
// Fonctionne dans le dossier serveur4, compile avec:


/*Variables globales pour la gestion du temps ecoule*/
double limite_temps = 2; // secondes de réflexion autorisées
double debut;
bool temps_depasse =false;
position_connue opening_book[TAILLE_LIVRE_OUVERTURE];


int main(){

    srand(time(NULL)); // Intialisation du random, faire qu'une seule fois sinon bug

    // Stockage de notre plateau de jeu
    Plateau plateau;
    initialiser_plateau_bitboard(&plateau);
    init_zobrist(); // la table de correspondance des hash
    uint64_t hash= calculer_hash_complet(plateau.noir, plateau.blanc,false); // le hash est de base à celui de la position initiale, avec trait aux noirs
	init_hash_ouvertures(opening_book);

    int nb_coup_jouees=0; //compte le nombre de jeton poses pour avoir une idee de ou en est la partie

    bool tour=false; // qui peut jouer, true-> Blancs ; false-> Noirs
    int p; // valeur du coup a jouer, 0<=p<=63


    //Variables de score
    int scoreB, scoreN;

    bool ia_a_les_blancs=false; // apres demande pr savoir quelle couleur joue l'IA: Noir=false; true= blanc
    while(!fin_de_partie_bitboard(plateau)){
        if (peut_jouer_bb(tour?plateau.blanc:plateau.noir,tour?plateau.noir:plateau.blanc)) { // sinon on lui passe le tour
            
            if ((ia_a_les_blancs && tour) || (!ia_a_les_blancs && !tour)){ // c'est un NXOR mais flemme d'opti
                        
                ordi_random_bitboard(&p,tour,plateau); 
            }
            else{ // JOUEUR 2: HUMAIN  
                do {
                    printf("Entre un coup valide sous format: ex:f5 \n");
                    char colonne; int ligne;
                    scanf(" %c%d", &colonne,&ligne); //On me un espace pour éviter le bug de double demande (sinon la solution alternative est while(getchar\n))
                    p= (colonne - 'a') + (ligne - 1)*8;
			printf("Coup compris: %d \n",p);
                } while ( p<0 || p>63 || !est_coup_legal(plateau,tour,p));
                
            }
            printf("On va jouer jouer le coup %d\n",p);
            jouer_coup_bitboard(&plateau,p,tour);
            afficher_plateau_bitboard(plateau);
            nb_coup_jouees++;
        }
        else{
            printf("On a du passer le tour du joueur par manque de coup legaux\n");
        } 
        tour=!tour; // changement de joueur    
        }	
        scoreB=__builtin_popcountll(plateau.blanc);scoreN=__builtin_popcountll(plateau.noir); 
        int res=0; // 0 = nulle, -1= IA gagne et 1: joueur humain gagne
        if( ((ia_a_les_blancs) && (scoreB> scoreN )) || ((!ia_a_les_blancs) && (scoreB< scoreN ) ) )printf("L'IA a gagne, impressionnant!\n");
        else if (scoreB == scoreN) printf("C'est une partie nulle, impressionnant!\n");
        else printf("L'humain a gagne, impressionnant!\n");


        return 0;
    }
