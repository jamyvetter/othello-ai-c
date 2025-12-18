#include <stdbool.h>

#define N 8//taille du plateau
#define BLANCS 'B'
#define NOIRS 'N'
#define VIDE '.'

void initialiser_plateau(char plateau[N][N]);             // crée la configuration de départ
void afficher_plateau(char plateau[N][N]);                // affiche le plateau

bool coup_dans_plateau(int x,int y);               // teste si les coordonnées sont valides
bool action_effectuee(int x, int y,bool tour,char plateau[N][N]);  // teste si un coup capture au moins une tuile
bool coup_valide(int x, int y, bool tour, char plateau[N][N]);                     // dis si le coup est jouable
bool jouer_coup(int x, int y,bool tour,char plateau[N][N]);                      // place la tuile et retourne les adversaires
void score_joueur(int *score_blanc, int *score_noir, char plateau[N][N]);                    // compte les scores
bool peut_jouer(bool tour, char plateau[N][N]);              // true si le joueur peut jouer(a des coups possibles)
void lire_coup(int *x, int *y,bool tour);         // lis un coup dans le terminal et le décode
bool fin_de_partie(char plateau[N][N]);                   // renvoie true si la partie est finie
void gagnant(char plateau[N][N]);                         // affiche le résultat final