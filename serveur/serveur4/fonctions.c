//Contient toutes les fonctions utiles à l'othello.


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "fonctions.h"
#include <stdint.h>
#include <sys/time.h>

#define INF 999999 // valeur maximale d'evaluation théoriquement inateignable


int nb_coup_analyses=0; // le nombre de position regardees avant chaque decision
//int Depth=4; // la profondeur pour les appels de minimax
int nb_coup_jouees=0;// Le stade de la partie en cours


// Gestion temporelle: mesure du temps

double obtenir_temps(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}


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

///* POUR ZOBRIST, NOUVEAU

//NOUVEAU ICI, HASH ZOBRIST

uint64_t ZOBRIST_TABLE[64][2]; // 64 cases, 2 couleurs
uint64_t ZOBRIST_TOUR;         // Pour hacher le tour de jeu
TT_Entry transposition_table[TT_SIZE];// le tableau des positions dejà analysées, contentant des elements de type TT_Entry



void init_zobrist() {
    for (int i = 0; i < 64; i++) {
        ZOBRIST_TABLE[i][0] = (uint64_t)rand() << 32 | rand(); // Pion Noir
        ZOBRIST_TABLE[i][1] = (uint64_t)rand() << 32 | rand(); // Pion Blanc
        /*rand() fait un entier sur 32 bits donc on appelle deux fois rand() et on décale pour l'avoir sur 64 bits*/
    }
    ZOBRIST_TOUR = (uint64_t)rand() << 32 | rand();
}

uint64_t calculer_hash_complet(Bitboard noir, Bitboard blanc, bool tour) {
    /* Calcule le hash d'une position donnée. Pour cela on XOR toutes les cases remplies avec le hash de joueur correspondant à la case*/
    
    uint64_t h = 0;
    //Bitboard n = noir;
    //Bitboard b = blanc;
    
    while (noir) {
        int i = __builtin_ctzll(noir);
        h ^= ZOBRIST_TABLE[i][0];
        noir &= (noir - 1);
    }
    while (blanc) {
        int i = __builtin_ctzll(blanc);
        h ^= ZOBRIST_TABLE[i][1];
        blanc &= (blanc - 1);
    }
    if (tour) h ^= ZOBRIST_TOUR;
    return h;
}

void jouer_coup_bitboard_hash(Plateau *p, uint64_t *nouveau_hash, int index_0_63, bool tour){
    Bitboard *joueur; Bitboard *adversaire;
    
    //On modifie les pointeurs selon quel joueur joue le coup
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

    // UPDATE DU hash
    while (captures) { // Pour chaque case capturee, on fait le XOR associé 
        int i = __builtin_ctzll(captures);
        *nouveau_hash ^= ZOBRIST_TABLE[i][0];
        *nouveau_hash ^= ZOBRIST_TABLE[i][1];
        captures &= (captures - 1);
    }

    *nouveau_hash ^= ZOBRIST_TABLE[index_0_63][tour ? 1 : 0]; // Pour cette case il n'y a que le joueur tour qui peut interagir avec car il pose son jeton dessus
    *nouveau_hash ^= ZOBRIST_TOUR; // changement de tour, après le coup c'est à !tour de jouer
}

///*/





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
/*bool fin_de_partie_bitboard(Plateau p) { // détecte la fin du jeu
    return (!peut_jouer_bitboard( p, true) && !peut_jouer_bitboard( p, false));
}*/

// Pour savoir si le joueur 'moi' peut jouer: 
bool peut_jouer_bb(Bitboard moi, Bitboard adv) {
    return coups_possibles(moi, adv) != 0;
}


bool fin_de_partie_bitboard(Plateau p) { // détecte la fin du jeu
    return (!peut_jouer_bb( p.blanc, p.noir) && !peut_jouer_bb( p.noir, p.blanc));
}



