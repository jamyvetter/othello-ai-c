#include <stdio.h> 
#include <stdlib.h> 
#include <stdint.h> 
#include <errno.h>

// Programme pour US2 , verifie que la fonction jouerCoup ou jouercoupavechistorique (ou ici faireCoupOthello) fonctionne. 
//Compiler avec gcc .\benchmark_othello.c -o benchmark_othello -Wall -Wextra
// executer avec ./benchmark_othello savedGames_

#define N 8 
#define BLACK 1 
#define WHITE 0 
#define EMPTY 2 

#define NOIRS BLACK
#define BLANCS WHITE // redefini les constantes de l'ancien programme pour garder la meme fonction jouer_coup
#define VIDE EMPTY

#define CONVENTIONOK 

#ifdef CONVENTIONOK 
typedef int8_t monType ;
#else 
typedef int32_t monType;
#define MYBLACK 1 
#define MYWHITE 0 
#define MYEMPTY 2 
#endif 



int dansTableau(int i, int j) 
{
	return ((i>=0) && (i<N) && (j>=0) && (j<N)); 
}

int deplacement_x[8]= {-1, 0, 1 , -1,1, -1, 1,0};
int deplacement_y[8]={-1, -1, -1, 0,0, 1,1,1}; // définit les vecteurs de déplacement sur l'axe x et y, 8 déplacements possibles, pour le fonction jouerCOup/fairecoupothello


// Assume we have a function that modifies our board 
 
int faireCoupOthello(monType tableau[N][N],int8_t coup,int8_t couleur)

//c'est le code adapté de :int jouer_coup(int x, int y,bool tour,char plateau[N][N]) // prend en parametre les coordonnées d'un coup valide à jouer dans le plateau (x,y)et le joue; 
// sortie:  renvoie true si le coup est valide (a bien retourné une tuile), false sinon. Si le coup est invalide (ne retourne pas de case), on ne place pas le jeton
{
    int coup_valide=0;
    int our_color, ennemy_color;
	int x=coup/8, y=coup%8;
   if (couleur==1) {our_color=NOIRS; ennemy_color=BLANCS;}
   else {our_color=BLANCS; ennemy_color=NOIRS;}
   
    if (tableau[x][y]==VIDE){ // verif en plus sur code du prof qu'on ecrit pas sur une case à nous; optionnel car dans notre code on verifie que le coup est légal avant de le jouer
        tableau[x][y]=our_color;//place notre jeton

        for (int i=0; i<N; i++){
            int x_suiv=x+deplacement_x[i] , y_suiv=y+deplacement_y[i];

            while( dansTableau(x_suiv,y_suiv) && tableau[x_suiv][y_suiv]==ennemy_color ){ // normalement le && regarde le deuxieme que quand le premier est valide, donc pas d'overflow
            x_suiv= x_suiv + deplacement_x[i] ; y_suiv= y_suiv + deplacement_y[i]; //on regarde la case suivante

            if(dansTableau(x_suiv,y_suiv) && tableau[x_suiv][y_suiv] == our_color) {
                while((x_suiv != x ) || (y_suiv!= y) ){             // pour chemin inverse tout retourner jusqu'au jeton de base
                    x_suiv=x_suiv-deplacement_x[i] ; y_suiv= y_suiv - deplacement_y[i]; 
                    tableau[x_suiv][y_suiv]= our_color; // on retourne les jetons intercalés
                    coup_valide=1;
                }
            }
            }

        }
        if (!coup_valide) tableau[x][y]=VIDE; // on remet la case intiale, le coup n'est pas bon
    }

return coup_valide;
}



//On definit deux structures afin de jouer un coup et de retenir les cases affectées par ce coup (pour pouvoir faire des retours en arrière)
typedef struct {
    int x, y; // les coordonnes de la case changee
    char old_value; // le contenue précédent (BLANC, NOIR ou VIDE)
} Changement; // la structure pour les changements d'état d'une case

typedef struct {
    Changement modifs[64];
    int nb_modifs;
} HistoriqueCoup;

