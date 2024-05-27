/* Copyright 2003   R. Civalero   L.G.P.L.
   Modifi� par Franck Rousseau et Simon Nieuviarts
   Inspir� du fichier list.h de Linux

	queue.h : Gestion de files g�n�riques avec priorit�
			  A priorit� �gale, comportement FIFO.

	Elle est impl�ment�e par une liste circulaire doublement
	chain�e. La file est tri�e par ordre croissant de priorit�
	lors de l'ajout. Donc l'�l�ment prioritaire qui doit sortir
	en premier est le dernier de la liste, c'est � dire l'�l�ment
	pr�c�dent la t�te de liste.

	Pour cr�er une file :
	 1) Cr�er une t�te de liste de type 'link',
	 2) L'initialiser avec LIST_HEAD_INIT,
	 3) APRES les avoir cr�es, ajouter des �lements dans la file
		avec la macro queue_add,
	Les �l�ments doivent �tre des structures contenant au moins un
	champ de type 'link' et un champ de type 'int' pour la priorit�.

	Pour utiliser la file :
	 - Supprimer des �l�ments particulier avec la macro queue_del,
	 - R�cuperer et enlever de la file l'�lement prioritaire avec
	   la macro queue_out,
	 - Ajouter d'autre �l�ments avec la macro queue_add,
	 - Tester si la file est vide avec la fonction queue_empty

	Attention : certains pointeurs pointent vers des �l�ments des
				files, alors que d'autres pointent vers le champ
			du lien de chainage de ces �l�ments.
			Les t�tes de file/liste sont des liens de chainage
			de type 'link' et non des �l�ments.
*/

#ifndef QUEUE_H
#define QUEUE_H

#include "debug.h"

/**
 * Type structure pour faire les liens de chainage
 */
typedef struct list_link
{
	struct list_link *prev;
	struct list_link *next;
} link;

/**
 * Initialisation d'une t�te de liste (t�te de file).
 */
#define LIST_HEAD_INIT(name) \
	{                        \
		&(name), &(name)     \
	}
#define LIST_HEAD(name) struct list_link name = LIST_HEAD_INIT(name)
#define INIT_LIST_HEAD(ptr)            \
	do                                 \
	{                                  \
		struct list_link *__l = (ptr); \
		__l->next = __l;               \
		__l->prev = __l;               \
	} while (0)

/**
 * Initialisation d'un maillon de liste.
 *
 * Si vous pensez en avoir besoin, il est fort probable que ce soit une erreur.
 * Les zones m�moires donn�es par l'allocateur ou allou�es � la compilation
 * sont initialis�es � 0 (respectivement par l'allocateur et par le crt0).
 */
#define INIT_LINK(ptr)                 \
	do                                 \
	{                                  \
		struct list_link *__l = (ptr); \
		__l->next = 0;                 \
		__l->prev = 0;                 \
	} while (0)

/**
 * Ajout d'un �l�ment dans la file avec tri par priorit�
 * ptr_elem  : pointeur vers l'�l�ment � chainer
 * head      : pointeur vers la t�te de liste
 * type      : type de l'�l�ment � ajouter
 * listfield : nom du champ du lien de chainage
 * priofield : nom du champ de la priorit�
 */
#define queue_add(ptr_elem, head, type, listfield, priofield)                                      \
	do                                                                                             \
	{                                                                                              \
		link *__cur_link = head;                                                                   \
		type *__elem = (ptr_elem);                                                                 \
		link *__elem_link = &((__elem)->listfield);                                                \
		assert((__elem_link->prev == 0) && (__elem_link->next == 0));                              \
		do                                                                                         \
			__cur_link = __cur_link->next;                                                         \
		while ((__cur_link != head) &&                                                             \
			   (((queue_entry(__cur_link, type, listfield))->priofield) < ((__elem)->priofield))); \
		__elem_link->next = __cur_link;                                                            \
		__elem_link->prev = __cur_link->prev;                                                      \
		__cur_link->prev->next = __elem_link;                                                      \
		__cur_link->prev = __elem_link;                                                            \
	} while (0)

/**
 * Macro � usage interne utilis�e par la macro queue_add
 * R�cup�ration du pointeur vers l'objet correspondant
 *   (On calcule la diff�rence entre l'adresse d'un �l�ment et l'adresse
 *   de son champ de type 'link' contenant les liens de chainage)
 * ptr_link  : pointeur vers le maillon
 * type      : type de l'�l�ment � r�cup�rer
 * listfield : nom du champ du lien de chainage
 */
#define queue_entry(ptr_link, type, listfield) \
	((type *)((char *)(ptr_link) - (unsigned long)(&((type *)0)->listfield)))

/**
 * Tester si une file est vide
 * head : pointeur vers la t�te de liste
 * retourne un entier (0 si pas vide)
 */
static __inline__ int queue_empty(link *head)
{
	return (head->next == head);
}

/**
 * Tester si un lien de file est non-initialisé
 * link : pointeur vers le lien
 * retourne un entier (0 si non-initialisé)
 */
static __inline__ int queue_initialized(link *link)
{
	return (link->next == 0 && link->prev == 0);
}

/**
 * Retrait de l'�l�ment prioritaire de la file
 * head      : pointeur vers la t�te de liste
 * type      : type de l'�l�ment � retourner par r�f�rence
 * listfield : nom du champ du lien de chainage
 * retourne un pointeur de type 'type' vers l'�l�ment sortant
 */