// Cette fonction là ne dépend pas du coup et renvoie un Bitboard de tous les coup legaux dans une direction donnee.
//vide: bitboard avec des 1 sur les cases vides
// is_left_shift : true : << , false : >> , inverse juste la fonction
static inline Bitboard get_moves_in_dir(Bitboard moi, Bitboard adv, Bitboard vide, int shift, Bitboard mask, bool is_left_shift) 
{
        /* Pour trouver les cases ou un joueur peut jouer, on recherche le pattern moi {i*adv} vide*/    
    
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



// Fonctions pour l'IA / la recherche du meilleur coup à jouer -----------------------------------------------------------

int evaluation_bitboard(Plateau p) {
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
    /*
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
*/
    Bitboard masques_cases[10]={c0_BB,c1_BB,c2_BB,c3_BB,c4_BB,c5_BB,c6_BB,c7_BB,c8_BB,c9_BB};
    //int poids_cases[10]= {c0_poids, c1_poids ,c2_poids, c3_poids, c4_poids ,c5_poids,c6_poids, c7_poids ,c8_poids, c9_poids};
    int poids_cases[10]= {0, 4 ,-3, 6, 7 ,-4,8, -24 ,-8, 99};

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
    
                        // --------------------- Critère de mobilité (nb de coups legaux) -------------------------
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


    /*Opti avec fonction lineaire: f1 debut de partie; f2 fin de partie
    NOTE: implicite float vers int, faut aussi majorer tout ça jsp
    */
    // gene[0] gene[1]....... gene[5]
    int f1 = 88*(nb_coup_legal_blanc - nb_coup_legal_noir) + 15*(nb_jeton_blanc - nb_jeton_noir)+ 34*(score_pondere_blanc - score_pondere_noir);
    int f2 = 60*(nb_coup_legal_blanc - nb_coup_legal_noir) + 35*(nb_jeton_blanc - nb_jeton_noir)+ -2*(score_pondere_blanc - score_pondere_noir);
    return (f1*nb_cases_vides + f2 *(60- nb_cases_vides ) )/60 ;
}

int evaluation_bitboard_finale(Plateau plateau) { // Quand la partie est finie, le score est sur et on le calcule plus vite avec ça:

    int score_final=__builtin_popcountll(plateau.blanc)-__builtin_popcountll(plateau.noir);
    if (score_final!=0) return ((score_final)/abs(score_final))*(INF-1);
    else return 0; 

}


int minimax_hash_temporel(int depth,int *best_move,int alpha, int betha, bool tour,Plateau plateau,uint64_t hash)
/* Minimax qui s'adapte selon les variables temps_depasse,debut,limite_temps (globales)*/
{
    static int noeuds_visites=0; 
    if (++noeuds_visites % 1024 == 0) {
        if (obtenir_temps() - debut > limite_temps) {
            temps_depasse= true;
            return 678; // arret du minimax, on n'a pas trouvé le meilleur coup dans les temps
        }
    }

    int alpha_original=alpha, betha_original=betha;
    int tt_coup= 64; // le meilleur coup vu sur une analyse de la position

    //  Recherche dans la TT
    TT_Entry *entry = &transposition_table[hash % TT_SIZE];
    
    if (entry->key == hash && entry->depth >= depth) {
        if (entry->flag == TT_FLAG_EXACT){
            if (best_move != NULL){  // si c'est l'appel de base de minimax (racine), on modifie le meilleur coup
                *best_move=entry->best_move;
                }  
            return entry->score;}

        // Si c'est une borne INF (valeur réelle >= score), ça peut couper Beta
        else if (entry->flag == TT_FLAG_LOWER && entry->score >= betha) {
            //if (best_move != NULL){  // si c'est l'appel de base de minimax (racine), on modifie le meilleur coup
              //  *best_move=entry->best_move;
                //}   
            return entry->score;}
        
        // Si c'est une borne SUP (valeur réelle <= score), ça peut couper Alpha
        else if (entry->flag == TT_FLAG_UPPER && entry->score <= alpha){ 
                //if (best_move != NULL){  // si c'est l'appel de base de minimax (racine), on modifie le meilleur coup
                //*best_move=entry->best_move;
                //}  
            
            return entry->score;}
    }
    
    if (depth == 0)return evaluation_bitboard(plateau); // pas efficace car pour la fonction fin de partie, on reparcoure le plateau, ça prend du temps
   // NOTE: on copie le tableau gene a chaque appel de minimax, on pourrait optimiser et l'utiliser qu'a la fin (à l'appel profondeur 0, var globale gene une 
   //fois que le meileur gene est determine)
   
   int meilleur_eval; int score;
   int meilleur_coup=64;// meilleur coup trouvé jusque là

    if (hash== entry->key ){ // on a déjà analysé la position à profondeur basse peut etre mais un meilleur coup local est utile à savoir
            tt_coup=entry->best_move;
    }

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
            score= minimax_hash_temporel(depth-1,NULL,alpha,betha,!tour,plateau, hash^ZOBRIST_TOUR); // NOTE depth doit aussi marcher car on a rien calculé ----------------------------
            if (temps_depasse) return 678;
            if (best_move != NULL) *best_move=64; // On passe notre tour si appel de base minimax
            return score;
        }
    }
    else{// On a passé le IF, on a donc des coups legaux
       // /*
        // TEST du coup prioritaire si il existe/ a ete trouve, on verifie aussi qu'il est legal
        if (tt_coup >= 0 && tt_coup < 64 && (coups_legaux_joueur & (1ULL << tt_coup))) {
            uint64_t prochain_hash = hash;
            Plateau copie_plateau = plateau;

            jouer_coup_bitboard_hash(&copie_plateau, &prochain_hash, tt_coup, tour);
            score = minimax_hash_temporel(depth - 1, NULL, alpha, betha, !tour, copie_plateau, prochain_hash);

            if (score > meilleur_eval) {
                meilleur_eval = score;
                meilleur_coup = tt_coup;
            }
            if (score > alpha) alpha = score;
            
            // Si coupure immédiate, on ne regarde même pas les autres coups
            if (betha <= alpha) {
                // Optionnel : mise à jour TT ici si nécessaire avant de return
                goto fin_recherche; 
            }

            // IMPORTANT : On retire ce coup du bitboard pour ne pas le rejouer dans le while
            coups_legaux_joueur &= ~(1ULL << tt_coup);
        }
//*/
        // TEST des autres coups 

        while (coups_legaux_joueur){ // tant qu'il reste des coups à tester
            int index = __builtin_ctzll(coups_legaux_joueur); // Donne l'index du premier bit à 1 == le premier coup dispo à tester
            coups_legaux_joueur= coups_legaux_joueur & (coups_legaux_joueur-1) ;// suppression du coup qu'on vient de regarder


            uint64_t prochain_hash= hash;
            Plateau copie_plateau = plateau;

            jouer_coup_bitboard_hash(&copie_plateau,&prochain_hash,index,tour);
            score = minimax_hash_temporel(depth - 1, NULL,alpha,betha, !tour, copie_plateau,prochain_hash);
            if (temps_depasse) return 678;
            
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
            score= minimax_hash_temporel(depth-1,NULL,alpha,betha,!tour,plateau,hash^ZOBRIST_TOUR);
            if (temps_depasse) return 678;
            if (best_move != NULL) *best_move=64; // On passe notre tour si appel de base minimax
            return score;
        }
    }
    else{// On a passé le IF, on a donc des coups legaux
        ///*
        // TEST du coup prioritaire si il existe/ a ete trouve, on verifie aussi qu'il est legal
        if (tt_coup >= 0 && tt_coup < 64 && (coups_legaux_joueur & (1ULL << tt_coup))) {
            uint64_t prochain_hash = hash;
            Plateau copie_plateau = plateau;

            jouer_coup_bitboard_hash(&copie_plateau, &prochain_hash, tt_coup, tour);
            score = minimax_hash_temporel(depth - 1, NULL, alpha, betha, !tour, copie_plateau, prochain_hash);

            if (score < meilleur_eval) {
                meilleur_eval = score;
                meilleur_coup = tt_coup;
            }
            if (score < betha) betha = score;
            
            // Si coupure immédiate, on ne regarde même pas les autres coups
            if (betha <= alpha) {
                // Optionnel : mise à jour TT ici si nécessaire avant de return
                goto fin_recherche; 
            }

            // IMPORTANT : On retire ce coup du bitboard pour ne pas le rejouer dans le while
            coups_legaux_joueur &= ~(1ULL << tt_coup);
        }
//*/        
        while (coups_legaux_joueur){ // tant qu'il reste des coups à tester
            int index = __builtin_ctzll(coups_legaux_joueur); // Donne l'index du premier bit à 1 == le premier coup dispo à tester
            coups_legaux_joueur= coups_legaux_joueur & (coups_legaux_joueur-1) ;// suppression du coup qu'on vient de regarder
            
            uint64_t prochain_hash= hash;
            Plateau copie_plateau = plateau;
            jouer_coup_bitboard_hash(&copie_plateau,&prochain_hash,index,tour);
            score = minimax_hash_temporel(depth - 1, NULL,alpha,betha, !tour, copie_plateau,prochain_hash);
            if (temps_depasse) return 678;
            if (score < meilleur_eval) {
                meilleur_eval = score;
                meilleur_coup =index ;
            }
            if (score<betha) betha=score;
            if (betha<=alpha) break;  // le noeud est condamne
        }    
    
    }
    }

