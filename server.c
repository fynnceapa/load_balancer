/* Copyright 2023 <> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "server.h"

#define DIE(assertion, call_description)				\
	do {								                \
		if (assertion) {					            \
			fprintf(stderr, "(%s, %d): ",			    \
					__FILE__, __LINE__);		        \
			perror(call_description);			        \
			exit(errno);				                \
		}							                    \
	} while (0)

unsigned int hash_function_key(void *a);
int compare_function_strings(void *a, void *b);


linked_list_t *ll_create(unsigned int data_size)
{
	linked_list_t* ll;

	ll = malloc(sizeof(*ll));

	ll->head = NULL;
	ll->data_size = data_size;
	ll->size = 0;

	return ll;
}

/*
 * Pe baza datelor trimise prin pointerul new_data, se creeaza un nou nod care e
 * adaugat pe pozitia n a listei reprezentata de pointerul list. Pozitiile din
 * lista sunt indexate incepand cu 0 (i.e. primul nod din lista se afla pe
 * pozitia n=0). Daca n >= nr_noduri, noul nod se adauga la finalul listei. Daca
 * n < 0, eroare.
 */
void ll_add_nth_node(linked_list_t* list, unsigned int n, const void* new_data)
{
	ll_node_t *prev, *curr;
	ll_node_t* new_node;

	if (!list) {
		return;
	}

	/* n >= list->size inseamna adaugarea unui nou nod la finalul listei. */
	if (n > list->size) {
		n = list->size;
	}

	curr = list->head;
	prev = NULL;
	while (n > 0) {
		prev = curr;
		curr = curr->next;
		--n;
	}

	new_node = malloc(sizeof(*new_node));
	new_node->data = malloc(list->data_size);
	memcpy(new_node->data, new_data, list->data_size);

	new_node->next = curr;
	if (prev == NULL) {
		/* Adica n == 0. */
		list->head = new_node;
	} else {
		prev->next = new_node;
	}

	list->size++;
}

/*
 * Elimina nodul de pe pozitia n din lista al carei pointer este trimis ca
 * parametru. Pozitiile din lista se indexeaza de la 0 (i.e. primul nod din
 * lista se afla pe pozitia n=0). Daca n >= nr_noduri - 1, se elimina nodul de
 * la finalul listei. Daca n < 0, eroare. Functia intoarce un pointer spre acest
 * nod proaspat eliminat din lista. Este responsabilitatea apelantului sa
 * elibereze memoria acestui nod.
 */
ll_node_t *ll_remove_nth_node(linked_list_t* list, unsigned int n)
{
	ll_node_t *prev, *curr;

	if (!list || !list->head) {
		return NULL;
	}

	/* n >= list->size - 1 inseamna eliminarea nodului de la finalul listei. */
	if (n > list->size - 1) {
		n = list->size - 1;
	}

	curr = list->head;
	prev = NULL;
	while (n > 0) {
		prev = curr;
		curr = curr->next;
		--n;
	}

	if (prev == NULL) {
		/* Adica n == 0. */
		list->head = curr->next;
	} else {
		prev->next = curr->next;
	}

	list->size--;

	return curr;
}

/*
 * Functia intoarce numarul de noduri din lista al carei pointer este trimis ca
 * parametru.
 */
unsigned int ll_get_size(linked_list_t* list)
{
	 if (!list) {
		return -1;
	}

	return list->size;
}

/*
 * Procedura elibereaza memoria folosita de toate nodurile din lista, iar la
 * sfarsit, elibereaza memoria folosita de structura lista si actualizeaza la
 * NULL valoarea pointerului la care pointeaza argumentul (argumentul este un
 * pointer la un pointer).
 */
void ll_free(linked_list_t** list)
{
	ll_node_t *node;

	if (!list || !*list)
		return;

	while ((*list)->size) {
		node = ll_remove_nth_node((*list), 0);
		free(node->data);
		free(node);
	}

	free(*list);
	*list = NULL;
}

/*
 * Atentie! Aceasta functie poate fi apelata doar pe liste ale caror noduri STIM
 * ca stocheaza int-uri. Functia afiseaza toate valorile int stocate in nodurile
 * din lista inlantuita separate printr-un spatiu.
 */
void ll_print_int(linked_list_t* list)
{
	ll_node_t* curr;

	if (!list) {
		return;
	}

	curr = list->head;
	while (curr != NULL) {
		printf("%d ", *((int*)curr->data));
		curr = curr->next;
	}

	printf("\n");
}

/*
 * Atentie! Aceasta functie poate fi apelata doar pe liste ale caror noduri STIM
 * ca stocheaza string-uri. Functia afiseaza toate string-urile stocate in
 * nodurile din lista inlantuita, separate printr-un spatiu.
 */
