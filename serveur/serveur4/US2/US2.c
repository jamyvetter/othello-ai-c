//US2
//  Code pour vérifier la mécanique de jeu (jouer_coup)
// Vérifie aussi que le hashage des positions fonctionne et que la fonction pour savoir si le coup est legal fonctionne aussi.
// Compile avec gcc .\US2.c .\fonctions.c -o US2 -Wall -Wextra
// s'éxécute avec .\US2 .\savedGames_

// NOTE: Tous les tests sont validés, et infos en plus dans la fonction faire_coup sur un 'bug' vu.

#include <stdio.h> 
#include <stdlib.h> 
#include <stdint.h> 
#include <errno.h>
#include <stdbool.h>
#include "fonctions.h" 

#define N 8 
#define TEST_BLACK 1 // redefini car dans mon fonctions.h j'utilise deja #define BLACK 0 
#define WHITE 0 
#define EMPTY 2 

#define CONVENTIONOK 

#ifdef CONVENTIONOK 
typedef int8_t monType ;
#else 
typedef int32_t monType;
#define MYBLACK 1 
#define MYWHITE 0 
#define MYEMPTY 2 
#endif 

//---------------------------------------------------
// variables globales obligatoirs, explique dans US1 ce pb
double limite_temps=0.1;double debut;bool temps_depasse=false;
position_connue opening_book[TAILLE_LIVRE_OUVERTURE];
//---------------------------------------------------


void afficheTableau(monType tableau[N][N]) 
{
	for (int i=0;i<N;i++) {
		for (int j=0;j<N;j++) {
			printf("%c ",tableau[i][j]==2?'.':'0'+tableau[i][j]); 
		} 
		printf("\n"); 
	}
	printf("\n\n\n"); 

}



// Assume we have a function that modifies our board 
int faireCoupOthello(monType tableau[N][N],int8_t coup,int8_t couleur){
	Plateau p={0,0};
	// copie les donnees de tableau[N][N] dans notre structure de Bitboard
	for(int i=0; i<N*N; i++){
		if (tableau[i/8][i%8] == TEST_BLACK) {
            p.noir |= (1ULL<< i);
        }
		else if (tableau[i/8][i%8] == WHITE) {
            p.blanc |= (1ULL<< i);
        }
	}
	//jouer_coup_bitboard(&p, coup, couleur); // Joue le coup sur les Bitboard
	
    
        // Jouer le coup demandé
        // NOTE: erreur si le coup demande est 64, par choix de codage je décide que on verifie à l'exterieur de jouer_coup que le coup ne soit pas pass.
        // ça permet notamment de faire moins de requete if move!= 64 dans chaque appel de jouer_coup dans minimax (ce qui serait long) 
        Bitboard nouveau_hash= calculer_hash_complet(p.noir,p.blanc,couleur);
        // On vérifie au passage la fonction coups_possibles pour voir si on savait bien que ce coup est legal
        if ( coups_possibles(couleur?p.blanc:p.noir,couleur?p.noir:p.blanc)& (1ULL << coup) ) jouer_coup_bitboard_hash(&p, &nouveau_hash, coup,couleur);
        //jouer_coup_bitboard(&p, coup, couleur); // Commenté mais marche aussi
         
        
        if (coup==64) nouveau_hash ^= ZOBRIST_TOUR; // Si le coup est pass, on doit hasher le tour 
        
        if (calculer_hash_complet(p.noir,p.blanc,!couleur) != nouveau_hash) { // SI le hashage a mal fonctionné( jouer_coup_bitboard n'a pas bien modifié le hash)    
            printf("erreur de hashage pour le coup:%d hashs: %lx %lx %lx\n",coup,nouveau_hash,calculer_hash_complet(p.noir,p.blanc,!couleur), nouveau_hash^ZOBRIST_TOUR);
            printf("plateau avant le coup:\n");afficheTableau(tableau);
            printf("plateau après le coup:\n");afficher_plateau_bitboard(p);

            /*
            NOTE sur ces tests: Il y a une position du jeu de test qui fait jouer un coup illégal(une case déjà prise). C'est donc un cas où le hash 'bug' car il n'actualise pas le changement de tour. 
            (on sait ça car nouveau_hash^ZOBRIST_TOUR donne le bon hash)
            Mais en réalité on s'en fiche car en vraie partie on ne joue pas de coup illégaux

		Etonnament ce bug n'apparait plus maintenant sur linux, alors que sur Windows si ??

            */

        }



    //récupère les bitboards modifiés (le plateau) et les mets dans le tableau qu'on avait en entrée
	for(int i=0; i<N*N; i++){
		if ( (p.noir >> i ) & 1ULL) { // si il y a un jeton à l'index i sur le plateau noir
            tableau[i/8][i%8]= TEST_BLACK;
        }
		else if ( (p.blanc >> i ) & 1ULL) {
            tableau[i/8][i%8]= WHITE;
        }
	}
return 1; // pour stop le warning
}


