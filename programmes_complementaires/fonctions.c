//Contient toutes les fonctions utiles à l'othello.


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "fonctions.h"
#include <stdint.h>

#define INF 999999 // valeur maximale d'evaluation théoriquement inateignable


int nb_coup_analyses=0; // le nombre de position regardees avant chaque decision
//int Depth=4; // la profondeur pour les appels de minimax
int nb_coup_jouees=0;// Le stade de la partie en cours

int deplacement_x[8]= {-1, 0, 1 , -1,1, -1, 1,0};
int deplacement_y[8]={-1, -1, -1, 0,0, 1,1,1}; // définit les vecteurs de déplacement sur l'axe x et y, 8 déplacements possibles


// Fonctions de gestion du plateau  --------------------------------------------------------------------

                                                //Fonctionnement du plateau:
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

void copierPlateau(char src[N][N], char dest[N][N]) { // copie le plateau dans un autre
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            dest[i][j] = src[i][j];
}

bool coup_dans_plateau(int x, int y){ // teste si les coordonnées d'un coup sont valides (dans le plateau)
	return ((x>=0) && (x<N) && (y>=0) && (y<N));
}
// Fin fonctions de gestion du plateau de char --------------------------------------------------------------------

















// Fonctions de gestion du bitboard  --------------------------------------------------------------------

/*
Note sur les bitboards: En realite, le plateau qui correspond à 
10
00 
(avec jeton en haut à gauche) sera stocké en binaire comme ça: 0001 
Cela permet que la constante 1ULL = le chiffre 1 correspond au bitboard avec une jeton en haut à gauche.
01234567
........
56 57 58 59 60 61 62 63
Et 64 = PASS


Il faut juste se dire que le bit de poids faible devient le bit de poids fort

Les directions/masques/shift. 8 directions pour jouer un coup.

NO: <<, +9, NOT_A_FILE     N: <<, 8,  NE: <<, 7, NOT_H_FILE
O: 1,<<, NOT_A_FILE                   E: 1,>>, NOT_H_FILE
SO: <<, +9, NOT_A_FILE    S: >>,8,    SE: >>, +9, NOT_H_FILE

Quand on fait des shitfs, cela correspond à l'operation *2^n. Les bits qui 'rentrent' valent 0 !
Par contre il faut des masques pour les décalages E/O car des bits peuvent rentrer alors qu'ils viennent pas du bon endroit

NOTE: il faut juste inverser les shifts (>> et << car codé à l'envers)
Et ausssi on peut faire un tableau pour simplifier tous les appels de chaque direction mais ça demandrait au processeur de à chaque fois trouver l'élement correspondant et ferait
perdre du temps au programme. C'est moche mais on, garde ça!
*/


void initialiser_plateau_bitboard(Plateau *jeu){
    // Comme dit avant, les shifts 'annulent' les autres bits, ici seuls les bits 27 et 36 sont gardés pour les blancs
    jeu->blanc = (1ULL << 27) | (1ULL << 36);   // en d4(27), e5(36)
    jeu->noir=  (1ULL << 28) | (1ULL << 35);    // en d5(35), e4(28) 
}

void afficher_bitboard(Bitboard b) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // On regarde si le bit à l'index (y*8 + x) est à 1
            int index = y * 8 + x;
            if ((b >> index) & 1ULL) printf("X ");
            else printf(". ");
        }
        printf("\n");
    }
    printf("\n");
}

void afficher_plateau_bitboard(Plateau jeu){ //affiche le plateau complet
	printf("  a|b|c|d|e|f|g|h| \n");
    for (int y=0;y<N;y++){
		printf("%d ", y+1);
        for (int x=0;x<N;x++) {
            // On regarde si le bit à l'index (y*8 + x) est à 1 sur l'un des joueurs
            int index = y * 8 + x;
            if ((jeu.noir >> index) & 1ULL) printf("N ");
            else if ((jeu.blanc >> index) & 1ULL) printf("B ");
            else printf(". ");
		}
		printf("\n");
	}
	printf("\n\n\n");
}

// ---Masques de decalage---
// Comme le bitboard est stocké lineairement, on ne veut pas passer du bit 7 au bit 8 quand on change de ligne, mais plutot du bit 7 au 15
// Les masques mettent a 0 les bits des 'bords' pour permettre le decalage
// NOT_H_FILE: 0111 1111 -> 0x7f
// NOT_A_FILE: 1111 1110 -> 0xFe (On inverse car memoire inverse ordre des bits)

const Bitboard NOT_A_FILE = 0xfefefefefefefefeULL; // Tout sauf colonne A
const Bitboard NOT_H_FILE = 0x7f7f7f7f7f7f7f7fULL; // Tout sauf colonne H

// Fonctions utilitaires pour calculer les pions retournés par un coup dans une direction
static inline Bitboard get_flips_dir(Bitboard move, Bitboard joueur, Bitboard adv, int shift, Bitboard mask) {
    Bitboard flips = 0;// les jetons retournes par le coup move
    Bitboard candidat = 0; // les candidats à retourner (une chaine de pions ennemis adjacente à notre jeton dans la bonne direction)
    
    Bitboard x = (move << shift) & mask & adv; // premier decalage dans la direction

    // Tant qu'on trouve des ennemis consécutifs, on les ajoute aux candidats
    // il y aura au max 6 ennemis donc on repete 6 fois.
    for(int i=0; i<6; i++) {
        if (x == 0) break; // fin de la chaine d'ennemis
        candidat |= x;     //ajout des candidats à la liste potentielle des jetons a retourner
        x = (x << shift) & mask & adv; // On avance dans la direction, il reste des ennemis
    }

    // on regarde si après la chaine d'ennemis consecutifs, il y'a un jeton à nous (on encadre les ennemis)
    Bitboard apres_candidat = (candidat << shift) & mask;
    if (apres_candidat & joueur) {
        flips = candidat; // La prise est valide
    }

    return flips;
}

// Fonctions utilitaires pour calculer les pions retournés par un coup dans une autre direction (inverse precedent)
static inline Bitboard get_flips_dir_inv(Bitboard move, Bitboard joueur, Bitboard adv, int shift, Bitboard mask) {
    Bitboard flips = 0;
    Bitboard candidat = 0;
    Bitboard x = (move >> shift) & mask & adv;

    for(int i=0; i<6; i++) {
        if (x == 0) break;
        candidat |= x;
        x = (x >> shift) & mask & adv;
    }

    Bitboard apres_candidat = (candidat >> shift) & mask;
    if (apres_candidat & joueur) {
        flips = candidat;
    }
    return flips;
}

