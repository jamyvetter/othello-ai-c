#include <stdio.h>
#include <stdbool.h>
#include "fonctions.h"

int deplacement_x[8]= {-1, 0, 1 , -1,1, -1, 1,0};
int deplacement_y[8]={-1, -1, -1, 0,0, 1,1,1}; // définit les vecteurs de déplacement sur l'axe x et y, 8 déplacements possibles



/* Attention, le plateau est codé en C donc
plateau[x][y] avec x la ligne associée de haut en bas et y la colonne de gauche à droite 
(en résumé par rapport aux maths, x_tab= axe -y  et y_tab= axe x)

        Pour plateau[x][y]:
    a b c d e f g h -> axe y
1
2
3
4
5
6
7
8
|\ axe x vers le bas

*/

void initialiser_plateau(char plateau[N][N]){ // crée la configuration de depart
	for (int i=0;i<N;i++){
		for (int j=0;j<N;j++){
			plateau[i][j]=VIDE;
}
}
plateau[3][3] = BLANCS;
plateau[4][4] = BLANCS;
plateau[3][4] = NOIRS; // position intitiale
plateau[4][3] = NOIRS;
}



void afficher_plateau(char plateau[N][N]){ //affiche le plateau
	//affiche les coordonnées des colonnes
	printf("   a||b||c||d||e||f||g||h| \n");
    for (int i=0;i<N;i++){
		printf("%d ", i+1);
        for (int j=0;j<N;j++) {
			printf("|%c|", plateau[i][j]);
		}
		printf("\n");
	}
	printf("\n\n\n");
}


bool coup_dans_plateau(int x, int y){ // teste si les coordonnées du coup sont valides
	return ((x>=0) && (x<N) && (y>=0) && (y<N));
}


bool jouer_coup(int x, int y,bool tour,char plateau[N][N]) // prend en parametre les coordonnées d'un coup valide à jouer dans le plateau (x,y)et le joue; 
// sortie:  renvoie true si le coup est valide (a bien retourné une tuile), false sinon. Si le coup est invalide (ne retourne pas de case), on ne place pas le jeton
{
    bool coup_valide=false;
    char our_color, ennemy_color;
   if (tour) {our_color=BLANCS; ennemy_color=NOIRS;}
   else {our_color=NOIRS; ennemy_color=BLANCS;}
   
    plateau[x][y]=our_color;//place notre jeton

    for (int i=0; i<N; i++){
        int x_suiv=x+deplacement_x[i] , y_suiv=y+deplacement_y[i];

        while( coup_dans_plateau(x_suiv,y_suiv) && plateau[x_suiv][y_suiv]==ennemy_color ){ // normalement le && regarde le deuxieme que quand le premier est valide, donc pas d'overflow
        x_suiv= x_suiv + deplacement_x[i] ; y_suiv= y_suiv + deplacement_y[i]; //on regarde la case suivante

        if(plateau[x_suiv][y_suiv] == our_color) {
            while( (x_suiv != x ) || (y_suiv!= y) ){             // pour chemin inverse tout retourner jusqu'au jeton de base
                x_suiv=x_suiv-deplacement_x[i] ; y_suiv= y_suiv - deplacement_y[i]; 
                plateau[x_suiv][y_suiv]= our_color; // on retourne les jetons intercalés
                coup_valide=true;
            }
        }
        }

    }

    if (!coup_valide) plateau[x][y]=VIDE; // on remet la case intiale, le coup n'est pas bon
return coup_valide;
}


bool action_effectuee(int x, int y,bool tour,char plateau[N][N]){ // dis si le coup retourne au moins une tuile
    bool coup_valide=false;
    char our_color, ennemy_color;
   if (tour) {our_color=BLANCS; ennemy_color=NOIRS;}
   else {our_color=NOIRS; ennemy_color=BLANCS;}

   int i=0;
    while ((!coup_valide) && (i<N) ){ // condition d'arret: on a trouve que le coup est valide ou fait tous les déplacements possibles
        int x_suiv=x+deplacement_x[i] , y_suiv=y+deplacement_y[i];

        while( coup_dans_plateau(x_suiv,y_suiv) && plateau[x_suiv][y_suiv]==ennemy_color && (!coup_valide) ){
        x_suiv= x_suiv + deplacement_x[i] ; y_suiv= y_suiv + deplacement_y[i]; //on regarde la case suivante

        if ( plateau[x_suiv][y_suiv] == our_color ) { // à ce stade on est allé à 2 cases minimum de la case intitiale, donc si on trouve un jeton à nous, on peut faire un retournement
                coup_valide=true; // si on arrive ici, le coup est légal car il peut retourner des tuiles
        }
        }

    i++;
    }
return coup_valide;
}



