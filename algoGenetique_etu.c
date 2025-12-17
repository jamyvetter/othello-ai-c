#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#include "algoGenetique.h"
#include "fonctions.h"

#define ELITE
#define NBPARTIES 3

//===============================================
// fonction d'affichage d'une population (paramètre en entrée) 
//
//===============================================
void displayPopulation(groupe population)
{
	for (int i = 0; i < population.nombre; i++) {
		afficheCircuit(population.membres[i]);
	}
}


//fonction de selection des parents en fonction de la population 
//===============================================
void selection(groupe * population, groupe * parents)
{
	int nb = population->nombre / parents->nombre;

	for (int parent = 0; parent < (parents->nombre); parent++) {
		
		int max, start;
		start = max = parent * nb;
		for (int i = 0; i < nb; i++) { 
			if ( (population->membres[start + i].elo)> (population->membres[max].elo) ) {
				max = start + i;
			}
		}
		parents->membres[parent] = population->membres[max];
		parents->membres[parent].score= 0; 
		parents->membres[parent].nombre_parties_jouees= 0;

	}
}

//===============================================
// fonction d'evaluation d'une population, calcul les scores des membres, l'entropy et trouve le meilleur
// Evalue à une profondeur Depth donnée
//===============================================
void evaluation(groupe * population,int Depth)
{
	double best_elo= -99999.0; // (beaucoup)
	int indice_meilleur_membre=-1;
	//int entropie=0;
	int indice_adversaire; int nb_combats=4;
	int somme_genes[NB_MAX_GENES]={0}; int somme_genes_carre[NB_MAX_GENES]={0}; // pr entropie
	int NB_pop= population->nombre;
	/*Les affrontements on peu pas en faire trop, à voir comment en faire assez sur chaque IA pour un résultat significatif.*/
	for (int i=0; i< NB_pop ; i++ ){ // parcourt tableau du groupe -> pour chaque circuit
		
		/* // enlevé temporairement car peut etre mieux vs random
		for (int x=0; x<nb_combats; x++){ // affrontements aleatoire dans la population
			indice_adversaire= rand()%NB_pop; // Si on tombe sur l'IA elle-même, son score reste inchangé car il y a une revanche
			affrontement_IA(&(population->membres[i]), &(population->membres[indice_adversaire]),Depth);
			affrontement_IA(&(population->membres[indice_adversaire]), &(population->membres[i]),Depth);
		}
			*/
		// Affrontement contre la meilleur IA actuelle
		if (i!= population->best){
			affrontement_IA(&(population->membres[i]),&population->membres[population->best] ,Depth);
			affrontement_IA(&population->membres[population->best],&(population->membres[i]) ,Depth); 
		}

		// Affrontement contre joueur aleatoire, on en censé gagner facilement même à profondeur 1
		for (int i=0; i<10;i++){ affrontement_IA_vs_random( &(population->membres[i]) , 1);}

		//NOTE OPTI: ajouter vs joueur greedy

		for (int g=0; g<NB_MAX_GENES;g++){ // calcul entropie
			int valeur_gene=(population->membres[i]).gene[g];
			somme_genes[g]+= valeur_gene;
			somme_genes_carre[g]+= valeur_gene*valeur_gene;
		}

		//Mise a jour du score elo
		if (population->membres[i].nombre_parties_jouees!=0)
		population->membres[i].elo= (double) (population->membres[i].score) /(population->membres[i].nombre_parties_jouees) ;
		else population->membres[i].elo =-999.0; // Si par malheur elle n'a pas jouée


		/*test pour voir si l'ia est la meilleure jusqu'ici*/
		if ( (population->membres[i]).elo > best_elo) {best_elo=(population->membres[i]).elo; indice_meilleur_membre= i;}
	}
	
	int variance_globale=0;
	for (int t=0; t< NB_MAX_GENES ; t++ ){
			int moyenne= somme_genes[t]/ NB_pop; int moyenne_carre= somme_genes_carre[t]/ NB_pop;
			int variance_indicet=moyenne_carre- moyenne*moyenne; // On utilise V(X) = E(X^2) - E(X)^2
			variance_globale += variance_indicet;
	}
	
	population->best = indice_meilleur_membre;
	population->entropy = variance_globale/NB_MAX_GENES;//entropie;	
	printf("Meilleur elo: %f, pour le circuit a l'indice %d\n",best_elo,indice_meilleur_membre);
	printf("Entropie de cette population: %d \n",population->entropy);
}

//===============================================
// fonction de génération d'une population (paramètre en entrée) 
//===============================================
void generationAleatoire(groupe * population)
{	
	for (int i=0; i< population->nombre ; i++ ){ // parcourt tableau du groupe
		for(int j=0;  j<NB_MAX_GENES;j++){
			(population->membres[i]).gene[j]= rand()%100; // intialise les genes à des valeurs randoms
		}
		(population->membres[i]).score=0;(population->membres[i]).nombre_parties_jouees=0;
	}
	population->best=0; population->entropy=0;
}

