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
	printf("Test 3\n"
			"| ZO | ZO | ZO | ZO | ---> | ZL : ZL : ZL : ZL |"
 		"\n");
	printf("Initializing memory\n");
    printf("On utilise une mémoire de taille 256 bytes\n\n");
    
    printf("\033[05;41m>\033[00m \033[01mIci, nous allons observer la fusion de l'apparition de zones\n\
libres depuis la libération de zones occupées ...\033[00m\n");
    
	mem_init(get_memory_adr(), (size_t)256);
    mem_show(afficher_zone);
    printf("\n");
    ptr = mem_alloc((size_t)50);
    printf("Memoire allouee en %d, de taille utilisateur 50\n", (int) (ptr-get_memory_adr()));
    ptr = mem_alloc((size_t)50);
    printf("Memoire allouee en %d, de taille utilisateur 50\n", (int) (ptr-get_memory_adr()));
    ptr = mem_alloc((size_t)50);
    printf("Memoire allouee en %d, de taille utilisateur 50\n", (int) (ptr-get_memory_adr()));
    ptr = mem_alloc((size_t)32);
    printf("Memoire allouee en %d, de taille utilisateur 32\n", (int) (ptr-get_memory_adr()));
    mem_show(afficher_zone);
    printf("\n");
    
    mem_free(get_memory_adr()+216);
    printf("Mémoire libérée en 216\n");
    mem_show(afficher_zone);
    printf("\n");
    mem_free(get_memory_adr()+24);
    printf("Mémoire libérée en 24\n");
    mem_show(afficher_zone);
    printf("\n");
    mem_free(get_memory_adr()+152);
    printf("Mémoire libérée en 152\n");
    mem_show(afficher_zone);
    printf("\n");
    mem_free(get_memory_adr()+88);
    printf("Mémoire libérée en 88\n");
    mem_show(afficher_zone);
	// TEST OK
	return 0;
}