#define queue_out(head, type, listfield) \
	(type *)__queue_out(head, (unsigned long)(&((type *)0)->listfield))

/**
 * Fonction � usage interne utilis�e par la macro ci-dessus
 * head : pointeur vers la t�te de liste
 * diff : diff�rence entre l'adresse d'un �l�ment et son champ de
 *        type 'link' (cf macro list_entry)
 */
static __inline__ void *__queue_out(link *head, unsigned long diff)
{
	// On r�cup�re un pointeur vers le maillon
	// du dernier �l�ment de la file.
	unsigned long ptr_link_ret = (unsigned long)(head->prev);

	// Si la file est vide, on retourne le pointeur NULL.
	if (queue_empty(head))
		return ((void *)0);

	// Sinon on retire l'�l�ment de la liste,
	head->prev = head->prev->prev;
	head->prev->next = head;

	((link *)ptr_link_ret)->prev = 0;
	((link *)ptr_link_ret)->next = 0;

	// Et on retourne un pointeur vers cet �l�ment.
	return ((void *)(ptr_link_ret - diff));
}

/**
 * Suppression d'un �l�ment dans la file
 * ptr_elem  : pointeur vers l'�l�ment � supprimer
 * listfield : nom du champ du lien de chainage
 */
#define queue_del(ptr_elem, listfield)                                \
	do                                                                \
	{                                                                 \
		link *__elem_link = &((ptr_elem)->listfield);                 \
		assert((__elem_link->prev != 0) && (__elem_link->next != 0)); \
		__elem_link->prev->next = __elem_link->next;                  \
		__elem_link->next->prev = __elem_link->prev;                  \
		__elem_link->next = 0;                                        \
		__elem_link->prev = 0;                                        \
	} while (0)

/**
 * Parcours d'une file
 * ptr_elem  : pointeur vers un �l�ment utilis� comme it�rateur de boucle
 * head      : pointeur vers la t�te de liste
 * type      : type des �l�ments de la liste
 * listfield : nom du champ du lien de chainage
 */
#define queue_for_each(ptr_elem, head, type, listfield)         \
	for (ptr_elem = queue_entry((head)->next, type, listfield); \
		 &ptr_elem->listfield != (head);                        \
		 ptr_elem = queue_entry(ptr_elem->listfield.next, type, listfield))

/**
 * Parcours d'une file en sens inverse
 * ptr_elem  : pointeur vers un �l�ment utilis� comme it�rateur de boucle
 * head      : pointeur vers la t�te de liste
 * type      : type des �l�ments de la liste
 * listfield : nom du champ du lien de chainage
 */
#define queue_for_each_prev(ptr_elem, head, type, listfield)    \
	for (ptr_elem = queue_entry((head)->prev, type, listfield); \
		 &ptr_elem->listfield != (head);                        \
		 ptr_elem = queue_entry(ptr_elem->listfield.prev, type, listfield))

/**
 * Recuperer un pointeur vers l'element prioritaire de la file
 * sans l'enlever de la file.
 * head : pointeur vers la t�te de liste
 * type : type de l'�l�ment � retourner par r�f�rence
 * listfield : nom du champ du lien de chainage
 * retourne un pointeur de type 'type' vers l'�l�ment prioritaire
 */
#define queue_top(head, type, listfield) \
	(type *)__queue_top(head, (unsigned long)(&((type *)0)->listfield))

/**
 * Fonction � usage interne utilis�e par la macro ci-dessus
 * head : pointeur vers la t�te de liste
 * diff : diff�rence entre l'adresse d'un �l�ment et son champ de
 * type 'link' (cf macro list_entry)
 */
static __inline__ void *__queue_top(link *head, unsigned long diff)
{
	// On r�cup�re un pointeur vers le maillon
	// du dernier �l�ment de la file.
	unsigned long ptr_link_ret = (unsigned long)(head->prev);

	// Si la file est vide, on retourne le pointeur NULL.
	if (queue_empty(head))
		return ((void *)0);

	// Sinon retourne un pointeur vers cet �l�ment.
	return ((void *)(ptr_link_ret - diff));
}

/**
 * Recuperer un pointeur vers l'element le moins prioritaire de la file
 * sans l'enlever de la file.
 * head : pointeur vers la t�te de liste
 * type : type de l'�l�ment � retourner par r�f�rence
 * listfield : nom du champ du lien de chainage
 * retourne un pointeur de type 'type' vers l'�l�ment prioritaire
 */
#define queue_bottom(head, type, listfield) \
	(type *)__queue_bottom(head, (unsigned long)(&((type *)0)->listfield))

/**
 * Fonction � usage interne utilis�e par la macro ci-dessus
 * head : pointeur vers la t�te de liste
 * diff : diff�rence entre l'adresse d'un �l�ment et son champ de
 * type 'link' (cf macro list_entry)
 */
static __inline__ void *__queue_bottom(link *head, unsigned long diff)
{
	// On recupere un pointeur vers le maillon
	// du premier element de la file.
	unsigned long ptr_link_ret = (unsigned long)(head->next);

	// Si la file est vide, on retourne le pointeur NULL.
	if (queue_empty(head))
		return ((void *)0);

	// Sinon retourne un pointeur vers cet element.
	return ((void *)(ptr_link_ret - diff));
}

#define LIST_HEAD_INIT_PROC(x) assert(!"utiliser INIT_LIST_HEAD au lieu de LIST_HEAD_INIT_PROC")

#endif