int faireCoupOthello_avec_historique(monType tableau[N][N],int8_t coup,int8_t couleur ,HistoriqueCoup *hist) { // prend en parametre les coordonnées d'un coup à jouer dans le plateau (x,y)et le joue; 
// sortie:  renvoie true si le coup est valide (a bien retourné une tuile), false sinon. Si le coup est invalide (ne retourne pas de case), on ne place pas le jeton
// + modifie les pointeurs pour enregistrer les etats precedents des jetons retournes

    bool coup_legal=false; // variable de retour

    //char our_color = tour ? BLANCS : NOIRS;
    //char ennemy_color = tour ? NOIRS : BLANCS;

    int our_color, ennemy_color;
	int x=coup/8, y=coup%8;
   if (couleur==1) {our_color=NOIRS; ennemy_color=BLANCS;}
   else {our_color=BLANCS; ennemy_color=NOIRS;}

    if (tableau[x][y]!=VIDE) return false; // Pas le droit de jouer si la case n'est pas vide

    hist->nb_modifs = 0;
    // Joue la case elle-même
    hist->modifs[hist->nb_modifs++] = (Changement){x, y, tableau[x][y]};
    tableau[x][y] = our_color;

    // Parcours les 8 directions
    for (int d = 0; d < 8; d++) {
        int dx = deplacement_x[d];
        int dy = deplacement_y[d];
        int x_suiv = x + dx, y_suiv = y + dy;
        bool ligne_valide = false;

        while (dansTableau(x_suiv, y_suiv) && tableau[x_suiv][y_suiv] == ennemy_color) { // On parcourre jusqu'a trouver un jeton qui n'est pas a l'ennemi ou si on sort du plateau
            x_suiv += dx; y_suiv += dy;
            ligne_valide = true;
        }

        if (ligne_valide && dansTableau(x_suiv, y_suiv) && tableau[x_suiv][y_suiv] == our_color) { // verifie que le jeton à l'extrémité est à nous
            // Revenir en arrière pour retourner les pions -> ici pas un retour en arrière mais on reparcouure le plateau pour jusqu'au jeton final a retourner
            int rx = x + dx, ry = y + dy;
            while (rx != x_suiv || ry != y_suiv) {
                hist->modifs[hist->nb_modifs++] = (Changement){rx, ry, tableau[rx][ry]};
                tableau[rx][ry] = our_color; coup_legal=true; // à ce stade un jeton a été retourné donc le coup fait bien qq chose
                rx += dx;
                ry += dy;
            }
        }
    }

    if (!coup_legal) tableau[x][y]=VIDE;
    return coup_legal;
}

void annuler_coup(HistoriqueCoup *hist, char tableau[N][N]) {
    for (int i = 0; i < hist->nb_modifs; i++) {
        tableau[hist->modifs[i].x][hist->modifs[i].y] = hist->modifs[i].old_value;
    }
}
// fin new




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


int lireCoup(int8_t *coup,int8_t *couleur, FILE *savedGames) 
{
	int res=0; 
	res=fread(coup,sizeof(int8_t),1,savedGames); 
	if (res) res=fread(couleur,sizeof(int8_t),1,savedGames); 
#ifndef CONVENTIONOK 
	*couleur=(*couleur==BLACK)?MYBLACK:MYWHITE; 
#endif 
	return res; 
}

int lireTableau(monType tableau[N][N], FILE *savedGames) 
{
	int res; 
#ifdef CONVENTIONOK 
	res=fread(tableau,sizeof(int8_t),64,savedGames); 
#else 
	int8_t temp[N][N]; 
	res=fread(temp,sizeof(int8_t),64,savedGames); 
	for (int i=0;i<N*N;i++) {
		tableau[i/8][i%8]=((temp[i/8][i%8]==BLACK)?MYBLACK:(temp[i/8][i%8]==WHITE)?MYWHITE:MYEMPTY); 
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
			printf("tableau initial\n"); 
			afficheTableau(monTableau); 
			lireCoup(&c,&p,fi); 
			lireTableau(after,fi); 
			printf("coup de %d en %d %d\n",p,c/8,c%8); 
			// ici mettez l'appel à votre fonction 
			//HistoriqueCoup hist;
            //faireCoupOthello_avec_historique(monTableau,c,p,&hist); // L1
            faireCoupOthello(monTableau,c,p); // L2

            //Pour faire le test, commenter un des deux appels

			printf("tableau ref\n"); 
			afficheTableau(after); 
			afficheTableau(monTableau); 
			if ((errors=compareTableau(monTableau,after))) { 
				printf("%d error(s) in test %d\n",errors,nbtest);
		       		nberrors+=errors;
			}
		}
	fclose(fi); 
	} else {
		perror("cannot open file"); 
	}
	printf("J'ai detecte %d Erreur(s)\n",nberrors); 
}

