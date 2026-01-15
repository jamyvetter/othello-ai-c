//code pour tester les fins de partie, voir si l'ordi trouve meilleur coup. OUI


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>

#define INF 999999 // valeur maximale d'evaluation théoriquement inateignable
#define N 8

double limite_temps = 0.1; // secondes
double debut;
bool temps_depasse =false;

int nb_coup_analyses=0; // le nombre de position regardees avant chaque decision
//int Depth=4; // la profondeur pour les appels de minimax
int nb_coup_jouees=0;// Le stade de la partie en cours

// On utilise uint64_t : un entier non signé de 64 bits exactement
typedef uint64_t Bitboard;

// Structure de jeu
typedef struct {
    Bitboard noir;   // Pions noirs    
    Bitboard blanc; // Pions blancs
} Plateau;


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

void afficher_bitboard_debug(Bitboard b) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // On regarde si le bit à l'index (y*8 + x) est à 1
            int index = y * 8 + x;
            if ((b >> index) & 1ULL) printf("X ");
            else printf(". ");
        }
        printf("%d ",y);
    }
    printf("FIN \n");
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

void afficher_plateau_bitboard_debug(Plateau jeu){ //affiche le plateau complet
    for (int y=0;y<N;y++){
		printf("%d ", y+1);
        for (int x=0;x<N;x++) {
            // On regarde si le bit à l'index (y*8 + x) est à 1 sur l'un des joueurs
            int index = y * 8 + x;
            if ((jeu.noir >> index) & 1ULL) printf("N ");
            else if ((jeu.blanc >> index) & 1ULL) printf("B ");
            else printf(". ");
		}
		printf(";");
	}
	printf("FIN\n");
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

// Fin fonctions de gestion du bitboard  --------------------------------------------------------------------


// Fonction d'évaluation pour l'IA
int evaluation_bitboard(Plateau p, int gene[7]) {
    int score_pondere_blanc=0, score_pondere_noir=0 ; // le score selon le placement des jetons dans l'othellier
    int nb_coup_legal_blanc=0, nb_coup_legal_noir=0; // nom de variable assez explicite

    int nb_jeton_blanc = __builtin_popcountll(p.blanc);
    int nb_jeton_noir = __builtin_popcountll(p.noir);
    if (nb_jeton_blanc==0) return -(INF-1); //Blanc a gagne
    if (nb_jeton_noir==0) return (INF-1); // Noir a perdu
    
    int nb_cases_vides=64 - (nb_jeton_blanc + nb_jeton_noir);
    if (nb_cases_vides==0 ){ // Si fin de partie-> pas vraiment mais la fonction fin de partie est trop lente
        int score_final=nb_jeton_blanc-nb_jeton_noir;
        if (score_final!=0) return ((score_final)/abs(score_final))*(INF-1); // fin de partie donc cette evaluation doit être la plus importante 
            // attention ! il faut que cette valeur soit inferieure à celle de la constante dans minimax (car sinon l'évaluation est trop grande/petite et n'est pas
            //prise en compte par minimax qui considère que le coup est nul)
        else return 0; // egalite 
    }
    // Compte le nombre de coups legaux
    //-> plus rapide: gain de temps enorme, sur 100 parties passe de 10 secondes à 5 !!!
    nb_coup_legal_blanc=__builtin_popcountll(coups_possibles(p.blanc,p.noir));
    nb_coup_legal_noir=__builtin_popcountll(coups_possibles(p.noir,p.blanc));
    
    if ((nb_coup_legal_blanc==0) && (nb_coup_legal_noir==0)){ // Si personne peut jouer = fin de partie
        int score_final=nb_jeton_blanc-nb_jeton_noir;
        if (score_final!=0) return ((score_final)/abs(score_final))*(INF-1); // fin de partie donc cette evaluation doit être la plus importante 
            // attention ! il faut que cette valeur soit inferieure à celle de la constante dans minimax (car sinon l'évaluation est trop grande/petite et n'est pas
            //prise en compte par minimax qui considère que le coup est nul)
        else return 0; // egalite 
    }


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
    /*
    Bitboard c0_BB = 0b00000000'00000000'00000000'00011000'00011000'00000000'00000000'00000000ULL;
    Bitboard c1_BB = 0b00000000'00000000'00011000'00100100'00100100'00011000'00000000'00000000ULL;
    Bitboard c2_BB = 0b00000000'00011000'00000000'01000010'01000010'00000000'00011000'00000000ULL;
    Bitboard c3_BB = 0b00011000'00000000'00000000'10000001'10000001'00000000'00000000'00011000ULL;
    Bitboard c4_BB = 0b00000000'00000000'00100100'00000000'00000000'00100100'00000000'00000000ULL;
    Bitboard c5_BB = 0b00000000'00100100'01000010'00000000'00000000'01000010'00100100'00000000ULL;
    Bitboard c6_BB = 0b00100100'00000000'10000001'00000000'00000000'10000001'00000000'00100100ULL;
    Bitboard c7_BB = 0b00000000'01000010'00000000'00000000'00000000'00000000'01000010'00000000ULL;
    Bitboard c8_BB = 0b01000010'10000001'00000000'00000000'00000000'00000000'10000001'01000010ULL;
    Bitboard c9_BB = 0b10000001'00000000'00000000'00000000'00000000'00000000'00000000'10000001ULL;
*/

    
    Bitboard NOT_A_FILE = 0xfefefefefefefefeULL; // Tout sauf colonne A
    Bitboard NOT_H_FILE = 0x7f7f7f7f7f7f7f7fULL; 
   
    Bitboard c0_BB = 0x18180000010ULL;
    Bitboard c1_BB = 0x182424180000ULL;
    Bitboard c2_BB = 0x18004242001800ULL;
    Bitboard c3_BB =0x240000240000ULL;
    Bitboard c4_BB =0x240000240000ULL;
    Bitboard c5_BB = 0x24420000422400;
    Bitboard c6_BB = 0x2400810000810000ULL;
    Bitboard c7_BB = 0x42000000004200ULL;
    Bitboard c8_BB = 0x4281000000008000ULL;
    Bitboard c9_BB =0x8100000000000000ULL;
    //Poids de chaque case          -------- A determiner/ modifier par l'IA -------------

    // NOTE OPTIMISATION FUTURE !!!!
    // Une fois determines penser à supprimer ceux qui valent 0 du calcul (sert a rien de compter si coeff=0)
    int c0_poids = 0;
    int c1_poids = 4;//0;
    int c2_poids = -3;//-2;
    int c3_poids = 6;//5;
    int c4_poids = 7;//0;
    int c5_poids =-4;// -2;
    int c6_poids = 8;//10;
    int c7_poids = -24;//-50;
    int c8_poids =-8;// -20;
    int c9_poids = 99;//100;

    Bitboard masques_cases[10]={c0_BB,c1_BB,c2_BB,c3_BB,c4_BB,c5_BB,c6_BB,c7_BB,c8_BB,c9_BB};
    int poids_cases[10]= {c0_poids, c1_poids ,c2_poids, c3_poids, c4_poids ,c5_poids,c6_poids, c7_poids ,c8_poids, c9_poids};

    for (int u=0; u<10; u++){ //On somme le tout pour obtenir le score pondere de chaque joueur
    score_pondere_blanc += __builtin_popcountll(p.blanc & masques_cases[u]) *poids_cases[u];
    score_pondere_noir += __builtin_popcountll(p.noir & masques_cases[u])* poids_cases[u];
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
   // printf("bb: %d %d %d %d %d %d \n",nb_coup_legal_blanc,nb_coup_legal_noir,nb_jeton_blanc,nb_jeton_noir,score_pondere_blanc,score_pondere_noir);
    int res= w1*(nb_coup_legal_blanc - nb_coup_legal_noir) + w2*(nb_jeton_blanc - nb_jeton_noir)+ w3*(score_pondere_blanc - score_pondere_noir);
    return res;
}

double obtenir_temps(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

int minimax_bitboard_ia(int depth,int *best_move,int alpha, int betha, bool tour, int gene[7],Plateau plateau)
{
    static int noeuds_visites=0; 
    if (++noeuds_visites % 1024 == 0) {
        if (obtenir_temps() - debut > limite_temps) {
            temps_depasse= true;
            return 678; // arret du minimax
        }
    }

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
           
            if (temps_depasse) return 678;
            
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
            return evaluation_bitboard(plateau,gene); //GAME OVER
        }
        else{
            score= minimax_bitboard_ia(depth-1,NULL,alpha,betha,!tour,gene,plateau);
            if (temps_depasse) return 678;
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
if (best_move != NULL){  // si c'est l'appel de base de minimax (racine), on modifie le meilleur coup
    *best_move=meilleur_coup;
    }

   return meilleur_eval ; // Renvoie le score supposé après tout les appels de minimax, c'est la racine et l'evaluation
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

void actualisation_plateau(Plateau *plateau){ // lis en entrée un plateau sous format string et le stocke dans le plateau passé en paramètre
    plateau->blanc = 0ULL;
    plateau->noir = 0ULL;
    
    for (int i = 0; i < N; i++) { // Lire etat plateau actuel
        // rows from top to bottom (viewer perspective).
        char line[13] = "";
        scanf("%s", line);
        for (int j=0;j<N;j++){
            if(line[j]=='0') {plateau->noir |= 1ULL << (i*8 +j); }
            else if(line[j]=='1'){ plateau->blanc |= 1ULL << (i*8 +j);}
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

void envoyer_coup_codingame_bitboard(int move,int Depth){ // pareil + message avec info diverses
        char coup[3];
        traduction( move/8,move%8 , coup);
        printf("%s MSG %d \n",coup,Depth); // le nombre de position regardees avant chaque decision); // envoie le coup au serveur
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



int main(){
    
    Plateau plateau;
    initialiser_plateau_bitboard(&plateau);
    bool tour=false;

    int p; //le coup à jouer

    nb_coup_jouees=0; //compte le nombre de jeton poses pour avoir une idee de ou en est la partie
    int Depth=5;
    int coeffs_ia[10]={18 ,43, 89, 93 ,6 ,2 ,40};
    
    actualisation_plateau(&plateau);
     
    while (1) { // game loop
        afficher_plateau_bitboard(plateau); 
    
        debut=obtenir_temps(); temps_depasse=false;
        for (int d=5; d<15; d++){
            if (minimax_bitboard_ia(d, &p ,-INF-10,INF+10, tour,  coeffs_ia, plateau)==678 ) break;
            d++;
            Depth=d;
        }
        envoyer_coup_codingame_bitboard(p,Depth);
        int x=0; int y=0;
        lire_coup_ia(&x,&y);
        int index= x*8+y;
        jouer_coup_bitboard(&plateau,index,tour);
        lire_coup_ia(&x,&y); index= x*8+y;
        jouer_coup_bitboard(&plateau,index,!tour);

        
    }
    return 0;
    }