void jouer_coup_bitboard(Plateau *p, int index_0_63, bool tour) {
    Bitboard *joueur; Bitboard *adversaire;
    //On modifie les pointeurs selon quel joueur joue le coup

    //---------------------NOTE: pour opti on pourrait appeler jouer_coup avec parametre bitboard ennemi et moi, et inverser ordre selon tour jeu --------------------
    // j'ai teste mais faut tout negamax c'est horrrrrrible et pas utile, rajoute des if tour ailleurs dans tous les cas
    
    if (tour) {
        joueur = &(p->blanc);
        adversaire = &(p->noir);
    } else {
        joueur = &(p->noir);
        adversaire = &(p->blanc);
    }
    
    Bitboard move = 1ULL << index_0_63;//  Crée un bitboard avec juste le nouveau pion qui veut être placé
    Bitboard captures = 0; // note les pions capturés par le coup dans chaque direction

    //Calcul des pions retournes
    //Lignes droites
    captures |= get_flips_dir(move, *joueur, *adversaire, 1, NOT_A_FILE); // Est (+1)
    captures |= get_flips_dir_inv(move, *joueur, *adversaire, 1, NOT_H_FILE);// Ouest (-1)
    captures |= get_flips_dir(move, *joueur, *adversaire, 8, ~0ULL); // Sud (+8)
    captures |= get_flips_dir_inv(move, *joueur, *adversaire, 8, ~0ULL);// Nord (-8)
    // Diagonales
    captures |= get_flips_dir(move, *joueur, *adversaire, 9, NOT_A_FILE);    // Sud-Est (+9) 
    captures |= get_flips_dir(move, *joueur, *adversaire, 7, NOT_H_FILE);    // Sud-Ouest (+7) 
    captures |= get_flips_dir_inv(move, *joueur, *adversaire, 7, NOT_A_FILE);    // Nord-Est (-7) 
    captures |= get_flips_dir_inv(move, *joueur, *adversaire, 9, NOT_H_FILE);    // Nord-Ouest (-9)

    *joueur |= move | captures; // joue le coup et recupere les pions retournees
    *adversaire &= ~captures; // On les retire aussi a l'ennemi
}

// Renvoie 1 si le coup est légal, 0 sinon
int est_coup_legal(Plateau p, bool tour, int coup) { // ! ne verifie pas que le coup soit bien entre 0 et 63 ! (par souci de performance)
    Bitboard joueur; Bitboard adversaire;
    //On modifie les pointeurs selon quel joueur joue le coup
    if (tour) {
        joueur = (p.blanc);
        adversaire = (p.noir);
    } else {
        joueur = (p.noir);
        adversaire = (p.blanc);
    }
    
    Bitboard mask_coup = 1ULL << coup; // cree le bitboard du jeton à placer: tout à 0 sauf l'indice du coup
    if ((joueur | adversaire) & mask_coup) return 0; // Case déjà occupée, coup invalide

    // test si le coup fait quelques chose dans chaque direction
    else if (get_flips_dir(mask_coup, joueur, adversaire, 1, NOT_A_FILE)) return 1;
    else if (get_flips_dir(mask_coup, joueur, adversaire, 8, ~0ULL)) return 1;   
    else if (get_flips_dir(mask_coup, joueur, adversaire, 9, NOT_A_FILE)) return 1;  
    else if (get_flips_dir(mask_coup, joueur, adversaire, 7, NOT_H_FILE)) return 1; 

    else if (get_flips_dir_inv(mask_coup, joueur, adversaire, 1, NOT_H_FILE)) return 1;
    else if (get_flips_dir_inv(mask_coup, joueur, adversaire, 8, ~0ULL)) return 1;
    else if (get_flips_dir_inv(mask_coup,joueur, adversaire, 9, NOT_H_FILE)) return 1;
    else if (get_flips_dir_inv(mask_coup, joueur, adversaire, 7, NOT_A_FILE)) return 1;
    // Si on arrive ici, aucune direction n'a marché, le coup est illegal
    return 0;
}


// Verion obsolete car trop lente par rapport à la nouvelle
int peut_jouer_bitboard(Plateau p, bool tour){
    for (int c=0;c<64;c++){
        if (est_coup_legal( p,  tour,  c)) return 1;
    }
    return false;
}


// Pour savoir si le joueur 'moi' peut jouer: 
bool peut_jouer_bb(Bitboard moi, Bitboard adv) {
    return coups_possibles(moi, adv) != 0;
}
/*bool fin_de_partie_bitboard(Plateau p) { // détecte la fin du jeu
    return (!peut_jouer_bitboard( p, true) && !peut_jouer_bitboard( p, false));
}*/

bool fin_de_partie_bitboard(Plateau p) { // détecte la fin du jeu
    return (!peut_jouer_bb( p.blanc, p.noir) && !peut_jouer_bb( p.noir, p.blanc));
}



// Cette fonction là ne dépend pas du coup et renvoie un Bitboard de tous les coup legaux dans une direction donnee.
//vide: bitboard avec des 1 sur les cases vides
// is_left_shift : true : << , false : >> , inverse juste la fonction
static inline Bitboard get_moves_in_dir(Bitboard moi, Bitboard adv, Bitboard vide, int shift, Bitboard mask, bool is_left_shift) {
/* Pour trouver les cases ou un joueur peut jouer, on recherche le pattern moi {i*adv} vide
*/    
    
    Bitboard candidats;
    
    //On recupere les pions adv à un déplacement de moi (ceux juste à coté)
    if (is_left_shift) {
        candidats = (moi << shift) & mask & adv;
    }
    else {
        candidats = (moi >> shift) & mask & adv; 
    }
   
    //On avance dans la chaine de pions adverse.
    
    // Deplacement 5 fois car on peut manger au maximum 6 jetons en un coup:
    //  pas utile de faire plus pour un jeu d'othello car: 1er jeton = nous; 2e = premier ennemi dans etape 1
    // 2e-7e: boucle
    // 8e: a la fin
    for (int i = 0; i < 5; i++) { 
        if (is_left_shift) 
            candidats |= (candidats << shift) & mask & adv;
        else 
            candidats |= (candidats >> shift) & mask & adv;
    }
    // A ce stade, candidats contient tous les pions adv qui forment une chaine continue depuis un jeton à moi


    // On verifie que le dernier deplacement du candidat atteint une case vide == qu'on puisse encadrer le pion adverse
    if (is_left_shift) {
        return (candidats << shift) & mask & vide;
    }
    else {
        return (candidats >> shift) & mask & vide;        
    }

}

// Retourne un Bitboard de tous les coups possibles du joueur moi
Bitboard coups_possibles(Bitboard moi, Bitboard adv) 
{
    Bitboard vide = ~(moi | adv);
    Bitboard legaux = 0;

    legaux |= get_moves_in_dir(moi, adv, vide, 1, NOT_H_FILE, false);   
    legaux |= get_moves_in_dir(moi, adv, vide, 8, ~0ULL, false);
    legaux |= get_moves_in_dir(moi, adv, vide, 7, NOT_A_FILE, false);
    legaux |= get_moves_in_dir(moi, adv, vide, 9, NOT_H_FILE, false);
    legaux |= get_moves_in_dir(moi, adv, vide, 1, NOT_A_FILE, true);
    legaux |= get_moves_in_dir(moi, adv, vide, 8, ~0ULL, true);
    legaux |= get_moves_in_dir(moi, adv, vide, 7, NOT_H_FILE, true);
    legaux |= get_moves_in_dir(moi, adv, vide, 9, NOT_A_FILE, true);

    return legaux;
}




// Fin fonctions de gestion du bitboard  --------------------------------------------------------------------


// Fonctions pour la mécanique de jeu

