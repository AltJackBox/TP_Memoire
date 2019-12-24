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
	printf("Test 2\n"
			"| ZL : ZL | ---> | ZO | ZL | + tentative de débordement"
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
    printf("Memoire allouee en %d, de taille utilisateur 108\n", (int) (ptr-get_memory_adr()));
    mem_show(afficher_zone);
    printf("\n");

    printf("\033[05;41m>\033[00m \033[01mIci nous allons tenter d'allouer 113 octets sachant que seulement\n\
120 octets sont disponibles. Or, 113+8 (la taille des méta-données d'une ZO)\n\
est égal à 121, soit 1 octet de trop pour allouer la zone et l'alligner...\033[00m\n");
    ptr = mem_alloc((size_t)113);
    if (ptr == NULL) {
        fprintf(stderr,"Problème lors de l'allocation !\n");
        exit(-1);
    }
    printf("Memoire allouee en %d, de taille utilisateur 104\n", (int) (ptr-get_memory_adr()));
    
    mem_show(afficher_zone);
    printf("\n");

	// TEST OK
	return 0;
}