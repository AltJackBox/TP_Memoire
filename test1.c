#include "mem.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void afficher_zone(void *adresse, size_t taille, int free)
{
  printf("Zone %s, Adresse : %lu, Taille : %lu\n", free?"libre":"occupee",
         adresse - get_memory_adr(), (unsigned long) taille);
}


int main(int argc, char *argv[]) {
    void *ptr;
    printf("\n");
	printf("Test 1\n"
			"| ZL : ZL | ---> | ZO | ZO |"
 		"\n");
	printf("Initializing memory\n");
    printf("On utilise une mémoire de taille 256 bytes\n");
    
    
	mem_init(get_memory_adr(), (size_t)256);
    mem_show(afficher_zone);
    printf("\n");
    ptr = mem_alloc((size_t)100);
    if (ptr == NULL) {
        fprintf(stderr,"Problème lors de l'allocation !\n");
        exit(-1);
    }
    printf("Memoire allouee en %d, de taille utilisateur 100\n", (int) (ptr-get_memory_adr()));
    mem_show(afficher_zone);
    printf("\n");
    
    ptr = mem_alloc((size_t)100);
    if (ptr == NULL) {
        fprintf(stderr,"Problème lors de l'allocation !\n");
        exit(-1);
    }
    printf("Memoire allouee en %d, de taille utilisateur 100\n", (int) (ptr-get_memory_adr()));

    printf("\033[05;41m>\033[00m \033[01mIci on devrait observer qu'une zone occupée est plus grosse qu'une autre.\n\
Car l'allocateur sait que la potentielle place restante ne suffirait pas pour les\n\
méta-données d'une zone libre : \033[00m\n");
    mem_show(afficher_zone);
    
	// TEST OK
	return 0;
}