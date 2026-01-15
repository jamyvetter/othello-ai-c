#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "fonctions.h"

// Programme US1: permet de faire jouer une partie d'othello à deux humains en local
// Fonctionne dans le dossier serveur4, compile avec:
//gcc US1.c fonctions.o fonctions.h -o US1 -Wall -Wextra

// Je viens d'apprendre que des variables globales doivent forcément apparaitre quelque part...
// J'en mets donc ici des inutiles, mais ça sert pour compiler
// Pour régler ce problème, il faut les déclarer dans fonctions.c vu qu'elles sont utilisées là bas

double limite_temps=0.1,debut;
bool temps_depasse=false;
position_connue opening_book[TAILLE_LIVRE_OUVERTURE];

int main(){

    // Stockage de notre plateau de jeu
    Plateau plateau;
    initialiser_plateau_bitboard(&plateau);
    
    bool tour=false; // qui peut jouer, true-> Blancs ; false-> Noirs
    int p; // valeur du coup a jouer, 0<=p<=63
	
    while(!fin_de_partie_bitboard(plateau)){
		afficher_plateau_bitboard(plateau); printf("C'est au tour des: %s \n",tour?"Blancs":"Noirs");
        
		if (peut_jouer_bb(tour?plateau.blanc:plateau.noir,tour?plateau.noir:plateau.blanc)) { // sinon on lui passe le tour
            do {
                printf("Entre un coup valide sous format: ex:f5 \n");
                char colonne; int ligne;
                scanf(" %c%d", &colonne,&ligne); //On me un espace pour éviter le bug de double demande (sinon la solution alternative est while(getchar\n))
                p= (colonne - 'a') + (ligne - 1)*8;
            } while ( p<0 || p>63 || !est_coup_legal(plateau,tour,p));
                
            printf("On va jouer jouer le coup determine: %d\n",p);
            jouer_coup_bitboard(&plateau,p,tour);          

        }
        else{
            printf("On a du passer le tour du joueur par manque de coup legaux\n");
        } 
        tour=!tour; // changement de joueur    
    }	
    	printf("Fin de la partie!\n");    
		afficher_plateau_bitboard(plateau);
        int scoreB, scoreN;//Variables de score
        scoreB=__builtin_popcountll(plateau.blanc);scoreN=__builtin_popcountll(plateau.noir); 
        if( scoreB> scoreN )printf("Les blancs ont gagnes; scoreB= %d - scoreN= %d \n",scoreB,scoreN);
        else if( scoreN> scoreB )printf("Les noirs ont gagnes; scoreB= %d - scoreN= %d \n",scoreB,scoreN);
        else printf("Chess played perfectly is a draw (ok c'est du othello)\n");

        return 0;
    }


