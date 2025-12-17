#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define N 8//taille du plateau
#define BLANCS 'B'
#define NOIRS 'N'
#define VIDE '.'
#define BLACK 0

#define INF 999999 // valeur maximale théoriquement inateignable

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

void initialiser_plateau(char plateau[N][N]);
void afficher_plateau(char plateau[N][N]);
void copierPlateau(char src[N][N], char dest[N][N]);
bool coup_dans_plateau(int x, int y);


// Fonctions de gestion du bitboard  --------------------------------------------------------------------
/*Infos:
Deux bitboard: 1 pour les blancs et 1 pour les noirs, 1=jeton sur la case; 0= pas de jeton.
La combinaison des deux (| ou) = le plateau des cases occupees
*/

// On utilise uint64_t : un entier non signé de 64 bits exactement
typedef uint64_t Bitboard;

// Structure de jeu
typedef struct {
    Bitboard noir;   // Pions noirs    
    Bitboard blanc; // Pions blancs
} Plateau;

void initialiser_plateau_bitboard(Plateau *jeu);
void afficher_bitboard(Bitboard b);
void afficher_plateau_bitboard(Plateau jeu);

void jouer_coup_bitboard(Plateau *p, int index_0_63, bool tour);

int est_coup_legal(Plateau p, bool tour, int coup); // pas necessaire ici peut etre à cacher à l'user

int peut_jouer_bitboard(Plateau p, bool tour);

bool peut_jouer_bb(Bitboard moi, Bitboard adv);

bool fin_de_partie_bitboard(Plateau p);
Bitboard coups_possibles(Bitboard moi, Bitboard adv) ;

int evaluation_bitboard(Plateau p, int gene[7]);// evalue un plateau, comme d'habitude

// Format du tableau coeff: undefined for now

int minimax_bitboard_ia(int depth,int *best_move,int alpha, int betha, bool tour, int gene[7],Plateau plateau);

//Fin Fonctions de gestion du bitboard  --------------------------------------------------------------------


// Fonctions pour la mécanique de jeu

//On definit deux structures afin de jouer un coup et de retenir les cases affectées par ce coup (pour pouvoir faire des retours en arrière)

typedef struct {
    int x, y; // les coordonnes de la case changee
    char old_value; // le contenue précédent (BLANC, NOIR ou VIDE)
} Changement; // la structure pour les changements d'état d'une case

typedef struct {
    Changement modifs[64];
    int nb_modifs;
} HistoriqueCoup;

bool jouer_coup_avec_historique(int x, int y, bool tour, char plateau[N][N], HistoriqueCoup *hist) ;// prend en parametre les coordonnées d'un coup à jouer dans le plateau (x,y)et le joue; 
// sortie:  renvoie true si le coup est valide (a bien retourné une tuile), false sinon. Si le coup est invalide (ne retourne pas de case), on ne place pas le jeton
// + modifie les pointeurs pour enregistrer les etats precedents des jetons retournes

void annuler_coup(HistoriqueCoup *hist, char plateau[N][N]);

bool jouer_coup(int x, int y,bool tour,char plateau[N][N]);// prend en parametre les coordonnées d'un coup valide à jouer dans le plateau (x,y)et le joue; 
// sortie:  renvoie true si le coup est valide (a bien retourné une tuile), false sinon. Si le coup est invalide (ne retourne pas de case), on ne place pas le jeton

void ordi_random(int *x, int *y,bool tour,char plateau[N][N]);// Modifie x,y  avec un coup aleatoire valide
void ordi_random_bitboard(int *p, bool tour,Plateau plateau);

bool action_effectuee(int x, int y,bool tour,char plateau[N][N]);// dis si le coup retourne au moins une tuile
bool coup_valide(int x, int y, bool tour, char plateau[N][N]); // vérifie qu'un coup respecte toutes les règles du jeu (le coup peut être accepté par lire_coup())

 
bool peut_jouer(bool tour, char plateau[N][N]); // dit a partir d'un plateau si le joueur tour peut jouer, autrement il devra passer son tour
bool fin_de_partie(char plateau[N][N]) ;// détecte la fin du jeu
void score_joueur(int *score_blanc, int *score_noir, char plateau[N][N]) ; //compte les pions de chaque joueur


// Fonctions pour l'IA / la recherche du meilleur coup à jouer -----------------------------------------------------------
int evaluation_ia(int gene[7],char plateau[N][N]);
int evaluation2(char plateau[N][N]) ; // renvoie une évaluation de la position; si positif les blancs sont mieux, si négatif les noirs sont mieux. 
// L'évaluation est symétrique, 0-> position égale

int minimax(int depth,int *best_x,int *best_y,int alpha, int betha, bool tour, char plateau[N][N]); // evalue une postion d'othello a une profondeur depth donnee et avec la connaissance de qui peut jouer
// retourne l'evaluation de la position (int) et modifie les variables best_x et best_y pour y noter le meilleur coup trouvé
int minimax_ia(int depth,int *best_x,int *best_y,int alpha, int betha, bool tour, int gene[7],char plateau[N][N]);//version avec parametres pas definis

// Fonctions d'interface utilisateur -----------------------------------------------------------

void lire_coup_ia(int *x, int *y); // lis un coup entré au clavier et le stocke avec le bon format pour le tableau dans les pointeurs x,y
void gagnant(char plateau[N][N]);// Affiche qui a gagné à la fin de la partie à partir d'un plateau de jeu othello

// Fonctions pour l'interface codingame: -----------------------------------------------------------

void actualisation_plateau(char plateau[N][N]); // lis en entrée un plateau sous format string et le stocke dans le plateau passé en paramètre
void traduction(int x, int y, char *resultat) ; //retourne la traduction du coup (de (x,y) à (a-h1-8))
void envoyer_coup_serveur(int x,int y); // Afficher le coup correspondant aux coordonnées (x,y) dans le tableau
void envoyer_coup_serveur_et_message(int x,int y); // pareil + message avec info diverses
void traduction_x_y_vers_entier(int x,int y, int*p);

//Fonctions pour l'API serveur---------
void jouer_api_humain(int *x, int *y); // // modifie les pointeurs x et y par le coup entré par l'utilisateur humain sous format : "a-h1-8"
void jouer_api_ia(int *x, int *y,short int *g_color,char plateau[N][N]); // modifie les pointeurs x et y par le coup suggeré par l'IA

// Programme pour jouer sur codingame: -----------------------------------------------------------
int main_codingame();

//Programme pour tester les fonctions: -----------------------------------------------------------
int main_test();
