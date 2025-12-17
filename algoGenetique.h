#ifndef NBPOPULATION
#define NBPOPULATION 100
#endif 

#ifndef NBPARENTS
#define NBPARENTS 40
#endif 

#define DMAX 999999 
#define NB_MAX_GENES 7

/*
Constantes utiles: 
w1,w2,w3 (fonction d'évaluation) * 3 (car différentes pour chaque phase du jeu)
Phase de parties: ouverture, milieu de jeu et fin de partie (pr faire varier les constantes)
Valeur des cases du plateau: Valeur des coins ! plutôt faire un lien des poids entre eux!
On change les parametres de fonction d'évaluation et fais jouer entre eux les IAs avec diff fonctions d'évaluation.
Faut il fixer des maximums pour pas que la fonction d'évaluation donnne des evals trop petites/trop grandes??


gene:int gene[10] -> [w1 w2 w3 w1' w2' w3' end ]
end: phase ou on passe sur les wi' (fin de partie)
*/

typedef struct {
	int gene[NB_MAX_GENES];  
	int score; //score relatif pas representatif du niveau
	int nombre_parties_jouees; 
	double elo; // le veritable score: score/nombre_parties_jouees ->bon ok rien a voir avec l'elo mais c'est des points quoi
} circuit; 

// le type circuit devient l'IA, avec le tableau gene qui contient les différentes valeurs de constantes nécessaires à l'IA.

typedef struct { 
	circuit *membres; 
	int nombre; // le nb de membres du groupe?.?
	int best; // indice dans tableau membres de la meilleur IA de la population
	int entropy; // mesurer les diffs de constantes choisies
} groupe; 

void afficheCircuit(circuit c); 
void selection(groupe *population,groupe *parents); 
void evaluation(groupe *population,int Depth); 
void displayPopulation(groupe population); 
void generationAleatoire(groupe *population); 
void reproduction(groupe *population,groupe parents); 
void mutation (groupe *population); 
void affrontement_IA(circuit *c1, circuit *c2,int Depth);
void affrontement_IA_avec_affichage(circuit *c1, circuit *c2,int Depth);
void affrontement_IA_vs_random(circuit *c1, int Depth);