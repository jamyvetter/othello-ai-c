#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

#define N 8//taille du plateau
#define BLANCS 'B'
#define NOIRS 'N'
#define VIDE '.'
#define BLACK 0

#define INF 999999 // valeur maximale théoriquement inateignable

extern  unsigned long long nb_noeuds_vus; // Le nombre d'appel recursifs de minimax

/*Variables globales pour la gestion du temps ecoule*/
extern double limite_temps; // secondes
extern double debut;
extern bool temps_depasse;

double obtenir_temps(); // Mesure le temps sur un double


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
Bitboard coups_possibles(Bitboard moi, Bitboard adv) ; // renvoie le bitboard des coups possibles

int evaluation_bitboard(Plateau p);// evalue un plateau, comme d'habitude

// Format du tableau coeff: undefined for now

int minimax_bitboard_ia(int depth,int *best_move,int alpha, int betha, bool tour, int gene[7],Plateau plateau);

//Fin Fonctions de gestion du bitboard  --------------------------------------------------------------------


// Fonctions pour la mécanique de jeu

//On definit deux structures afin de jouer un coup et de retenir les cases affectées par ce coup (pour pouvoir faire des retours en arrière)



void ordi_random_bitboard(int *p, bool tour,Plateau plateau);
void ordi_greedy_bitboard(int *p, bool tour,Plateau plateau);// Modifie p  avec un coup qui maximise le nombre de jetons du joueur tour


// Fonctions d'interface utilisateur -----------------------------------------------------------

void lire_coup_ia(int *x, int *y); // lis un coup entré au clavier et le stocke avec le bon format pour le tableau dans les pointeurs x,y
// Fonctions pour l'interface codingame: -----------------------------------------------------------

void traduction(int x, int y, char *resultat) ; //retourne la traduction du coup (de (x,y) à (a-h1-8))
void envoyer_coup_serveur(int x,int y); // Afficher le coup correspondant aux coordonnées (x,y) dans le tableau
void envoyer_coup_serveur_et_message(int x,int y); // pareil + message avec info diverses
void traduction_x_y_vers_entier(int x,int y, int*p);

//Fonctions pour l'API serveur---------
void jouer_api_humain(int *x, int *y); // // modifie les pointeurs x et y par le coup entré par l'utilisateur humain sous format : "a-h1-8"



// NOUVEAU ZOBRIST HASH:

#define TT_SIZE (1 << 20) // Environ 1 million d'entrées
#define TT_FLAG_EXACT 2
#define TT_FLAG_LOWER -1
#define TT_FLAG_UPPER 1

typedef struct {
    uint64_t key;   // Le hash complet (pour vérifier les collisions)
    int depth;      // La profondeur à laquelle ce score a été calculé
    int score;      // Le score trouvé
    int flag;       // EXACT, ALPHA (borne sup) ou BETA (borne inf)
    int best_move;  // Le meilleur coup trouvé ici
} TT_Entry;


// Reference extern au fichier fonctions.c , on met ces tableaux en global sinon trop de choses à retenir lors des appels recursifs pour le processeur
extern uint64_t ZOBRIST_TABLE[64][2]; // 64 cases, 2 couleurs
extern uint64_t ZOBRIST_TOUR;         // Pour hacher le tour de jeu
extern TT_Entry transposition_table[TT_SIZE]; // doit etre declare ailleurs lol



void init_zobrist(); // intialise le tableau zobrist avec un hash pour chaque combo case/couleur
uint64_t calculer_hash_complet(Bitboard noir, Bitboard blanc, bool tour); // calcule le hash d'une position à partir des bitboards


void jouer_coup_bitboard_hash(Plateau *p, uint64_t *nouveau_hash, int index_0_63, bool tour); // modifie aussi nouveau_hash selon les pions retournes

int minimax_hash_temporel(int depth,int *best_move,int alpha, int betha, bool tour,Plateau plateau,uint64_t hash); //version adaptative du temps restant


/*NOVEAU, ouvertures
On part du principe que tout le monde va jouer en haut à gauche au début: d3
*/
#define TAILLE_LIVRE_OUVERTURE 20
typedef struct {
    uint64_t key; //le hash de la position
    uint64_t move; // le coup connu
} position_connue; // une position d'ouverture connue avec son coup sera stockée dans cette structure

extern position_connue opening_book[TAILLE_LIVRE_OUVERTURE]; // faire un malloc??

// livre le plus à jour, et pour le hash, true:false indique qui a le tour (true=à blanc de jouer)
void init_hash_ouvertures(position_connue opening_book[TAILLE_LIVRE_OUVERTURE]); // ! A faire après init du hash zobrist

int recherche_ouverture(uint64_t hash);
    /*Cherche dans le livre d'ouvertures(global) si la position associée à hash est connue, au quel cas on renvoie le coup connu. Sinon, renvoie -1*/