//===============================================
// fonction de reproduction qui prend parents en entrée et génère une nouvelle popultaion (sortie) 
//===============================================
void reproduction(groupe * population, groupe parents)
{
	circuit p1, p2;
	// génère tous les fils dans une "nouvelle" population
	for (int fils = 0; fils < population->nombre; fils++) {
		p1 = parents.membres[rand() % (parents.nombre)]; // choix des parents
		p2 = parents.membres[rand() % (parents.nombre)];
		
		circuit fiston; fiston.score=0;
		for (int g=0; g<NB_MAX_GENES ;g++){ // choix des genes pour l'enfant
			if (rand()%2==0) fiston.gene[g]= p1.gene[g]; // le parent un a gagne
			else fiston.gene[g]= p2.gene[g];

		}
		population->membres[fils] = fiston; // on copie les genes determinees dans le fils
		population->membres[fils].score=0;population->membres[fils].nombre_parties_jouees=0;
	}

	// on conserve toujours le meilleur père dans le fils #0 
	population->membres[0] = parents.membres[parents.best];
//Note: pour optimiser on peut peut etre faire deux enfants avec un qui serait le complementaire de l'autre

}
//===============================================


//===============================================
// fonction de mutation qui prend population (entrée) et la modifie (sortie) 
//===============================================
void mutation(groupe * population)
{
	int indice_mute=rand()%NB_MAX_GENES; // le gene qui va être modifie
	int taux_mutation= 10; // plus ou moins de mutations
	int variation_gene = 6; // plus ou moins grand pour des mutations plus ou moins importantes 
	if (population->entropy < 40*population->nombre){taux_mutation=60;variation_gene = 3;} 
	if (population->entropy < 10*population->nombre){taux_mutation=80;variation_gene = 1;} 


	for (int i=0; i< population->nombre ; i++ ){ // parcourt tableau du groupe, pr chaque individu
		
		if (rand()% 1000 <= taux_mutation){ 
			if (rand()%2==0) population->membres[i].gene[indice_mute] += variation_gene;
			else population->membres[i].gene[indice_mute] -= variation_gene;
		}
		
		}
		}



//===============================================
// fonction d'affichage d'un circuit (paramètre en entrée) 
//===============================================
void afficheCircuit(circuit c)
{
	for (int i = 0; i < NB_MAX_GENES; i++) {
		printf("%d ", c.gene[i]);
	}
	printf(" --> elo : %f, nb_games: %d score:%d\n", c.elo, c.nombre_parties_jouees,c.score);
}

//===============================================
// Fait jouer deux Ias, modifie leur score et leur nombre de parties jouees
//===============================================
void affrontement_IA(circuit *c1, circuit *c2,int Depth){ 

	// Stockage de notre plateau de jeu
	Plateau plateau;
	initialiser_plateau_bitboard(&plateau);
	int p; // Le coup joué

	int nb_coup_jouees=0; //compte le nombre de jeton poses pour avoir une idee de ou en est la partie
	int fin_de_partie=0; // Si plus de deux pass d'affilé il y a fin de partie -> detecte pas si plateau rempli? si normalement on verra
	bool tour=false;//premier joueur:noir

while (fin_de_partie<=1) {
	p=64;
	if(tour){ // mets les parametres de l'IA true=blanche
		minimax_bitboard_ia(Depth, &p,-INF-10,INF+10, tour,c1->gene,plateau);
	}
	else {minimax_bitboard_ia(Depth, &p,-INF-10,INF+10, tour,c2->gene,plateau);
	}
	
	 // avec parametres de l'IA -> depth 1 pour faire les parties vite
    if (p!=64){ // Si coup légal trouvé
		jouer_coup_bitboard(&plateau,p,tour);// jouer le coup sur notre plateau
        nb_coup_jouees++;
		fin_de_partie=0;
	}
	else{ fin_de_partie++;}// on n'a pas pu jouer
	
	tour=!tour;
	}
	// Fin de parte
	int score_blanc=__builtin_popcountll(plateau.blanc); int score_noir=__builtin_popcountll(plateau.noir); 
	int differentiel_score = score_blanc- score_noir; // Permet de juger si la victoire a ete ecrasante ou serree
	// On modifie le score avec +100 pour une victoire et la difference de nombre de pions 
	if (score_blanc> score_noir) {c1->score += 100 + differentiel_score ; c2->score-= differentiel_score;}
	else if (score_noir>score_blanc )  {c2->score += 100 - differentiel_score ; c1->score+= differentiel_score;}
	else {c2->score += 50; c1->score+=50;} // Risque que les IAs fassent des nulles arangees donc on leur donne que la moitie des points
	c1->nombre_parties_jouees++;c2->nombre_parties_jouees++;
	} 


