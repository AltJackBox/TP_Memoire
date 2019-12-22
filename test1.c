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
			"| ZL | ZO | ---> | ZL : ZL |"
 		"\n");
	printf("Initializing memory\n");
    printf("On utilise une mémoire de taille 256 bytes\n");
    
    
	mem_init(get_memory_adr(), (size_t)256);
    mem_show(afficher_zone);
    printf("\n");
    ptr = mem_alloc((size_t)108);
    printf("Memoire allouee en %d, de taille utilisateur 108\n", (int) (ptr-get_memory_adr()));
    ptr = mem_alloc((size_t)104);
    printf("Memoire allouee en %d, de taille utilisateur 104\n", (int) (ptr-get_memory_adr()));
    mem_show(afficher_zone);
    printf("\n");
    
    mem_free(get_memory_adr()+24);
    printf("Mémoire libérée en 24\n");
    mem_show(afficher_zone);
    printf("\n");
    mem_free(get_memory_adr()+144);
    printf("Mémoire libérée en 144\n");
    mem_show(afficher_zone);
	// TEST OK
	return 0;
}