fin_recherche:
if (best_move != NULL){  // si c'est l'appel de base de minimax (racine), on modifie le meilleur coup
    *best_move=meilleur_coup;
    }

//Actualiser la TT selon les resultats du minimax
if (entry->key != hash || depth >= entry->depth) {
    entry->key = hash;
    entry->depth = depth;
    entry->score = meilleur_eval;
    entry->best_move = meilleur_coup;

    if (tour) { 
        if (meilleur_eval <= alpha_original) entry->flag = TT_FLAG_UPPER;
        else if (meilleur_eval >= betha)      entry->flag = TT_FLAG_LOWER;
        else                                  entry->flag = TT_FLAG_EXACT;
    } 
    else {
        // Attention : Logique INVERSÉE pour les bornes
        if (meilleur_eval <= alpha)          entry->flag = TT_FLAG_UPPER; 
        else if (meilleur_eval >= betha_original) entry->flag = TT_FLAG_LOWER;
        else                                  entry->flag = TT_FLAG_EXACT;
    }
}

   return meilleur_eval ; // Renvoie le score supposé après tout les appels de minimax, c'est la racine et l'evaluation
}
//-------------

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


// Fonctions pour l'interface codingame: -----------------------------------------------------------

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

	// NOTE: modifier pour que ça vérifie que le coup est légal
} 