//On definit deux structures afin de jouer un coup et de retenir les cases affectées par ce coup (pour pouvoir faire des retours en arrière)
// definies dans le fonctions_ia.h
bool jouer_coup_avec_historique(int x, int y, bool tour, char plateau[N][N], HistoriqueCoup *hist) { // prend en parametre les coordonnées d'un coup à jouer dans le plateau (x,y)et le joue; 
// sortie:  renvoie true si le coup est valide (a bien retourné une tuile), false sinon. Si le coup est invalide (ne retourne pas de case), on ne place pas le jeton
// + modifie les pointeurs pour enregistrer les etats precedents des jetons retournes

    bool coup_legal=false; // variable de retour

    char our_color = tour ? BLANCS : NOIRS;
    char ennemy_color = tour ? NOIRS : BLANCS;
    if (plateau[x][y]!=VIDE) return false; // Pas le droit de jouer si la case n'est pas vide

    hist->nb_modifs = 0;
    // Joue la case elle-même
    hist->modifs[hist->nb_modifs++] = (Changement){x, y, plateau[x][y]};
    plateau[x][y] = our_color;

    // Parcours les 8 directions
    for (int d = 0; d < 8; d++) {
        int dx = deplacement_x[d];
        int dy = deplacement_y[d];
        int x_suiv = x + dx, y_suiv = y + dy;
        bool ligne_valide = false;

        while (coup_dans_plateau(x_suiv, y_suiv) && plateau[x_suiv][y_suiv] == ennemy_color) { // On parcourre jusqu'a trouver un jeton qui n'est pas a l'ennemi ou si on sort du plateau
            x_suiv += dx; y_suiv += dy;
            ligne_valide = true;
        }

        if (ligne_valide && coup_dans_plateau(x_suiv, y_suiv) && plateau[x_suiv][y_suiv] == our_color) { // verifie que le jeton à l'extrémité est à nous
            // Revenir en arrière pour retourner les pions -> ici pas un retour en arrière mais on reparcouure le plateau pour jusqu'au jeton final a retourner
            int rx = x + dx, ry = y + dy;
            while (rx != x_suiv || ry != y_suiv) {
                hist->modifs[hist->nb_modifs++] = (Changement){rx, ry, plateau[rx][ry]};
                plateau[rx][ry] = our_color; coup_legal=true; // à ce stade un jeton a été retourné donc le coup fait bien qq chose
                rx += dx;
                ry += dy;
            }
        }
    }

    if (!coup_legal) plateau[x][y]=VIDE;
    return coup_legal;
}

void annuler_coup(HistoriqueCoup *hist, char plateau[N][N]) {
    for (int i = 0; i < hist->nb_modifs; i++) {
        plateau[hist->modifs[i].x][hist->modifs[i].y] = hist->modifs[i].old_value;
    }
}