void affrontement_IA_avec_affichage(circuit *c1, circuit *c2,int Depth){ //avec l'affichage pour avoir des 1v1s de qualite
	// Modifie le score du circuit/individu selon le résulat

	// Stockage de notre plateau de jeu
	Plateau plateau;
	initialiser_plateau_bitboard(&plateau);
	int p; // Le coup joué
	int nb_coup_jouees=0; //compte le nombre de jeton poses pour avoir une idee de ou en est la partie
	int fin_de_partie=0; // Si plus de deux pass d'affilé il y a fin de partie -> detecte pas si plateau rempli? si normalement on verra
	bool tour=false;//premier joueur:noir

while (fin_de_partie<=1) {
	p=64;

	/*Bonus comme cette fonction sert contre le boss, on augmente la profondeur des 1v1s*/
	if (nb_coup_jouees>50) Depth=10;// profondeur max il calcule tout jusqu'au bout
    else if (nb_coup_jouees>45) Depth= 8; // augmente la profondeur en fin de partie
    else if (nb_coup_jouees>39) Depth= 6; 


	if(tour){ // mets les parametres de l'IA true=blanche
		minimax_bitboard_ia(Depth, &p,-INF-10,INF+10, tour,c1->gene,plateau);
	}
	else {minimax_bitboard_ia(Depth, &p,-INF-10,INF+10, tour,c2->gene,plateau);
	}
	
	 // avec parametres de l'IA -> depth 1 pour faire les parties vite
    if (p!=64){ // Si coup légal trouvé
		jouer_coup_bitboard(&plateau,p,tour);// jouer le coup sur notre plateau
        nb_coup_jouees++;
		fin_de_partie=0;
	}
	else{ fin_de_partie++;}// on n'a pas pu jouer
	tour=!tour;
	}
	//printf("Fin de partie\n");
	//afficher_plateau(plateau);
	// Fin de parte
	int score_blanc=__builtin_popcountll(plateau.blanc); int score_noir=__builtin_popcountll(plateau.noir); 
	printf("score: b:%d n:%d \n",score_blanc,score_noir);
	if (score_blanc> score_noir) {c1->score ++; c2->score--;}
	else if (score_noir>score_blanc ) {c2->score ++; c1->score--;}
	//en cas de nulle on fait rien
	//printf("ed\n");
	} 


//===============================================
// Fait jouer l'IA contre le joueur random modifie son score et le nombre de parties jouees
//===============================================
void affrontement_IA_vs_random(circuit *c1,int Depth){ 

	// Stockage de notre plateau de jeu
    Plateau plateau;
    initialiser_plateau_bitboard(&plateau); // même pas besoin mais ça fait pas de mal
    int p; //variables pour le stockage des coordonnées dans le tableau correspondant au coup qui va être joué
    int nb_coup_jouees=0; //compte le nombre de jeton poses pour avoir une idee de ou en est la partie
	int fin_de_partie=0; // Si plus de deux pass d'affilé il y a fin de partie -> detecte pas si plateau rempli? si normalement on verra
	
	
	bool tour=false;
	int ia_a_les_blancs= rand()%2;
while (fin_de_partie<=1) {
	p=64;
	if((tour && ia_a_les_blancs) || (!tour && !ia_a_les_blancs)){ 
		minimax_bitboard_ia(Depth, &p,-INF-10,INF+10, tour,c1->gene,plateau);
	}
	else {ordi_random_bitboard(&p, tour, plateau);}
	
	 // avec parametres de l'IA -> depth 1 pour faire les parties vite
    if (p!=64){ // Si coup légal trouvé
		jouer_coup_bitboard(&plateau,p,tour);// jouer le coup sur notre plateau
        nb_coup_jouees++;
		fin_de_partie=0;
	}
	else{ fin_de_partie++;} // on n'a pas pu jouer
	
	tour=!tour;
}
	// Fin de parte
	int score_blanc=__builtin_popcountll(plateau.blanc); int score_noir=__builtin_popcountll(plateau.noir); 
	int differentiel_score = score_blanc- score_noir; // Permet de juger si la victoire a ete ecrasante ou serree
	// On modifie le score avec +100 pour une victoire et la difference de nombre de pions 
	if (score_blanc> score_noir) {c1->score += 100 + differentiel_score ;}
	else if (score_noir>score_blanc )  {c1->score+= differentiel_score;}
	else {c1->score+=50;} // Risque que les IAs fassent des nulles arangees donc on leur donne que la moitie des points
	c1->nombre_parties_jouees++;
	}