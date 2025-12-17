#include "fonctions.h"
#include "algoGenetique.h"
#include <time.h>

int main(){
    
    groupe population,parents;
	population.nombre = NBPOPULATION;
	parents.nombre = NBPARENTS;
  int nbgeneration=0;
    //Allocation mémoire
    if ((population.membres =malloc(sizeof(circuit) * population.nombre)) == NULL)
		exit(EXIT_FAILURE);
	if ((parents.membres = malloc(sizeof(circuit) * parents.nombre)) == NULL)
		exit(EXIT_FAILURE);
    srand(time(NULL));

  // Intialisation de la population, 1ere generation (d'IAs)
	generationAleatoire(&population);
  printf("gen de base:\n");
  evaluation(&population,1);
  displayPopulation(population); printf("\n");

  while (nbgeneration<45) {
	printf("nbgen: %d\n",nbgeneration);
	if (nbgeneration>=30){
		selection(&population, &parents);
 		//displayPopulation(population); printf("\n");
		evaluation(&parents,3); //printf("et\n"); displayPopulation(parents); printf("te\n");
		//      printf("Parent min score = %d -- entropy = %d\n",parents.membres[parents.best].score,parents.entropy); 
	}	
	else{
		selection(&population, &parents); 
		evaluation(&parents,1); //printf("et\n"); 
		//      printf("Parent min score = %d -- entropy = %d\n",parents.membres[parents.best].score,parents.entropy); 
	}
	
	//afficheCircuit(parents.membres[parents.best]);
	reproduction(&population, parents);
	mutation(&population);evaluation(&population,1);
	nbgeneration++;
   // printf("er\n"); displayPopulation(population); printf("re\n");
	}
	
	
	printf("Generation %d\n", nbgeneration);
	evaluation(&population,1);
	displayPopulation(population);
	printf("Best\n");
  evaluation(&parents,2);
	displayPopulation(parents);
	printf("better\n");afficheCircuit(parents.membres[parents.best]);



circuit Magnus; // La boss des IAs
 //circuit c2;

Magnus.score=0; Magnus.nombre_parties_jouees=0;
parents.membres[parents.best].score=0;parents.membres[parents.best].nombre_parties_jouees=0;
//c2.score=0;
//int genes_meilleurs[]={10,1,30,20,25,10, 15,50,0,13}; // l'ia que j'avais avant
//int genes_meilleurs[]={44, 12 ,31, 27, 25 ,33 ,21};// celle là gagne plus
//int genes_meilleurs[]={29, 48 ,56, 69, 9  ,82 ,45};
int genes_meilleurs[]={21, 10, 72, 88, 23, 38, 80}; 

//int genes_candidat[]={31, 91, 59, 23 ,32 ,18 ,54 ,99 ,1 ,75}; //genes de l'outsider qui veut detronner notre IA dans un duel epique
//int genes_candidat[]={39, 85, 8 ,10 ,29 ,51, 43 ,10, 39 ,11};
for(int p=0; p<NB_MAX_GENES;p++ ){
  Magnus.gene[p]= genes_meilleurs[p];// c2.gene[p]= genes_candidat[p];
}

affrontement_IA_avec_affichage(&Magnus, &parents.membres[parents.best],4); affrontement_IA_avec_affichage(&parents.membres[parents.best], &Magnus,4);
printf("Affrontement dela boss contre la meilleure IA de la gen\n");
printf("Ancien boss:");afficheCircuit(Magnus);
printf("Challenger:");afficheCircuit(parents.membres[parents.best]);
    
free(population.membres);
free(parents.membres);
return 0;
}


//Pleins de pgmes, bilan: ça fait s'affronter les ia, mtn faut un moyen de changer minimax/evaluation2 en fonction de l'ia qui joue

/*


Ce code fait bien des generations avec parametres qui evoluent mais tendents tous vers le même modèle qui ne fait pas trembler Magnus.
Il faut upgrade les tournois d'IAs pour savoir qui vraiment est le meilleur


Il faut faire jouer les ias pleins de fois contre la meilleur precedente (le parent d'incice 0, normalement il est conserve)
Si une IA bat le boss, elle devient le boss


on a le dernier parametre cense dire quand fin de jeu devient trop eleve et donc les premiers coefficients ne sont pas utilises par l'ia qui joue pareil dans tte la partie
l'elo n'est pas tout a fait ajuste car certaines ias sont sollicitees pour jouer d'autres parties, et leur changement d'elo n'est pas pris en compte

On a quand même la pluart dess IAs qui ont pour modele w1=88 w2=23 w3=38 et à chaque fois la variable qui differentie la finale du debut de partie est mise à plus de 64 (l'ia ne change pas sa strategie selon le nombre
de coups joues)

*/


/*
Les winners: 18 43 89 93 6 2 40 a mis 2-0 a Magnus 
32 0 20 38 21 62 26 -> 1.5 - 0.5 a Magnus

*/