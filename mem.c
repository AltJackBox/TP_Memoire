#include "mem.h"
#include "common.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

// constante définie dans gcc seulement
#ifdef __BIGGEST_ALIGNMENT__
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
#define ALIGNMENT 16
#endif

/* structure placée au début de la zone de l'allocateur

   Elle contient toutes les variables globales nécessaires au
   fonctionnement de l'allocateur

   Contient : 
		la taille de la zone mémoire à gérer
		une référence vers la première zone libre
		une reférence vers la fonction de recherche de zone libre
*/
struct allocator_header {
    size_t memory_size; 
	struct fb *tete;
	mem_fit_function_t *fit;
};

/* La seule variable globale autorisée
 * On trouve à cette adresse le début de la zone à gérer
 * (et une structure 'struct allocator_header)
 */
static struct allocator_header* memory_addr; 

static inline void *get_system_memory_addr() {
	return memory_addr;
}

static inline struct allocator_header *get_header() {
	struct allocator_header *h;
	h = get_system_memory_addr();
	return h;
}

static inline size_t get_system_memory_size() {
	return get_header()->memory_size;
}

/*Zone libre*/
struct fb {
	size_t size;
	struct fb* next;
};

/* Zone occupée*/
struct zo { 
	size_t size;
};

/*Permet de fournir des adresses et des tailles multiples de 8, car on gère des octets dans notre allocateur*/
void *allign(void* addr,size_t taille){
	int modulo = (intptr_t) addr % taille;
	if (modulo == 0) return addr;
	else return addr + taille - modulo;
}


/*Initialise l'allocateur mémoire, avec mem comme adresse de début d'allocateur et taille pour la taille totale de l'allocateur*/
void mem_init(void* mem, size_t taille)
{	
	
    memory_addr = mem;
    memory_addr->memory_size = taille;
	assert(mem == get_system_memory_addr());
	assert(taille == get_system_memory_size());

	/*Première zone libre, après la structure allocateur header*/
	void* adr_depart = allign(mem + sizeof(struct allocator_header),8);
	struct fb* first_block = (struct fb*)(adr_depart);
	first_block->size = get_system_memory_size() - (size_t)((void *)(first_block) - mem);
	first_block->next = NULL;

	memory_addr->tete = first_block;
	memory_addr->fit = &mem_fit_first;
}

/*Parcourt l'ensemble des blocs gerés par l'allocateur et appelle la fonction print en paramètre*/
void mem_show(void (*print)(void *, size_t, int)) {
	/*Premiere zone libre*/
	struct fb* adr_prochaine_zl = memory_addr->tete;
	void *adr_arrivee = (void *)memory_addr + memory_addr->memory_size; 
	void *adr_actuelle = allign((void *)memory_addr + sizeof(struct allocator_header),8); 

	while (adr_actuelle < adr_arrivee) {
		/*On vérifie que l'adresse est égale à celle d'une zone libre, et si c'est le cas on l'affiche*/
		if (adr_actuelle == adr_prochaine_zl){
			print(adr_actuelle,adr_prochaine_zl->size,1);
			adr_actuelle = allign(adr_actuelle + adr_prochaine_zl->size,8);
			adr_prochaine_zl = adr_prochaine_zl->next;
		/*Sinon, c'est une zone occupée*/
		}else{
			struct zo* zone_occuppee = (struct zo*)(adr_actuelle);
			print(adr_actuelle,zone_occuppee->size,0);
			adr_actuelle = allign(adr_actuelle + zone_occuppee->size,8);
		}
	}
}

/*Remplace la fonction de l'allocator_header par celle donnée en argument*/
void mem_fit(mem_fit_function_t *f) {
	get_header()->fit = f;
}

//fonction pour mem_alloc qui gère le chaînage des zones_libres depuis le header
void changer_next(struct fb* old_addr, struct fb* new_addr) {
	struct allocator_header* head = get_system_memory_addr();
	if (head->tete == old_addr) {
		head->tete = new_addr;
	} else {
		struct fb* free_block = head->tete;
		while(free_block != NULL && free_block->next != old_addr) {
			free_block = free_block->next;
		}
		if (free_block != NULL && free_block->next == old_addr) {
			free_block->next = new_addr;
		} else {
			/*UNE ERREUR S'EST PRODUITE (cas normalement impossible)*/
			fprintf(stderr, "Quelque chose cloche avec la liste chaînée des ZL ...\n");
			exit(-1);
		}
	}
}

/*Alloue une zone dans la mémoire selon la taille donnée par l'utilisateur*/
void *mem_alloc(size_t taille_utilisateur) {
	struct allocator_header* mem = get_system_memory_addr();
	struct fb* firstfb = mem->tete;
	size_t alloc_size = taille_utilisateur + sizeof(struct zo); //taille de la zone à allouer
	void* fb = (void*)mem->fit(firstfb, alloc_size);
	if (fb == NULL) return NULL; //pas de bloc dispos, donc impossible.

	/** Une zone mémoire d'adresse fb est disponible **/
	void* new_potential_fb_addr = allign(fb + alloc_size, 8);
	alloc_size = (size_t)(new_potential_fb_addr - fb); //on réajuste alloc size après l'alignement.
	
	//On calcule le Δ restant potentiel après l'alloc
	size_t delta = (fb + ((struct fb*)fb)->size) - new_potential_fb_addr;
	/*Si Δ < sizeof(struct fb), on a un problème,
	 il faut donc pousser la taille de la futur ZO pour remplacer
	 la ZL, il faut aussi gérer les pointeurs des zones libres.*/
	if (delta < sizeof(struct fb)) {
		alloc_size = ((struct fb*)fb)->size; //pas besoin de s'ennuyr avec l'alignement ici.
		changer_next(fb, ((struct fb*)fb)->next);
	} else {
		/*Sinon, on réajuste le fb renvoyer par la fonction de fit pour la réduire en
		fonction de la nouvelle zone occupée*/
		//On déplace le contenu de la ZL a son nouvel emplacement
		((struct fb*)new_potential_fb_addr)->size = delta;
		((struct fb*)new_potential_fb_addr)->next = ((struct fb*)fb)->next;
		//Puis on met à jour le pointeur de la ZL/Header concernant cette zone libre
		changer_next(fb, new_potential_fb_addr);
	}
	
	//On créer une nouvelle zone occupée de size alloc_size à l'adresse de la ZL d'addr fb
	struct zo* ob = (struct zo*) fb;
	ob->size = alloc_size;
	//Et on retourne l'addr de la nouvelle zone occupée
	return ob;
}


