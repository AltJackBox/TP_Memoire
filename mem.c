#include "mem.h"
#include "common.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

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

/*Alloue une zone dans la mémoire selon la taille donnée par l'utilisateur*/

void *mem_alloc(size_t taille_utilisateur) {

	struct fb* ancienne_zl = memory_addr->fit(memory_addr->tete,taille_utilisateur + sizeof(struct zo));
	if (ancienne_zl == NULL) return NULL;
	struct zo* zone_allouee;
	struct fb* nouv_first_block;
	void* nouvelle_zl;
	void* adr_fin = memory_addr + memory_addr->memory_size;
	
	/*La zone qui est allouée correspond à la zone libre trouvée avec la fonction fit*/
	zone_allouee = (void *)ancienne_zl;
	/*La taille finale de la zone allouée sera supérieure à celle donnée par l'utilisateur car on prend en compte la taille de la structure zo*/
	zone_allouee->size = (size_t)allign((void *)(taille_utilisateur + sizeof(struct zo)),8);

	nouvelle_zl = allign((void *)ancienne_zl + zone_allouee->size,8);

	size_t taille_zl = (size_t)allign((void *)get_system_memory_size() - (nouvelle_zl - (void *)memory_addr),8);
	
	/*On gère le cas ou on n'a plus la place de créer une nouvelle zone libre*/
	/*Cela signifie que la zone que nous avons précédemment allouée doit être plus grande, on l'additionne avec la taille de l'ancienne zl)*/
	if ((nouvelle_zl > adr_fin) || (taille_zl < (size_t)allign((void *)(sizeof(struct zo) + 1),8))){ 
		memory_addr->tete = NULL;
		size_t za_ancienne_taille = zone_allouee->size;
		zone_allouee->size = (size_t)allign((void *)(za_ancienne_taille + taille_zl), 8);
	}else{
		nouv_first_block = nouvelle_zl;
		nouv_first_block->size = taille_zl;
		nouv_first_block->next = NULL;

		memory_addr->tete = nouv_first_block;
	}
	return (void *)zone_allouee;
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
