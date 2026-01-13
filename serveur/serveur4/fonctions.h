// Code allégé (pas toutes les fonctions) et compressé, pour fonctionner de manière optimale lors du tournoi

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

#define INF 999999 // valeur maximale lors d'une évaluation d'une position (inateignable)

//Variables globales pour la gestion du temps ecoule
extern  unsigned long long nb_noeuds_vus; // Le nombre d'appel recursifs de minimax
extern double limite_temps; // en secondes
extern double debut; // le moment où on commence à réflechir
extern bool temps_depasse; // passe à true quand on a mis trop de temps

double obtenir_temps(); // Mesure le temps et le stocke dans un double

// Fonctions de gestion du bitboard  --------------------------------------------------------------------
/*Infos:
Deux bitboard: 1 pour les blancs et 1 pour les noirs, 1=jeton sur la case; 0= pas de jeton.
La combinaison des deux (| ou) = le plateau des cases occupees

Documentation: https://www.geeksforgeeks.org/c/builtin-functions-gcc-compiler/ pour les fonctions ultra rapides assembleur (ex: __builtin_popcount() )
*/

// On utilise uint64_t : un entier non signé de 64 bits exactement
typedef uint64_t Bitboard;

// Structure de jeu nommé Plateau
typedef struct {
    Bitboard noir;   // Pions noirs    
    Bitboard blanc; // Pions blancs
} Plateau;

void initialiser_plateau_bitboard(Plateau *jeu); // Modifie jeu en la position de base
void afficher_bitboard(Bitboard b); // affiche le bitboard précisé avec des croix sur les jetons du joueur
void afficher_plateau_bitboard(Plateau jeu); // affiche le plateau, N=case avec jeton noir et B=case avec jeton blanc

void jouer_coup_bitboard(Plateau *p, int index_0_63, bool tour); // Joue le coup de la perspective du joueur tour

int est_coup_legal(Plateau p, bool tour, int coup); // Vérifie qu'on coup (entier de 0à63) est legal (== retourne des jetons et coup est une case vide)

int peut_jouer_bitboard(Plateau p, bool tour);// obsolète
bool peut_jouer_bb(Bitboard moi, Bitboard adv);// vérifie que le joueur moi a des coups dispoibles
bool fin_de_partie_bitboard(Plateau p); // true quand aucun joueur n'a de coup disponible
Bitboard coups_possibles(Bitboard moi, Bitboard adv) ; // renvoie le bitboard des coups possibles



int evaluation_bitboard(Plateau p);// évalue un plateau avec -X si les noirs sont mieux et +X si les blancs sont mieux

//Fin Fonctions de gestion du bitboard  --------------------------------------------------------------------

// IAS diverses

void ordi_random_bitboard(int *p, bool tour,Plateau plateau); // IA qui joue un coup aléatoirement (pas vraiment) parmi ses possibilitées
void ordi_greedy_bitboard(int *p, bool tour,Plateau plateau);// Modifie p  avec un coup qui maximise le nombre de jetons du joueur tour à une profondeur 1


// Fonctions d'interface utilisateur -----------------------------------------------------------

void lire_coup_ia(int *x, int *y); // lis un coup entré au clavier et le stocke avec le bon format pour le tableau dans les pointeurs x,y
// Fonctions pour l'interface codingame: -----------------------------------------------------------

void traduction(int x, int y, char *resultat) ; //retourne la traduction du coup (de (x,y) à (a-h1-8))
void envoyer_coup_serveur(int x,int y); // Afficher le coup correspondant aux coordonnées (x,y) dans le tableau
void envoyer_coup_serveur_et_message(int x,int y); // pareil + message avec info diverses
void traduction_x_y_vers_entier(int x,int y, int*p);

//Fonctions pour l'API serveur---------
void jouer_api_humain(int *x, int *y); // // modifie les pointeurs x et y par le coup entré par l'utilisateur humain sous format : "a-h1-8"



/* HASH ZOBRIST:
En résumé, on hash chaque position analysée dans la table TT_ENTRY
Les hash fonctionnent avec des XOR (car réversible et donc rapide)
Pour retrouver les position déjà analysées, on vérifie que la key de la position corresponde à celle stockée dans TT_ENTRY (pour plus d'infos voir code dans minimax)
*/

#define TT_SIZE (1 << 20) // Environ 1 million d'entrées
#define TT_FLAG_EXACT 2
#define TT_FLAG_LOWER -1
#define TT_FLAG_UPPER 1

typedef struct {
    uint64_t key;   // Le hash complet (pour vérifier les collisions)
    int depth;      // La profondeur à laquelle ce score a été calculé
    int score;      // Le score trouvé
    int flag;       // TT_FLAG_EXACT, TT_FLAG_UPPER (borne sup) ou TT_FLAG_LOWER (borne inf)
    int best_move;  // Le meilleur coup trouvé ici
} TT_Entry;


// Reference extern au fichier fonctions.c , on met ces tableaux en global sinon trop de choses à retenir lors des appels recursifs pour le processeur
extern uint64_t ZOBRIST_TABLE[64][2]; // 64 cases, 2 couleurs
extern uint64_t ZOBRIST_TOUR;         // Pour hacher le tour de jeu
extern TT_Entry transposition_table[TT_SIZE];

// Note de programmation: si on veut utiliser ces variables globales dans un .c , il faut les déclarer AVANT le main et une seule fois.


void init_zobrist(); // intialise le tableau zobrist avec un hash pour chaque combo case/couleur, aléatoirement
uint64_t calculer_hash_complet(Bitboard noir, Bitboard blanc, bool tour); // calcule le hash d'une position à partir des bitboards


void jouer_coup_bitboard_hash(Plateau *p, uint64_t *nouveau_hash, int index_0_63, bool tour); // modifie aussi nouveau_hash selon les pions retournes

// Le minimax dépend des variables globales de temps, et demande aussi le hash de la position
int minimax_hash_temporel(int depth,int *best_move,int alpha, int betha, bool tour,Plateau plateau,uint64_t hash); //version adaptative du temps restant


/*
Ouvertures
On part du principe que tout le monde va jouer en haut à gauche au début: d3
J'ai codé manuellement des ouvertures sur lesquelles l'IA avait du mal
*/


#define TAILLE_LIVRE_OUVERTURE 20
typedef struct {
    uint64_t key; //le hash de la position
    uint64_t move; // le coup connu
} position_connue; // une position d'ouverture connue avec son coup sera stockée dans cette structure

extern position_connue opening_book[TAILLE_LIVRE_OUVERTURE]; // faire un malloc??

// livre le plus à jour, et pour le hash, true:false indique qui a le tour (true=à blanc de jouer)
void init_hash_ouvertures(position_connue opening_book[TAILLE_LIVRE_OUVERTURE]); // ! A faire après init du hash zobrist

int recherche_ouverture(uint64_t hash);//Cherche dans le livre d'ouvertures(global) si la position associée à hash est connue, 
// au quel cas on renvoie le coup connu. Sinon, renvoie -1