void mem_free(void* mem) {
	mem = allign(mem,8);

	struct zo* zone_free = (struct zo*)mem;
	/*Adresse de fin de la zone à libérer, utile pour savoir s'il y a présence d'une zone libre "juste après"*/
	void* fin_zone_free = allign((void *)(mem + zone_free->size),8);

	struct fb* zl = get_header()->tete;
	/*Adresse de fin de la zone libre, utile pour savoir s'il y a présence d'une zone libre "juste avant" la zone à libérer*/
	void * fin_zl_prev;
	
	struct fb* nouvelle_zl;

	struct fb* zl_prev=NULL;
	struct fb* zl_suiv=NULL;


	/*On regarde s'il y a présence de zones libres avant ou après la zone à libérer*/

	while ((zl != NULL) && (zl_prev == NULL) && (zl_suiv == NULL)){
		/*Si on est dans ce cas là, la zone est déjà libre, donc on sort de la fonction*/
		if ((void *)zl == (void *)zone_free) return;
		if (((void *)zl < (void *)zone_free) && ((void *)(zl->next) > (void *)zone_free)){
			zl_prev = zl;
			zl_suiv = zl->next;
		}
		else if ((void *)zl > (void *)zone_free){
			zl_suiv = zl;
		}
		else if (((void *)zl < (void *)zone_free) && (zl->next == NULL)){
			zl_prev = zl;
		}
		zl=zl->next;
	}

	/*On calcule la fin de la zone libre qui précède la zone à libérer, pour gérer les cas de fusion*/
	if (zl_prev != NULL) fin_zl_prev = allign((void *)(zl_prev) + zl_prev->size,8);

	/*Dans les 3 premiers cas, on doit fusionner d'anciennes zl avec la zone liberée*/

	/* ZL_prev | ZF | ZL_suiv    ZL :  ZL  : ZL */
	if ((fin_zl_prev == (void *)zone_free) && (zl_suiv == fin_zone_free)){
		nouvelle_zl = zl_prev;
		nouvelle_zl->size = zl_prev->size + zone_free->size + zl_suiv->size;
		nouvelle_zl->next = zl_suiv->next;
	}
	/* ZL_prev | ZF | ZO  ----> ZL : ZL | ZO */
	else if ((fin_zl_prev == (void *)zone_free) && (zl_suiv != fin_zone_free)){
		nouvelle_zl = zl_prev;
		nouvelle_zl->size = zl_prev->size + zone_free->size;
		nouvelle_zl ->next = zl_prev->next;
	}
	/* ZO | ZF | ZL_suiv ----> ZO | ZL : ZL */ 
	else if ((fin_zl_prev != (void *)zone_free) && (zl_suiv == fin_zone_free)){
		nouvelle_zl = (void *)zone_free;
		nouvelle_zl->size = zone_free->size + zl_suiv->size;
		nouvelle_zl->next = zl_suiv->next; 
		if (zl_prev != NULL) {
			zl_prev->next = nouvelle_zl;
		}else{
			get_header()->tete = nouvelle_zl;
		}
	}
	/*Dans ce cas, il n'y a pas de zones à fusionner, mais on doit changer les liens de chaînage dans la liste des zones libres*/
	/* ZO | ZF | ZO */
	else if ((fin_zl_prev != (void *)zone_free) && (zl_suiv != fin_zone_free)){
		nouvelle_zl = (void *)zone_free;
		nouvelle_zl->size = zone_free->size;
		if (zl_prev != NULL){
			zl_prev->next = nouvelle_zl;
		}else{
			get_header()->tete = nouvelle_zl;
		}
		if (zl_suiv != NULL){
			nouvelle_zl->next = zl_suiv;
		}else{
			nouvelle_zl->next = NULL;
		}
	}
}


struct fb* mem_fit_first(struct fb* list, size_t size) {
		while (list != NULL && (list->size < size)){
			list = list->next;
		}
		return list;
}

/* Fonction à faire dans un second temps
 * - utilisée par realloc() dans malloc_stub.c
 * - nécessaire pour remplacer l'allocateur de la libc
 * - donc nécessaire pour 'make test_ls'
 * Lire malloc_stub.c pour comprendre son utilisation
 * (ou en discuter avec l'enseignant)
 */
size_t mem_get_size(void *zone) {
	/* zone est une adresse qui a été retournée par mem_alloc() */

	/* la valeur retournée doit être la taille maximale que
	 * l'utilisateur peut utiliser dans cette zone */
	return ((struct zo*)(zone))->size;
}

/* Fonctions facultatives
 * autres stratégies d'allocation
 */
struct fb* mem_fit_best(struct fb *list, size_t size) {
	return NULL;
}

struct fb* mem_fit_worst(struct fb *list, size_t size) {
	return NULL;
}
