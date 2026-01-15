// Fichier test fonction qui joue un coup pour le bitboard
// Compile avec gcc .\test_unitaires.c ..\fonctions.c -o test -Wall -Wextra
// Excutable avec ./test à condition que savedGames_ soit dans le même dossier

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "fonctions.h" 



// variables globales obligatoirs, explique dans US1 ce pb
double limite_temps=0.1;double debut;bool temps_depasse=false;
position_connue opening_book[TAILLE_LIVRE_OUVERTURE];




// --- Conventions du fichier de test 
#define TEST_BLACK 0
#define TEST_WHITE 1
#define TEST_EMPTY 2

// Structure pour lire une entrée du fichier binaire brut
// Attention : on lit les données brutes, on convertira après
void run_tests(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Impossible d'ouvrir savedGames_");
        return;
    }

    int8_t board_in[64];    // Plateau initial (format fichier)
    int8_t move_data[2];    // Coup (index, couleur)
    int8_t board_out[64];   // Résultat attendu (format fichier)

    int test_id = 0;
    int tests_reussis = 0;

    printf("Demarrage des tests...\n");

    // Boucle de lecture : on lit bloc par bloc
    while (fread(board_in, sizeof(int8_t), 64, f) == 64) {
        fread(move_data, sizeof(int8_t), 2, f);
        fread(board_out, sizeof(int8_t), 64, f);

        test_id++;
        Plateau p = {0, 0};
        
        for (int i = 0; i < 64; i++) {
            if (board_in[i] == TEST_BLACK) {
                p.noir |= (1ULL << i);
            } else if (board_in[i] == TEST_WHITE) {
                p.blanc |= (1ULL << i);
            }
        }

        int index_coup = move_data[0];
        int couleur_coup = move_data[1];
        
        bool est_blanc = (couleur_coup == TEST_WHITE);

        // Jouer le coup demandé
        // NOTE: erreur si le coup demande est 64, par choix de codage je décide que on verifie à l'exterieur de jouer_coup que le coup ne soit pas pass.
        // ça permet notamment de faire moins de requete if move!= 64 dans chaque appel de joeur coup dans minimax (ce qui serait long) 
        

        
        
        Bitboard nouveau_hash= calculer_hash_complet(p.noir,p.blanc,est_blanc);

        // On vérifie au passage la fonction coups_possibles pour voir si on savait bien que ce coup est legal
        if ( coups_possibles(est_blanc?p.blanc:p.noir,est_blanc?p.noir:p.blanc)& (1ULL << index_coup) ) jouer_coup_bitboard_hash(&p, &nouveau_hash, index_coup,est_blanc);
        //jouer_coup_bitboard(&p, index_coup, est_blanc); // Commenté mais marche aussi
         
        
        if (index_coup==64) nouveau_hash ^= ZOBRIST_TOUR; // Si le coup est pass, on doit hasher le tour 
        if (calculer_hash_complet(p.noir,p.blanc,!est_blanc) != nouveau_hash) printf("erreur move:%d %lld %lld\n",index_coup,nouveau_hash,calculer_hash_complet(p.noir,p.blanc,!est_blanc));

        bool success = true; // pour savoir si le coup a fait le resultat attendu
        
        for (int i = 0; i < 64; i++) {
            int8_t valeur_attendue = board_out[i];
            int8_t valeur_reelle = TEST_EMPTY; // Par défaut vide

            // On regarde ce qu'il y a dans le bitboard à l'index i
            if ((p.noir >> i) & 1ULL) {
                valeur_reelle = TEST_BLACK;
            } else if ((p.blanc >> i) & 1ULL) {
                valeur_reelle = TEST_WHITE;
            }

            if (valeur_reelle != valeur_attendue) {
                success = false;
                printf("[ECHEC] Test #%d : Erreur a l'index %d.\n", test_id, i);
                printf("Attendu: %d, Obtenu: %d\n", valeur_attendue, valeur_reelle);
                break; // Pas la peine de continuer ce test
            }
        }

        if (success) {
            tests_reussis++;
        } else {
            printf(" -> Coup joue : index %d, couleur %s\n", index_coup, est_blanc ? "BLANC" : "NOIR");
            afficher_plateau_bitboard(p); // si erreur on veut voir le plateau fautif
        }
    }

    fclose(f);
    printf("------------------------------------------------\n");
    printf("RESULTAT FINAL : %d / %d tests reussis.\n", tests_reussis, test_id);
}

int main() {
    init_zobrist();
    run_tests("savedGames_");
    return 0;
}