bool coup_valide(int x, int y, bool tour, char plateau[N][N]){ // vérifie qu'un coup respecte toutes les règles du jeu (le coup peut être accepté par lire_coup())
	if ( (coup_dans_plateau(x,y) ) && (plateau[x][y]==VIDE) && (action_effectuee(x, y,tour, plateau))  ) return true;
return false;
}

void score_joueur(int *score_blanc, int *score_noir, char plateau[N][N]) { //compte les pions
    *score_blanc = 0;
    *score_noir = 0;

    for (int i=0;i<N; i++) {
        for (int j = 0;j<N;j++) {
            if (plateau[i][j] == BLANCS) { (*score_blanc)++; }
            else if (plateau[i][j] == NOIRS) { (*score_noir)++;}
        }
    }
}




void lire_coup(int *x, int *y,bool tour) // lis un coup entré au clavier et le stocke en format pour le tableau dans x,y
{  // optimisation: si ecrit en majuscule traduire aussi
	char colonne;
	int ligne;
	printf("Entrez votre coup Mr %s (ex: e6) : ", tour ? "Blancs [B]" : "Noirs [N]");// dire a qui c'est
	scanf("%c%d",&colonne,&ligne);
	printf("\n");
    *y = colonne - 'a';  // a-h → 0-7
    *x = ligne - 1;      // 1-8 → 0-7
}

 
 
bool peut_jouer(bool tour, char plateau[N][N]) { // dit si un joueur peut jouer, autrement il devra passer son tour
    for (int i=0;i<N;i++) {
        for (int j=0;j<N;j++) {
            if (plateau[i][j] == VIDE && action_effectuee(i, j,tour, plateau)) {
                return true;
            }
        }
    }
    return false;
}


bool fin_de_partie(char plateau[N][N]) { // détecte la fin du jeu
    return (!peut_jouer(true,plateau) && !peut_jouer(false,plateau));
}


void gagnant(char plateau[N][N]) {// dit qui a gagné à la fin de la partie
    int scoreB, scoreN;
    score_joueur(&scoreB, &scoreN, plateau);

    printf("\n=== Fin de la partie ===\n");
    printf("Score Blancs (B): %d\n", scoreB);
    printf("Score Noirs  (N): %d\n", scoreN);

    if (scoreB > scoreN)
        printf("Les Blancs gagnent !\n");
    else if (scoreN > scoreB)
        printf("Les Noirs gagnent !\n");
    else
        printf("Égalité !\n");
}





int main(){

//Initialisation de l'othellier
char plateau[N][N];
initialiser_plateau(plateau);

bool tour=false; // qui peut jouer, true-> Blancs ; false-> Noirs
int x; int y; //stockage des coordonnées dans le tableau correspondant au coup qui va être joué


printf("=== Debut de la partie ===\n");
afficher_plateau(plateau);

//Initialisation des scores
int scoreB, scoreN;
score_joueur(&scoreB, &scoreN, plateau);
//printf("Score initial; Blancs: %d, Noirs: %d\n", scoreB, scoreN);  // enlevé car pas utile au début

		//char joueur; 
    while(!fin_de_partie(plateau)){
        //joueur= tour ? BLANCS : NOIRS;
		if (peut_jouer(tour, plateau)) {
			do {
				lire_coup(&x, &y, tour);
			} while (!coup_valide(x, y, tour, plateau));
			
			jouer_coup(x, y,tour, plateau);
			afficher_plateau(plateau);					  // on affiche l'état du plateau a chaque tour
				
			score_joueur(&scoreB, &scoreN, plateau);	
			printf("Score B:%d  N:%d\n", scoreB, scoreN); //on affiche le score actuel à chaque tour
		} else {
			printf("Le joueur %s ne peut pas jouer, on passe le tour.\n", tour ? "Blanc" : "Noir"); // pour le serveur, on sait pas encore si on doit jouer le coup je passe mon tour ou juste pas jouer
		}
			
		tour=!tour; // changement de joueur
	}	

	// partie finie
	gagnant(plateau);
	return 0;
}