int dansTableau(int i, int j) 
{
	return ((i>=0) && (i<N) && (j>=0) && (j<N)); 
}

int lireCoup(int8_t *coup,int8_t *couleur, FILE *savedGames) 
{
	int res=0; 
	res=fread(coup,sizeof(int8_t),1,savedGames); 
	if (res) res=fread(couleur,sizeof(int8_t),1,savedGames); 
#ifndef CONVENTIONOK 
	*couleur=(*couleur==TEST_BLACK)?MYBLACK:MYWHITE; 
#endif 
	return res; 
}

int lireTableau(monType tableau[N][N], FILE *savedGames) 
{
	int res; 
#ifdef CONVENTIONOK 
	res=fread(tableau,sizeof(int8_t),64,savedGames); //printf("-------/////-----////--/\n");
#else 
	int8_t temp[N][N]; 
	res=fread(temp,sizeof(int8_t),64,savedGames); 
	for (int i=0;i<N*N;i++) {
		tableau[i/8][i%8]=((temp[i/8][i%8]==TEST_BLACK)?MYBLACK:(temp[i/8][i%8]==WHITE)?MYWHITE:MYEMPTY); 
	}
#endif 
	return res; 
}
int compareTableau(monType tableau[N][N],monType reference[N][N]) 
{
	int erreur=0; 
	for (int i=0;i<64;i++) { 
		erreur+=(tableau[i/8][i%8]!=reference[i/8][i%8]); 
	}
	return erreur; 
}


int main(int argc, char *argv[])
{
    init_zobrist(); // pour vérifier que la hashage fonctionne


	// tableau before and after (ie. expected after) 
	monType monTableau[N][N]; // 1 Black, 0 white , 2 empty 
	monType after[N][N]; 
	int8_t  c,p; // coup de 0 à 63 pour un coup valide, 64 pour PASS. 
		     // p player (1 Black, 0 white) 
	int errors=0,nberrors=0; 
	int nbtest=0; 
	FILE *fi; 

	if (argc<2) { printf("%s <file>\n",argv[0]); exit (1); } 
	if ((fi=fopen(argv[1],"r"))!=NULL) {
		while (lireTableau(monTableau,fi)) {
			nbtest++; 
			//printf("tableau initial\n"); 
			//afficheTableau(monTableau); 
			lireCoup(&c,&p,fi); 
			lireTableau(after,fi); 
			//printf("coup de %d en %d %d\n",p,c/8,c%8); 
			
			faireCoupOthello(monTableau,c,!p);// ici mettez l'appel à votre fonction NOTE: j'ai convention inverse pour la couleur, donc je mets !p
			//printf("tableau ref\n"); 
			//afficheTableau(after); 
			//printf("Le tableau fait par ma fonction jouer_coup:\n");afficheTableau(monTableau); 
			if ((errors=compareTableau(monTableau,after))) { 
				printf("%d error(s) in test %d\n",errors,nbtest);
		       		nberrors+=errors;
			}
		}
	fclose(fi); 
	} else {
		perror("cannot open file"); 
	}
	printf("J'ai détecté %d Erreur(s)\n",nberrors); 

	return 0;
}