bool jouer_coup(int x, int y,bool tour,char plateau[N][N]) // prend en parametre les coordonnées d'un coup valide à jouer dans le plateau (x,y)et le joue; 
// sortie:  renvoie true si le coup est valide (a bien retourné une tuile), false sinon. Si le coup est invalide (ne retourne pas de case), on ne place pas le jeton
{
   if (plateau[x][y]!=VIDE) return false; // Pas le droit de jouer si la case n'est pas vide

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



bool action_effectuee(int x, int y,bool tour,char plateau[N][N]){ // dis si le coup retourne au noirns une tuile
    bool coup_valide=false;
    char our_color, ennemy_color;
   if (tour) {our_color=BLANCS; ennemy_color=NOIRS;}
   else {our_color=NOIRS; ennemy_color=BLANCS;}

   int i=0;
    while ((!coup_valide) && (i<N) ){ // condition d'arret: on a trouve que le coup est valide ou fait tous les déplacements possibles
        int x_suiv=x+deplacement_x[i] , y_suiv=y+deplacement_y[i];

        while( coup_dans_plateau(x_suiv,y_suiv) && plateau[x_suiv][y_suiv]==ennemy_color && (!coup_valide) ){
            x_suiv= x_suiv + deplacement_x[i] ; y_suiv= y_suiv + deplacement_y[i]; //on regarde la case suivante

            if ( coup_dans_plateau(x_suiv,y_suiv) && plateau[x_suiv][y_suiv] == our_color ) { // à ce stade on est allé à 2 cases minimum de la case intitiale, donc si on trouve un jeton à nous, on peut faire un retournement
                coup_valide=true; // si on arrive ici, le coup est légal car il peut retourner des tuiles
            }
        }
 
    i++; // on regarde le prochain deplacement
    }
return coup_valide;
}

bool coup_valide(int x, int y, bool tour, char plateau[N][N]){ // vérifie qu'un coup respecte toutes les règles du jeu (le coup peut être accepté par lire_coup())
	if ( (coup_dans_plateau(x,y) ) && (plateau[x][y]==VIDE) && (action_effectuee(x, y,tour, plateau))  ) return true;
return false;
}

 
bool peut_jouer(bool tour, char plateau[N][N]) { // dit a partir d'un plateau si le joueur tour peut jouer, autrement il devra passer son tour
    for (int i=0;i<N;i++) {
        for (int j=0;j<N;j++) {
            if (plateau[i][j] == VIDE && action_effectuee(i, j,tour, plateau)) {
                return true; // dès qu'on trouve un coup valide, le joueur tour peut jouer
            }
        }
    }
    return false;
}

bool fin_de_partie(char plateau[N][N]) { // détecte la fin du jeu
    return (!peut_jouer(true,plateau) && !peut_jouer(false,plateau));
}

void score_joueur(int *score_blanc, int *score_noir, char plateau[N][N]) { //compte les pions de chaque joueur
    *score_blanc = 0;
    *score_noir = 0;

    for (int i=0;i<N; i++) {
        for (int j = 0;j<N;j++) {
            if (plateau[i][j] == BLANCS) { (*score_blanc)++; }
            else if (plateau[i][j] == NOIRS) { (*score_noir)++;}
        }
    }
}




// Fonctions pour l'IA / la recherche du meilleur coup à jouer -----------------------------------------------------------

//#define max(a, b) ((a) > (b) ? (a) : (b))
//#define min(a, b) ((a) < (b) ? (a) : (b))
// apparement c'est plus safe les static inline (par rapport aux DEF) et plus rapide qu'une fonction simple
static inline int max(int a, int b) { return (a > b) ? a : b;}
static inline int min(int a, int b) { return (a < b) ? a : b;}


int evaluation_ia(int gene[7],char plateau[N][N]) { // Version pour déterminer les valeurs des constantes
// L'évaluation est symétrique, 0-> position égale
    int score_pondere_blanc=0, score_pondere_noir=0 ; // le score selon le placement des jetons dans l'othellier
    int nb_coup_legal_blanc=0, nb_coup_legal_noir=0; // nom de variable assez explicite
    int nb_jeton_blanc=0,nb_jeton_noir=0;
    int nb_cases_vides=0;
    static const int poids[N][N] = { // tableau pour donner plus ou noirns de valeur aux cases, static pour garder quand evaluation sera re-appelee
       {100, -20, 10,  5,  5, 10, -20, 100},
       {-20, -50, -2, -2, -2, -2, -50, -20},
       {10,  -2,   0,  0,  0,  0,  -2,  10},
       { 5,  -2,   0,  0,  0,  0,  -2,   5},
       { 5,  -2,   0,  0,  0,  0,  -2,   5},
       {10,  -2,   0,  0,  0,  0,  -2,  10},
       {-20,-50,  -2, -2, -2, -2, -50, -20},
       {100, -20, 10,  5,  5, 10, -20, 100}
    };

    for (int i=0;i<N;i++) {
        for (int j=0;j<N;j++) {
            if (plateau[i][j]==VIDE){
                nb_cases_vides++;
                if (action_effectuee(i, j,true, plateau)) { // si blanc peut jouer à cet emplacement
                nb_coup_legal_blanc ++;
                }
                if (action_effectuee(i, j,false, plateau)) {// si les noirs peuvent jouer à cet emplacement
                nb_coup_legal_noir ++;
                }
            }
            else if(plateau[i][j]==BLANCS){
                nb_jeton_blanc++; score_pondere_blanc+=poids[i][j];
            }
            else if(plateau[i][j]==NOIRS){
                nb_jeton_noir++; score_pondere_noir+=poids[i][j];

            }
        }
    }
    if (nb_jeton_blanc==0) return -(INF-1); //Blanc a gagne
    if (nb_jeton_noir==0) return (INF-1); // Noir a perdu
    if ((nb_cases_vides==0) ){ // Si fin de partie-> pas vraiment mais la fonction fin de partie est trop lente
        int score_final=nb_jeton_blanc-nb_jeton_noir;
        if (score_final!=0) return ((score_final)/abs(score_final))*(INF-1); // fin de partie donc cette evaluation doit être la plus importante 
            // attention ! il faut que cette valeur soit inferieure à celle de la constante dans minimax (car sinon l'évaluation est trop grande/petite et n'est pas
            //prise en compte par minimax qui considère que le coup est nul)
        else return 0; // egalite 
    }
    // Coefficients de pondération, à ajuster selon test IA, voir quels paramètres sont les mieux
    int w1 = gene[0];  // mobilité (nb de coup legaux)
    int w2 = gene[1];   // nombre pions
    int w3 = gene[2];  // position poids

    // ajustement selon phase du jeu
    if (nb_cases_vides < gene[6]) { // vers la fin de partie
        w1=gene[3];
        w2 = gene[4]; // a la fin le nombre de pions devient important, mais tout peut encore changer
        w3 = gene[5]; // noirns important à la fin la position des jetons
    }
    int score = w1*(nb_coup_legal_blanc - nb_coup_legal_noir) + w2*(nb_jeton_blanc - nb_jeton_noir)+ w3*(score_pondere_blanc - score_pondere_noir);
    return score;
}


int evaluation2(char plateau[N][N]) { // renvoie une évaluation de la position; si positif les blancs sont mieux, si négatif les noirs sont mieux. 
// L'évaluation est symétrique, 0-> position égale
    int score_pondere_blanc=0, score_pondere_noir=0 ; // le score selon le placement des jetons dans l'othellier
    int nb_coup_legal_blanc=0, nb_coup_legal_noir=0; // nom de variable assez explicite
    int nb_jeton_blanc=0,nb_jeton_noir=0;
    int nb_cases_vides=0;
    static const int poids[N][N] = { // tableau pour donner plus ou noirns de valeur aux cases, static pour garder quand evaluation sera re-appelee
       {100, -20, 10,  5,  5, 10, -20, 100},
       {-20, -50, -2, -2, -2, -2, -50, -20},
       {10,  -2,   0,  0,  0,  0,  -2,  10},
       { 5,  -2,   0,  0,  0,  0,  -2,   5},
       { 5,  -2,   0,  0,  0,  0,  -2,   5},
       {10,  -2,   0,  0,  0,  0,  -2,  10},
       {-20,-50,  -2, -2, -2, -2, -50, -20},
       {100, -20, 10,  5,  5, 10, -20, 100}
    };

    for (int i=0;i<N;i++) {
        for (int j=0;j<N;j++) {
            if (plateau[i][j]==VIDE){
                nb_cases_vides++;
                if (action_effectuee(i, j,true, plateau)) { // si blanc peut jouer à cet emplacement
                nb_coup_legal_blanc ++;
                }
                if (action_effectuee(i, j,false, plateau)) {// si les noirs peuvent jouer à cet emplacement
                nb_coup_legal_noir ++;
                }
            }
            else if(plateau[i][j]==BLANCS){
                nb_jeton_blanc++; score_pondere_blanc+=poids[i][j];
            }
            else if(plateau[i][j]==NOIRS){
                nb_jeton_noir++; score_pondere_noir+=poids[i][j];

            }
        }
    }
    if (nb_jeton_blanc==0) return -(INF-1); //Blanc a gagne
    if (nb_jeton_noir==0) return (INF-1); // Noir a perdu
    if ((nb_cases_vides==0) ){ // Si fin de partie-> pas vraiment mais la fonction fin de partie est trop lente
        int score_final=nb_jeton_blanc-nb_jeton_noir;
        if (score_final!=0) return ((score_final)/abs(score_final))*(INF-1); // fin de partie donc cette evaluation doit être la plus importante 
            // attention ! il faut que cette valeur soit inferieure à celle de la constante dans minimax (car sinon l'évaluation est trop grande/petite et n'est pas
            //prise en compte par minimax qui considère que le coup est nul)
        else return 0; // egalite 
    }
    // Coefficients de pondération, à ajuster selon test IA, voir quels paramètres sont les mieux
    int w1 = 10;  // mobilité (nb de coup legaux)
    int w2 = 1;   // nombre pions
    int w3 = 30;  // position poids

    // ajustement selon phase du jeu
    if (nb_cases_vides < 15) { // vers la fin de partie
        w1=20;
        w2 = 25; // a la fin le nombre de pions devient important, mais tout peut encore changer
        w3 = 10; // noirns important à la fin la position des jetons
    }
    //printf("normal:%d %d %d %d %d %d \n",nb_coup_legal_blanc,nb_coup_legal_noir,nb_jeton_blanc,nb_jeton_noir,score_pondere_blanc,score_pondere_noir);
    int score = w1*(nb_coup_legal_blanc - nb_coup_legal_noir) + w2*(nb_jeton_blanc - nb_jeton_noir)+ w3*(score_pondere_blanc - score_pondere_noir);
    return score;
}

// Fonction d'évaluation pour l'IA
int evaluation_bitboard(Plateau p, int gene[7]) {
    int score_pondere_blanc=0, score_pondere_noir=0 ; // le score selon le placement des jetons dans l'othellier
    int nb_coup_legal_blanc=0, nb_coup_legal_noir=0; // nom de variable assez explicite
    int nb_jeton_blanc= 0;//__builtin_popcountll(p.blanc);
    int nb_jeton_noir =0;// __builtin_popcountll(p.noir);
    

    // Evaluation de la mobilite:
/*
    Le jeu comporte de part ses symétries 10 cases differentes. On affecte une valeur à chacune:
     
        c9 c8 c6 c3 c3 c6 c8 c9
        c8 c7 c5 c2 c2 c5 c7 c8
        c6 c5 c4 c1 c1 c4 c5 c6
        c3 c2 c1 c0 c0 c1 c2 c3
        c3 c2 c1 c0 c0 c1 c2 c3
        c6 c5 c4 c1 c1 c4 c5 c6
        c8 c7 c5 c2 c2 c5 c7 c8
        c9 c8 c6 c3 c3 c6 c8 c9
     
     */
    //Masques pour chaque type de case

    static Bitboard c0_BB = 0b0000000000000000000000000001100000011000000000000000000000000000ULL;
    static Bitboard c1_BB = 0b0000000000000000000110000010010000100100000110000000000000000000ULL;
    static Bitboard c2_BB = 0b0000000000011000000000000100001001000010000000000001100000000000ULL;
    static Bitboard c3_BB = 0b0001100000000000000000001000000110000001000000000000000000011000ULL;
    static Bitboard c4_BB = 0b0000000000000000001001000000000000000000001001000000000000000000ULL;
    static Bitboard c5_BB = 0b0000000000100100010000100000000000000000010000100010010000000000ULL;
    static Bitboard c6_BB = 0b0010010000000000100000010000000000000000100000010000000000100100ULL;
    static Bitboard c7_BB = 0b0000000001000010000000000000000000000000000000000100001000000000ULL;
    static Bitboard c8_BB = 0b0100001010000001000000000000000000000000000000001000000101000010ULL;
    static Bitboard c9_BB = 0b1000000100000000000000000000000000000000000000000000000010000001ULL;


    //Poids de chaque case          -------- A determiner/ modifier par l'IA -------------

    // NOTE OPTIMISATION FUTURE !!!!
    // Une fois determines penser à supprimer ceux qui valent 0 du calcul (sert a rien de compter si coeff=0)
    static int c0_poids = 0;
    static int c1_poids = 4;//0;
    static int c2_poids = -3;//-2;
    static int c3_poids = 6;//5;
    static int c4_poids = 7;//0;
    static int c5_poids =-4;// -2;
    static int c6_poids = 8;//10;
    static int c7_poids = -24;//-50;
    static int c8_poids =-8;// -20;
    static int c9_poids = 99;//100;

    Bitboard masques_cases[10]={c0_BB,c1_BB,c2_BB,c3_BB,c4_BB,c5_BB,c6_BB,c7_BB,c8_BB,c9_BB};
    int poids_cases[10]= {c0_poids, c1_poids ,c2_poids, c3_poids, c4_poids ,c5_poids,c6_poids, c7_poids ,c8_poids, c9_poids};

    int compteB; int compteN;
    for (int u=0; u<10; u++){ //On somme le tout pour obtenir le score pondere de chaque joueur
        compteB= __builtin_popcountll(p.blanc & masques_cases[u]); compteN=__builtin_popcountll(p.noir & masques_cases[u]);
        
        score_pondere_blanc += compteB *poids_cases[u]; nb_jeton_blanc += compteB;
        score_pondere_noir += compteN* poids_cases[u]; nb_jeton_noir += compteN;
    }
    
    if (nb_jeton_blanc==0) return -(INF-1); //Blanc a gagne
    else if (nb_jeton_noir==0) return (INF-1); // Noir a perdu
    int nb_cases_vides=64 - (nb_jeton_blanc + nb_jeton_noir);
    if ((nb_cases_vides==0) ){ // Si fin de partie-> pas vraiment mais la fonction fin de partie est trop lente
        int score_final=nb_jeton_blanc-nb_jeton_noir;
        if (score_final!=0) return ((score_final)/abs(score_final))*(INF-1); // fin de partie donc cette evaluation doit être la plus importante 
            // attention ! il faut que cette valeur soit inferieure à celle de la constante dans minimax (car sinon l'évaluation est trop grande/petite et n'est pas
            //prise en compte par minimax qui considère que le coup est nul)
        else return 0; // egalite 
    }
    // Compte le nombre de coups legaux 
    // NOTE: Approximer car couteux en temps
    nb_coup_legal_blanc=__builtin_popcountll(coups_possibles(p.blanc,p.noir));
    nb_coup_legal_noir=__builtin_popcountll(coups_possibles(p.noir,p.blanc));
    
 // /*
    if ((nb_coup_legal_blanc==0) && (nb_coup_legal_noir==0)){ // Si personne peut jouer = fin de partie
        int score_final=nb_jeton_blanc-nb_jeton_noir;
        if (score_final!=0) return ((score_final)/abs(score_final))*(INF-1); // fin de partie donc cette evaluation doit être la plus importante 
            // attention ! il faut que cette valeur soit inferieure à celle de la constante dans minimax (car sinon l'évaluation est trop grande/petite et n'est pas
            //prise en compte par minimax qui considère que le coup est nul)
        else return 0; // egalite 
    }
//*/

    /*
    // Coefficients de pondération, à ajuster selon test IA, voir quels paramètres sont les mieux
    int w1 = gene[0];  // mobilité (nb de coup legaux)
    int w2 = gene[1];   // nombre pions
    int w3 = gene[2];  // position poids

    // ajustement selon phase du jeu
    if (nb_cases_vides < 20) {
    // if (nb_cases_vides < gene[6]) { // vers la fin de partie
        w1 = gene[3];
        w2 = gene[4]; // a la fin le nombre de pions devient important, mais tout peut encore changer
        w3 = gene[5]; // moins important à la fin la position des jetons
    }
    int res= w1*(nb_coup_legal_blanc - nb_coup_legal_noir) + w2*(nb_jeton_blanc - nb_jeton_noir)+ w3*(score_pondere_blanc - score_pondere_noir);
    */
    /*Opti avec fonction lineaire: f1 debut de partie; f2 fin de partie
    NOTE: implicite float vers int, faut aussi majorer tout ça jsp
    */
    int f1 = gene[0]*(nb_coup_legal_blanc - nb_coup_legal_noir) + gene[1]*(nb_jeton_blanc - nb_jeton_noir)+ gene[2]*(score_pondere_blanc - score_pondere_noir);
    int f2 = gene[3]*(nb_coup_legal_blanc - nb_coup_legal_noir) + gene[4]*(nb_jeton_blanc - nb_jeton_noir)+ gene[5]*(score_pondere_blanc - score_pondere_noir);
    int res2=  (f1*nb_cases_vides + f2 *(60- nb_cases_vides ) )/60 ;

    return res2;
}

int evaluation_bitboard_finale(Plateau plateau) { // Quand la partie est finie, le score est sur et on le calcule plus vite avec ça:

    int score_final=__builtin_popcountll(plateau.blanc)-__builtin_popcountll(plateau.noir);
    if (score_final!=0) return ((score_final)/abs(score_final))*(INF-1);
    else return 0; 

}


// -fonction d'evaluation d'une position donnee
int minimax(int depth,int *best_x,int *best_y,int alpha, int betha, bool tour, char plateau[N][N]) // evalue une postion d'othello a une profondeur depth donnee et avec la connaissance de qui peut jouer
{ // retourne l'evaluation de la position (int) et modifie les variables best_x et best_y pour y noter le meilleur coup trouvé
   
   
    if ((depth == 0) || fin_de_partie(plateau) )return evaluation2(plateau); // pas efficace car pour la fonction fin de partie, on reparcoure le plateau, ça prend du temps
    
   int meilleur_eval; int score;
   int meilleur_x=-1,meilleur_y=-1;// meilleur coup trouvé jusque là
   HistoriqueCoup hist; // Va stocker/retenire le coup joué

   if (tour) { // on évalue pour le jouer blanc (~=MAX), il veut maximiser son score
    meilleur_eval = -INF; // la pire eval possible pour les blancs
    for (int x=0;x<N;x++){ // pour chaque case, regarder si un coup est jouable
        for (int y=0; y<N;y++){
            if ( jouer_coup_avec_historique(x, y, tour, plateau, &hist) ){ //si le coup a fait qq chose, le coup sera joue
                nb_coup_analyses++;
                
                score = minimax(depth - 1, NULL, NULL,alpha,betha, !tour, plateau);
                annuler_coup(&hist, plateau);
                if (score > meilleur_eval) {
                    meilleur_eval = score;
                    meilleur_x =x ; meilleur_y= y;
                }
                alpha=max(alpha,score);
                if (betha<=alpha) break;  // le noeud est condamne
            }
            } if (betha<=alpha) break;// le noeud est condamne
    }
    }else { //type MIN = NOIRS    
    meilleur_eval = INF; // la pire eval possible pour les noirs
    for (int x=0;x<N;x++){ // pour chaque case, regarder si un coup est jouable
        for (int y=0; y<N;y++){
            if ( jouer_coup_avec_historique(x, y, tour, plateau, &hist) ){ //si le coup a fait qq chose, le coup sera joue
                nb_coup_analyses++;
                
                score = minimax(depth - 1, NULL, NULL,alpha,betha, !tour, plateau);
                annuler_coup(&hist, plateau);
                if (score < meilleur_eval) {
                    meilleur_eval = score;
                    meilleur_x =x ; meilleur_y= y;
                }
                betha=min(betha,score);
                if (betha<=alpha) break;  // le noeud est condamne
            }
            
            } if (betha<=alpha) break;// le noeud est condamne
    }
}

if (  (meilleur_x!=-1) && (meilleur_y!=-1)){ // on vérifie aussi que l'on a trouve un coup valide (on a modifié la valeur de meilleur_x et meilleur_y)
    if  ( (best_x != NULL) && (best_y!=NULL ) ) {  // si c'est l'appel de base de minimax (racine), on modifie le meilleur coup
        
        *best_x=meilleur_x;
        *best_y=meilleur_y;
        }
}  else{ // aucun coup n'a ete trouve pour le joueur tour
    return minimax(depth -1, NULL,NULL,alpha,betha,!tour,plateau);

}
   return meilleur_eval ;
}

int minimax_ia(int depth,int *best_x,int *best_y,int alpha, int betha, bool tour, int gene[7],char plateau[N][N])//version avec parametres pas definis
{
    if ((depth == 0) || fin_de_partie(plateau) )return evaluation_ia(gene,plateau); // pas efficace car pour la fonction fin de partie, on reparcoure le plateau, ça prend du temps
    
   int meilleur_eval; int score;
   int meilleur_x=-1,meilleur_y=-1;// meilleur coup trouvé jusque là
   HistoriqueCoup hist; // Va stocker/retenire le coup joué

   if (tour) { // on évalue pour le jouer blanc (~=MAX), il veut maximiser son score
    meilleur_eval = -INF; // la pire eval possible pour les blancs
    for (int x=0;x<N;x++){ // pour chaque case, regarder si un coup est jouable
        for (int y=0; y<N;y++){
            
            if ( jouer_coup_avec_historique(x, y, tour, plateau, &hist) ){ //si le coup a fait qq chose, le coup sera joue
                nb_coup_analyses++;
                
                score = minimax_ia(depth - 1, NULL, NULL,alpha,betha, !tour,gene, plateau);
                annuler_coup(&hist, plateau);
                if (score > meilleur_eval) {
                    meilleur_eval = score;
                    meilleur_x =x ; meilleur_y= y;
                }
                alpha=max(alpha,score);
                if (betha<=alpha) break;  // le noeud est condamne
            }
            } if (betha<=alpha) break;// le noeud est condamne
    }
    }else { //type MIN = NOIRS    
    meilleur_eval = INF; // la pire eval possible pour les noirs
    for (int x=0;x<N;x++){ // pour chaque case, regarder si un coup est jouable
        for (int y=0; y<N;y++){
//            HistoriqueCoup hist; // ! lié au if en dessous
            if ( jouer_coup_avec_historique(x, y, tour, plateau, &hist) ){ //si le coup a fait qq chose, le coup sera joue
                nb_coup_analyses++;
                
                score = minimax_ia(depth - 1, NULL, NULL,alpha,betha, !tour, gene,plateau);
                annuler_coup(&hist, plateau);
                if (score < meilleur_eval) {
                    meilleur_eval = score;
                    meilleur_x =x ; meilleur_y= y;
                }
                betha=min(betha,score);
                if (betha<=alpha) break;  // le noeud est condamne
            }
            
            } if (betha<=alpha) break;// le noeud est condamne
    }
}

if (  (meilleur_x!=-1) && (meilleur_y!=-1)){ // on vérifie aussi que l'on a trouve un coup valide (on a modifié la valeur de meilleur_x et meilleur_y)
    if  ( (best_x != NULL) && (best_y!=NULL ) ) {  // si c'est l'appel de base de minimax (racine), on modifie le meilleur coup
        
        *best_x=meilleur_x;
        *best_y=meilleur_y;
        }
}  else{ // aucun coup n'a ete trouve pour le joueur tour
    return minimax_ia(depth -1, NULL,NULL,alpha,betha,!tour,gene,plateau);

}
   return meilleur_eval ;
}


unsigned long long nb_noeuds_vus=0;
const Bitboard c8_c9_BB = 0b1011110100111100111111111111111111111111111111110011110010111101ULL;

/*
// version move_ordering qui ne fait rien de bien...
int minimax_bitboard_ia(int depth,int *best_move,int alpha, int betha, bool tour, int gene[7],Plateau plateau)
{
    nb_noeuds_vus++;
    if (depth == 0)return evaluation_bitboard(plateau,gene); // pas efficace car pour la fonction fin de partie, on reparcoure le plateau, ça prend du temps
   // NOTE: on copie le tableau gene a chaque appel de minimax, on pourrait optimiser et l'utiliser qu'a la fin (à l'appel profondeur 0, var globale gene une 
   //fois que le meileur gene est determine)
   
   int meilleur_eval; int score;
   int meilleur_coup=64;// meilleur coup trouvé jusque là

   if (tour) { // on évalue pour le jouer blanc (~=MAX), il veut maximiser son score
    meilleur_eval = -INF; // la pire eval possible pour les blancs
    
    Bitboard coups_legaux_joueur= coups_possibles( plateau.blanc , plateau.noir); // creation du bitboard des cases possibles
    
    if(coups_legaux_joueur ==0){ // la profondeur n'est pas 0 mais on ne peux pas jouer
        Bitboard coups_legaux_adversaire= coups_possibles(plateau.noir,plateau.blanc);
        
        if(coups_legaux_adversaire ==0){ //GAME OVER
            return evaluation_bitboard(plateau,gene); // note: fonction evaluation sait detecter la fin de partie donc le score sera fiable
        }
        else{
            score= minimax_bitboard_ia(depth-1,NULL,alpha,betha,!tour,gene,plateau); // NOTE depth doit aussi marcher car on a rien calculé ----------------------------
            if (best_move != NULL) *best_move=64; // On passe notre tour si appel de base minimax
            return score;
        }
    }
    else{// On a passé le IF, on a donc des coups legaux
      
        Bitboard coups_prioriataires = coups_legaux_joueur & (c8_c9_BB); //Les coups a regarder en priorité
        Bitboard others = coups_legaux_joueur & ~c8_c9_BB;
        Bitboard liste[]={coups_prioriataires,others};
        for (int u=0; u<2; u++){
            Bitboard a_tester= liste[u];
            while (a_tester){ // tant qu'il reste des coups à tester
                int index = __builtin_ctzll(a_tester); // Donne l'index du premier bit à 1 == le premier coup dispo à tester
                a_tester= a_tester & (a_tester-1) ;// suppression du coup qu'on vient de regarder

                Plateau copie_plateau = plateau;
                jouer_coup_bitboard(&copie_plateau,index,tour);// Joue sur la copie
                score = minimax_bitboard_ia(depth - 1, NULL,alpha,betha, !tour,gene, copie_plateau);
                
                if (score > meilleur_eval) {
                    meilleur_eval = score;
                    meilleur_coup =index ;
                }
                if (score >alpha) alpha = score;
                if (betha<=alpha) break;  // le noeud est condamne
            }        
        }

    }
    }
    else { //type MIN = NOIRS    
    meilleur_eval = INF; // la pire eval possible pour les noirs
    
    
    Bitboard coups_legaux_joueur= coups_possibles( plateau.noir , plateau.blanc); // creation du bitboard des cases possibles
    
    if(coups_legaux_joueur ==0){ // la profondeur n'est pas 0 mais on ne peux jouer
        Bitboard coups_legaux_adversaire= coups_possibles(plateau.blanc,plateau.noir);
        if(coups_legaux_adversaire ==0){
            return evaluation_bitboard(plateau,gene); //GAME OVER
        }
        else{
            score= minimax_bitboard_ia(depth-1,NULL,alpha,betha,!tour,gene,plateau);
            if (best_move != NULL) *best_move=64; // On passe notre tour si appel de base minimax
            return score;
        }
    }
    else{// On a passé le IF, on a donc des coups legaux

        Bitboard coups_prioriataires = coups_legaux_joueur & (c8_c9_BB); //Les coups a regarder en priorité
        Bitboard others = coups_legaux_joueur & ~c8_c9_BB;
        Bitboard liste[]={coups_prioriataires,others};
        for (int u=0; u<2; u++){
            Bitboard a_tester= liste[u];
            while (a_tester){ // tant qu'il reste des coups à tester
                int index = __builtin_ctzll(a_tester); // Donne l'index du premier bit à 1 == le premier coup dispo à tester
                a_tester= a_tester & (a_tester-1) ;// suppression du coup qu'on vient de regarder

                Plateau copie_plateau = plateau;
                jouer_coup_bitboard(&copie_plateau,index,tour);// Joue sur la copie
                score = minimax_bitboard_ia(depth - 1, NULL,alpha,betha, !tour,gene, copie_plateau);
            
            if (score < meilleur_eval) {
                meilleur_eval = score;
                meilleur_coup =index ;
            }
            if (score<betha) betha=score;
            if (betha<=alpha) break;  // le noeud est condamne
        }    }
    
    }
    }
if (best_move != NULL){  // si c'est l'appel de base de minimax (racine), on modifie le meilleur coup
    *best_move=meilleur_coup;
    }

   return meilleur_eval ; // Renvoie le score supposé après tout les appels de minimax, c'est la racine et l'evaluation
}
*/

//------------------------------
///*
int minimax_bitboard_ia(int depth,int *best_move,int alpha, int betha, bool tour, int gene[7],Plateau plateau)
{
    nb_noeuds_vus++;
    if (depth == 0)return evaluation_bitboard(plateau,gene); // pas efficace car pour la fonction fin de partie, on reparcoure le plateau, ça prend du temps
   // NOTE: on copie le tableau gene a chaque appel de minimax, on pourrait optimiser et l'utiliser qu'a la fin (à l'appel profondeur 0, var globale gene une 
   //fois que le meileur gene est determine)
   
   int meilleur_eval; int score;
   int meilleur_coup=64;// meilleur coup trouvé jusque là

   if (tour) { // on évalue pour le jouer blanc (~=MAX), il veut maximiser son score
    meilleur_eval = -INF; // la pire eval possible pour les blancs
    
    Bitboard coups_legaux_joueur= coups_possibles( plateau.blanc , plateau.noir); // creation du bitboard des cases possibles
    
    if(coups_legaux_joueur ==0){ // la profondeur n'est pas 0 mais on ne peux pas jouer
        Bitboard coups_legaux_adversaire= coups_possibles(plateau.noir,plateau.blanc);
        
        if(coups_legaux_adversaire ==0){ //GAME OVER
            return evaluation_bitboard_finale(plateau);
            //return evaluation_bitboard(plateau,gene); // note: fonction evaluation sait detecter la fin de partie donc le score sera fiable
        }
        else{
            score= minimax_bitboard_ia(depth-1,NULL,alpha,betha,!tour,gene,plateau); // NOTE depth doit aussi marcher car on a rien calculé ----------------------------
            if (best_move != NULL) *best_move=64; // On passe notre tour si appel de base minimax
            return score;
        }
    }
    else{// On a passé le IF, on a donc des coups legaux
        while (coups_legaux_joueur){ // tant qu'il reste des coups à tester
            int index = __builtin_ctzll(coups_legaux_joueur); // Donne l'index du premier bit à 1 == le premier coup dispo à tester
            coups_legaux_joueur= coups_legaux_joueur & (coups_legaux_joueur-1) ;// suppression du coup qu'on vient de regarder

            Plateau copie_plateau = plateau;
            jouer_coup_bitboard(&copie_plateau,index,tour);// Joue sur la copie
            score = minimax_bitboard_ia(depth - 1, NULL,alpha,betha, !tour,gene, copie_plateau);
            
            if (score > meilleur_eval) {
                meilleur_eval = score;
                meilleur_coup =index ;
            }
            if (score >alpha) alpha = score;
            if (betha<=alpha) break;  // le noeud est condamne
        }

        }
    }
    else { //type MIN = NOIRS    
    meilleur_eval = INF; // la pire eval possible pour les noirs
    
    Bitboard coups_legaux_joueur= coups_possibles( plateau.noir , plateau.blanc); // creation du bitboard des cases possibles
    
    if(coups_legaux_joueur ==0){ // la profondeur n'est pas 0 mais on ne peux jouer
        Bitboard coups_legaux_adversaire= coups_possibles(plateau.blanc,plateau.noir);
        if(coups_legaux_adversaire ==0){
            return evaluation_bitboard_finale(plateau);
            //return evaluation_bitboard(plateau,gene); //GAME OVER
        }
        else{
            score= minimax_bitboard_ia(depth-1,NULL,alpha,betha,!tour,gene,plateau);
            if (best_move != NULL) *best_move=64; // On passe notre tour si appel de base minimax
            return score;
        }
    }
    else{// On a passé le IF, on a donc des coups legaux
        while (coups_legaux_joueur){ // tant qu'il reste des coups à tester
            int index = __builtin_ctzll(coups_legaux_joueur); // Donne l'index du premier bit à 1 == le premier coup dispo à tester
            coups_legaux_joueur= coups_legaux_joueur & (coups_legaux_joueur-1) ;// suppression du coup qu'on vient de regarder

            Plateau copie_plateau = plateau; //copie du plateau de jeu -> a voir si une copie de 2 bitboard marche bien
            jouer_coup_bitboard(&copie_plateau,index,tour);// Joue sur la copie
            score = minimax_bitboard_ia(depth - 1, NULL,alpha,betha, !tour,gene, copie_plateau);
            
            if (score < meilleur_eval) {
                meilleur_eval = score;
                meilleur_coup =index ;
            }
            if (score<betha) betha=score;
            if (betha<=alpha) break;  // le noeud est condamne
        }    
    
    }
    }
if (best_move != NULL){  // si c'est l'appel de base de minimax (racine), on modifie le meilleur coup
    *best_move=meilleur_coup;
    }

   return meilleur_eval ; // Renvoie le score supposé après tout les appels de minimax, c'est la racine et l'evaluation
}
//-------------
//*/





void ordi_random(int *x, int *y, bool tour,char plateau[N][N])// Modifie x,y  avec un coup aleatoire valide
{
    int liste_coups_valides_x[64]; // stocke l'indice des coups_valdides
    int liste_coups_valides_y[64];
    int nb_coups_valides=0; //sera aussi le nb d'elements du tableau
    for (int i=0; i<N; i++){
        for (int j=0;j<N; j++){
            if (coup_valide( i, j,tour,plateau)){
                liste_coups_valides_x[nb_coups_valides]=i; liste_coups_valides_y[nb_coups_valides]=j;
                nb_coups_valides++;
            }

        }
    }
    if (nb_coups_valides != 0){ // verifie qu'on a bien un coup valide
    int indice_random= rand()%nb_coups_valides; 
   // printf("random: %d nb coups:%d\n",indice_random,nb_coups_valides );
   // Note: pas un vrai aleatoire mais ça suffira
    *x= liste_coups_valides_x[indice_random]; *y= liste_coups_valides_y[indice_random];
    //jouer_coup(liste_coups_valides_x[indice_random] , liste_coups_valides_y[indice_random],tour, plateau);
    }
}

void ordi_random_bitboard(int *p, bool tour,Plateau plateau)// Modifie p  avec un coup aleatoire valide
{
    // NOTE opti: demander coup legaux et regarder chaque coup legal au pif ?-> on aura instant le nb de coups et on fait l'algo random nb de fois
    int liste_coups_valides[64]; // stocke l'indice des coups_valdides
    int nb_coups_valides=0; //sera aussi le nb d'elements du tableau
    for (int coup=0; coup<64; coup++){
            if (est_coup_legal( plateau, tour, coup)){
                liste_coups_valides[nb_coups_valides]=coup;
                nb_coups_valides++;
            }
    }
    if (nb_coups_valides != 0){ // verifie qu'on a bien un coup valide
    int indice_random= rand()%nb_coups_valides; 
    //printf("random: %d nb coups:%d\n",indice_random,nb_coups_valides );
   // Note: pas un vrai aleatoire mais ça suffira
    *p= liste_coups_valides[indice_random];
    }
}

void ordi_greedy_bitboard(int *p, bool tour,Plateau plateau)// Modifie p  avec un coup qui maximise le nombre de jetons du joueur tour
{
    int differentiel_score;
    int meilleur_differentiel=-99; // on veut maximiser ça
    int meilleur_coup_greedy=64;

    int facteur; // pour savoir si on veut un score blanc ou noir
    if (tour) facteur=1;
    else facteur =-1;

    Plateau copie_plateau;
    for (int coup=0; coup<64; coup++){
            if (est_coup_legal( plateau, tour, coup)){
                copie_plateau= plateau;
                jouer_coup_bitboard(&copie_plateau,coup,tour);
                differentiel_score= ( __builtin_popcountll(copie_plateau.blanc) - __builtin_popcountll(copie_plateau.noir))*facteur ;
                if (differentiel_score> meilleur_differentiel){
                    meilleur_differentiel= differentiel_score;
                    meilleur_coup_greedy= coup;
                }
            }
    }
    *p= meilleur_coup_greedy;
    
}



// Fonctions d'interface utilisateur -----------------------------------------------------------

void lire_coup_ia(int *x, int *y) // lis un coup entré au clavier et le stocke avec le bon format pour le tableau dans les pointeurs x,y
{ 
	char colonne;
	int ligne;
	scanf(" %c%d",&colonne,&ligne); // Pour eviter bug double demande, soit on met un espace, soit on met un while(getchar\n)
    *y = colonne - 'a';  // a-h → 0-7
    *x = ligne - 1;      // 1-8 → 0-7
}

void gagnant(char plateau[N][N]) {// Affiche qui a gagné à la fin de la partie à partir d'un plateau de jeu othello
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

// Fonctions pour l'interface codingame: -----------------------------------------------------------

void actualisation_plateau(char plateau[N][N]){ // lis en entrée un plateau sous format string et le stocke dans le plateau passé en paramètre
    for (int i = 0; i < N; i++) { // Lire etat plateau actuel
            // rows from top to bottom (viewer perspective).
        char line[13] = "";
        scanf("%s", line);
        for (int j=0;j<N;j++){
            if(line[j]=='0') plateau[i][j]=NOIRS;
            else if(line[j]=='1') plateau[i][j]=BLANCS;
            else plateau[i][j]=VIDE;
        }
        }
}

void traduction(int x, int y, char *resultat) { //retourne la traduction du coup (de (x,y) à (a-h1-8))
    resultat[0] = 'a' + y;
    resultat[1] = '1' + x;
    resultat[2] = '\0';
}
void envoyer_coup_serveur(int x,int y){ // Afficher le coup correspondant aux coordonnées (x,y) dans le tableau
        char coup[3];
        traduction(x, y, coup);
        printf("%s\n",coup);
}
void envoyer_coup_serveur_et_message(int x,int y){ // pareil + message avec info diverses
        char coup[3];
        traduction(x, y, coup);
        printf("%s MSG %d \n",coup,nb_coup_analyses); // le nombre de position regardees avant chaque decision); // envoie le coup au serveur
}


// Fonctions pour l'API serveur  -----------------------------------------------------------

void traduction_x_y_vers_entier(int x,int y, int*p){ 
    if (x==-1 || y==-1) *p=64; // on passe le tour
    else *p=x*8 +y;
}

void jouer_api_humain(int *x, int *y){ // // modifie les pointeurs x et y par le coup entré par l'utilisateur humain sous format : "a-h1-8"
    printf("Enter your move:\n");
    lire_coup_ia(x,y); //lit le coup de l'utilisateur
} 

void jouer_api_ia(int *x, int *y,short int *g_color,char plateau[N][N]){ // modifie les pointeurs x et y par le coup suggeré par l'IA
    int Depth =4; //profondeur de base 
    if (nb_coup_jouees>50) Depth= 10;// profondeur max il calcule tout car il reste peu de possibilitées
    else if (nb_coup_jouees>45) Depth= 8; // augmente la profondeur en fin de partie; 
    else if (nb_coup_jouees>39) Depth= 6;

    minimax(Depth,x, y,-INF-10,INF+10,g_color,plateau);
    //envoyer_coup_serveur(x,y); //optionnel affiche le coup joué par l'IA
}



/*
Faire en sorte que puisse trouver peut_jouer plus vite, parrallelisme à etudier

*/