void ll_print_string(linked_list_t* list)
{
	ll_node_t* curr;

	if (!list) {
		return;
	}

	curr = list->head;
	while (curr != NULL) {
		printf("%s ", (char*)curr->data);
		curr = curr->next;
	}

	printf("\n");
}

/*
 * Functii de comparare a cheilor:
 */
int compare_function_ints(void *a, void *b)
{
	int int_a = *((int *)a);
	int int_b = *((int *)b);

	if (int_a == int_b) {
		return 0;
	} else if (int_a < int_b) {
		return -1;
	} else {
		return 1;
	}
}

int compare_function_strings(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

/*
 * Functii de hashing:
 */
unsigned int hash_function_int(void *a)
{
	/*
	 * Credits: https://stackoverflow.com/a/12996028/7883884
	 */
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int hash_function_string(void *a)
{
	/*
	 * Credits: http://www.cse.yorku.ca/~oz/hash.html
	 */
	unsigned char *puchar_a = (unsigned char*) a;
	unsigned long hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c; /* hash * 33 + c */

	return hash;
}
int compute_index(hashtable_t *ht, void *key) {
	return ht->hash_function(key) % ht->hmax;
}
/*
 * Functie apelata pentru a elibera memoria ocupata de cheia si valoarea unei
 * perechi din hashtable. Daca cheia sau valoarea contin tipuri de date complexe
 * aveti grija sa eliberati memoria luand in considerare acest aspect.
 */
void key_val_free_function(void *data) {
	info *pair = (info*)data;
	free(pair->key);
	pair->key = NULL;
	free(pair->value);
	pair->value = NULL;
	free(pair);
}

/*
 * Functie apelata dupa alocarea unui hashtable pentru a-l initializa.
 * Trebuie alocate si initializate si listele inlantuite.
 */
hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
		int (*compare_function)(void*, void*),
		void (*key_val_free_function)(void*))
{
	hashtable_t* hashtable = malloc(sizeof(hashtable_t));
	DIE(!hashtable, "Malloc Fail");

	hashtable->buckets = malloc(hmax * sizeof(linked_list_t *));
	DIE(!hashtable->buckets, "Malloc fail");

	for (unsigned int i = 0; i < hmax; i++) {
		hashtable->buckets[i] = ll_create(sizeof(info));
	}

	hashtable->size = 0;
	hashtable->hmax = hmax;
	hashtable->compare_function = compare_function;
	hashtable->hash_function = hash_function;
	hashtable->key_val_free_function = key_val_free_function;

	return hashtable;
}

/*
 * Functie care intoarce:
 * 1, daca pentru cheia key a fost asociata anterior o valoare in hashtable
 * folosind functia put;
 * 0, altfel.
 */
int ht_has_key(hashtable_t *ht, void *key)
{
	int index = ht->hash_function(key) % ht->hmax;
	linked_list_t *bucket = ht->buckets[index];
	ll_node_t *current = bucket->head;
	for (unsigned int i = 0; i < bucket->size; i++) {
		if (ht->compare_function(key, ((info *)current->data)->key) == 0) {
			return 1;
		}
		current = current->next;
	}
	return 0;
}

void *ht_get(hashtable_t *ht, void *key)
{
	int index = ht->hash_function(key) % ht->hmax;
	linked_list_t *bucket = ht->buckets[index];
	ll_node_t *current = bucket->head;
	for (unsigned int i = 0; i < bucket->size; i++) {
		if (ht->compare_function(key, ((info *)current->data)->key) == 0) {
			return ((info *)current->data)->value;
		}
		current = current->next;
	}
	return NULL;
}

/*
 * Atentie! Desi cheia este trimisa ca un void pointer (deoarece nu se impune
 * tipul ei), in momentul in care se creeaza o noua intrare in hashtable (in
 * cazul in care cheia nu se gaseste deja in ht), trebuie creata o copie a
 * valorii la care pointeaza key si adresa acestei copii trebuie salvata in
 * structura info asociata intrarii din ht. Pentru a sti cati octeti trebuie
 * alocati si copiati, folositi parametrul key_size.
 *
 * Motivatie:
 * Este nevoie sa copiem valoarea la care pointeaza key deoarece dupa un apel
 * put(ht, key_actual, value_actual), valoarea la care pointeaza key_actual
 * poate fi alterata (de ex: *key_actual++). Daca am folosi direct adresa
 * pointerului key_actual, practic s-ar modifica din afara hashtable-ului cheia
 * unei intrari din hashtable. Nu ne dorim acest lucru, fiindca exista riscul sa
 * ajungem in situatia in care nu mai stim la ce cheie este inregistrata o
 * anumita valoare.
 */
void ht_put(hashtable_t *ht, void *key, unsigned int key_size,
	void *value, unsigned int value_size)
{
	info data;
	int index = compute_index(ht, key);
	linked_list_t *bucket = ht->buckets[index];
	ll_node_t *current = bucket->head;
	for (unsigned int i = 0; i < bucket->size; i++) {
		if (ht->compare_function(key, ((info *)current->data)->key) == 0) {
			break;
		}
		current = current->next;
	}
	data.key = malloc(key_size);
	DIE(!data.key, "Malloc fail");
	memcpy(data.key, key, key_size);

	data.value = malloc(value_size);
	DIE(!data.value, "Malloc fail");
	memcpy(data.value, value, value_size);

	ll_add_nth_node(bucket, 0, &data);
	ht->size++;
}

/*
 * Procedura care elimina din hashtable intrarea asociata cheii key.
 * Atentie! Trebuie avuta grija la eliberarea intregii memorii folosite pentru o
 * intrare din hashtable (adica memoria pentru copia lui key --vezi observatia
 * de la procedura put--, pentru structura info si pentru structura Node din
 * lista inlantuita).
 */
void ht_remove_entry(hashtable_t *ht, void *key)
{
	int index = compute_index(ht, key);
	linked_list_t *bucket = ht->buckets[index];
	int what_pos;
	ll_node_t *current = bucket->head;
	for (unsigned int i = 0; i < bucket->size; i++) {
		if (ht->compare_function(key, ((info *)current->data)->key) == 0) {
			what_pos = i;
			break;
		}
		current = current->next;
	}
	ll_node_t *remove = ll_remove_nth_node(bucket, what_pos);
	ht->key_val_free_function(remove->data);
	free(remove);
	ht->size--;
}

info **ht_get_all_data(hashtable_t *ht, unsigned int *result_size) {
	unsigned int size = ht->size;
	if (size == 0) {
		return NULL;
	}
	info **result = malloc(size * sizeof(info *));
	for (unsigned int i = 0; i < size; i++)
		result[i] = NULL;
	unsigned int count = 0;
	for (unsigned int i = 0; i < ht->hmax; i++) {
		linked_list_t *bucket = ht->buckets[i];
		ll_node_t *current = bucket->head;
		while(current != NULL) {
			result[count] = malloc(sizeof(info));
			result[count]->key = malloc(strlen(((info *)current->data)->key) + 1);
			strcpy(result[count]->key, ((info *)current->data)->key);
			result[count]->value = malloc(strlen(((info *)current->data)->value) + 1);
			strcpy(result[count]->value, ((info *)current->data)->value);
			count++;
			current = current->next;
		}
	}
	*result_size = count;
	if (count == 0) {
		free(result);
		return NULL;
	}
	return result;
}

/*
 * Procedura care elibereaza memoria folosita de toate intrarile din hashtable,
 * dupa care elibereaza si memoria folosita pentru a stoca structura hashtable.
 */

void ht_free(hashtable_t *ht)
{
	ll_node_t *current;
	for (unsigned int i = 0; i < ht->hmax; i++) {
		if (ht->buckets[i]->head != NULL) {
			current = ht->buckets[i]->head;
			while (current != NULL) {
				ht->key_val_free_function(current->data);
				current = current->next;
			}
		}
		ll_free(&ht->buckets[i]);
	}
	free(ht->buckets);
	free(ht);
}

unsigned int ht_get_size(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->size;
}

unsigned int ht_get_hmax(hashtable_t *ht)
{
	if (ht == NULL)
		return 0;

	return ht->hmax;
}


server_memory *init_server_memory()
{
	server_memory *server = malloc(sizeof(server_memory));
	server->hashtable = ht_create(HMAX, hash_function_key,
						compare_function_strings, key_val_free_function);
	server->original_server = 0;
	server->server_id = 0;
	return server;
}

void server_store(server_memory *server, char *key, char *value) {
	ht_put(server->hashtable, key, strlen(key) + 1, value, strlen(value) + 1);
}

char *server_retrieve(server_memory *server, char *key) {
	char *value;
	value = ht_get(server->hashtable, key);
	return value;
}

void server_remove(server_memory *server, char *key) {
	ht_remove_entry(server->hashtable, key);
}

void free_server_memory(server_memory *server) {
	ll_node_t *current;
	hashtable_t *ht = server->hashtable;
	for (unsigned int i = 0; i < ht->hmax; i++) {
		if (ht->buckets[i]->head != NULL) {
			current = ht->buckets[i]->head;
			while (current != NULL) {
				free(((info *)current->data)->value);
				free(((info *)current->data)->key);
				current = current->next;
			}
		}
		ll_free(&ht->buckets[i]);
	}
	free(ht->buckets);
	free(ht);
	free(server);
	server = NULL;
}