// Les ouvertures ont été trouvées sur ce site: https://samsoft.org.uk/reversi/bigbook.asp
// J'ai manuellement mis les positions où l'IA avait faux.
// livre le plus à jour, et pour le hash, true:false indique qui a le tour (true=à blanc de jouer)
void init_hash_ouvertures(position_connue opening_book[TAILLE_LIVRE_OUVERTURE]){ // ! A faire après init du hash zobrist
    opening_book[0].key = calculer_hash_complet((1ULL << 28) | (1ULL << 35),(1ULL << 27) | (1ULL << 36),false);
    opening_book[0].move= 19; //D3
    
    opening_book[1].key = calculer_hash_complet((1ULL << 19) | (1ULL << 27) | (1ULL << 35), (1ULL << 20) |(1ULL << 28) | (1ULL << 36),false);
    opening_book[1].move= 29; //d3-e3-F4 ; erreur des blancs dès le premier coup

    opening_book[2].key = calculer_hash_complet((1ULL << 28) | (1ULL << 29) | (1ULL << 35), (1ULL << 18) |(1ULL << 19) | (1ULL << 20)|(1ULL << 27) | (1ULL << 36) ,false);
    opening_book[2].move= 37; //d3-e3-F4-c3-f5

    opening_book[3].key = calculer_hash_complet((1ULL << 19) | (1ULL << 28) | (1ULL << 35), (1ULL << 18) |(1ULL << 27) | (1ULL << 36),false);
    opening_book[3].move= 26; //d3-c3- c4

    /*transpostions blanches, on va dans la position de l'etoile car les deux coups blancs apres sont les deux meilleurs et on transpsoe*/
    
    opening_book[4].key = calculer_hash_complet((1ULL << 19) | (1ULL << 27) | (1ULL << 28) | (1ULL << 35) , (1ULL << 36),true);
    opening_book[4].move= 18; //d3-c3-

    opening_book[5].key = calculer_hash_complet((1ULL << 26) | (1ULL << 27) | (1ULL << 28) | (1ULL << 35) , (1ULL << 36) ,true);
    opening_book[5].move= 18; //c4-c3

    opening_book[6].key = calculer_hash_complet((1ULL << 28) | (1ULL << 35) | (1ULL << 36) | (1ULL << 37), (1ULL << 27),true);
    opening_book[6].move= 45; //f5-f6

    opening_book[7].key = calculer_hash_complet((1ULL << 28) | (1ULL << 35) | (1ULL << 36) | (1ULL << 44), (1ULL << 27),true);
    opening_book[7].move= 45; //e6-f6
// fin etoile
    opening_book[8].key = calculer_hash_complet((1ULL << 26) | (1ULL << 27) | (1ULL << 35), (1ULL << 18)| (1ULL << 19)| (1ULL << 20)| (1ULL << 28) | (1ULL << 36),false);
    opening_book[8].move= 29; //d3-c3-c4-e3-f4, on rentre dans 'the COW'

    opening_book[9].key = calculer_hash_complet((1ULL << 19) | (1ULL << 27) | (1ULL << 28), (1ULL << 18) | (1ULL << 26) | (1ULL << 34) | (1ULL << 35) | (1ULL << 36),false);
    opening_book[9].move= 43; //d3-c3-c4-c5-d6, on rentre dans 'the COW'

}

int recherche_ouverture(uint64_t hash){
    /*Cherche dans le livre d'ouvertures(global) si la position associée à hash est connue, au quel cas on renvoie le coup connu. Sinon, renvoie -1*/
    for (int pos=0; pos< TAILLE_LIVRE_OUVERTURE; pos++){
        if (opening_book[pos].key == hash) return opening_book[pos].move;
    }
    return -1; // Pas de coup connu
}


