/* Copyright 2023 <> */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "load_balancer.h"
#include "utils.h"

unsigned int hash_function_servers(void *a) {
	unsigned int uint_a = *((unsigned int *)a);

	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = ((uint_a >> 16u) ^ uint_a) * 0x45d9f3b;
	uint_a = (uint_a >> 16u) ^ uint_a;
	return uint_a;
}

unsigned int hash_function_key(void *a) {
	unsigned char *puchar_a = (unsigned char *)a;
	uint hash = 5381;
	int c;

	while ((c = *puchar_a++))
		hash = ((hash << 5u) + hash) + c;

	return hash;
}

// sterge un server din array
static void remove_server_from_array(load_balancer *main, uint server_id) {
	int ok = 0, pos = 0;
	for (uint i = 0; i < main->servers_count; i++) {
		if (main->servers[i]->server_id == server_id) {
			ok = 1;
			pos = i;
			break;
		}
	}
	if (ok == 0)
		pos = 0;

	free_server_memory(main->servers[pos]);

	for (uint i = pos; i < main->servers_count - 1; i++) {
		main->servers[i] = main->servers[i + 1];
	}
	main->servers_count--;
	size_t size = main->servers_count * sizeof(server_memory *);
	main->servers = realloc(main->servers, size);
	DIE(main->servers == NULL, "Server realloc fail (remove_server_from_array)");
}

// adauga un server in array
static void array_add_server(load_balancer *main, uint replica, uint id) {
	main->servers_count++;
	size_t size = (main->servers_count + 1) * sizeof(server_memory *);
	main->servers = realloc(main->servers, size);
	DIE(main->servers == NULL, "Server realloc fail (array_add_server)");

	main->servers[main->servers_count - 1] = init_server_memory();
	main->servers[main->servers_count - 1]->server_id = replica;
	main->servers[main->servers_count - 1]->original_server = id;
}

load_balancer *init_load_balancer() {
	load_balancer *main = malloc(sizeof(load_balancer));
	DIE(main == NULL, "Load_balancer malloc failed");

	main->servers = malloc(sizeof(server_memory *));
	DIE(main->servers == NULL, "Servers malloc failed");

	main->servers_count = 0;
	return main;
}

void loader_add_server(load_balancer *main, int server_id) {
	uint replica0, replica1, replica2;
	replica0 = server_id;
	replica1 = 1 * 100000 + server_id;
	replica2 = 2 * 100000 + server_id;

	array_add_server(main, replica0, server_id);
	array_add_server(main, replica1, server_id);
	array_add_server(main, replica2, server_id);

	// sortare in functie de hash
	server_memory *temp;
	for (uint i = 0;  i < main->servers_count - 1; i++) {
		for (uint j = 0; j < main->servers_count - i - 1; j++) {
			uint hash1, hash2;
			hash1 = main->servers[j]->server_id;
			hash2 = main->servers[j + 1]->server_id;
			hash1 = hash_function_servers(&hash1);
			hash2 = hash_function_servers(&hash2);
			if (hash1 > hash2) {
				temp = main->servers[j];
				main->servers[j] = main->servers[j + 1];
				main->servers[j + 1] = temp;
			}
		}
	}

	// redistribuire
	for (uint i = 0; i < main->servers_count; i++) {
		uint current = main->servers[i]->server_id;
		if (current == replica0 || current == replica1 || current == replica2) {
			uint what_srv;
			// iau serverul "din fata" celui nou sau din fata unei replci
			if (i < main->servers_count - 1)
				what_srv = i + 1;
			else
				what_srv = 0;

			// daca este replica: what_srv -> pozitia serverului original
			for (uint j = 0; j < main->servers_count; j++) {
				uint id = main->servers[j]->server_id;
				uint original = main->servers[what_srv]->original_server;
				if (id == original) {
					what_srv = j;
					break;
				}
			}

			uint info_size = 0;
			hashtable_t *ht = main->servers[what_srv]->hashtable;
			info **data = ht_get_all_data(ht, &info_size);
			if (data != NULL) {
				for (uint j = 0; j < info_size; j++) {
					char *key = malloc(strlen(data[j]->key) + 1);
					char *value = malloc(strlen(data[j]->value) + 1);

					strcpy(key, data[j]->key);
					strcpy(value, data[j]->value);

					ht_remove_entry(main->servers[what_srv]->hashtable, data[j]->key);

					// dummy pentru ca nu ne este de folos server_id din loader_store
					int dummy = 0;
					loader_store(main, key, value, &dummy);

					free(key);
					free(value);
				}
				for (uint j = 0; j < info_size; j++) {
					free(data[j]->key);
					free(data[j]->value);
					free(data[j]);
				}
				free(data);
			}
		}
	}
}

void loader_remove_server(load_balancer *main, int server_id) {
	uint replica0, replica1, replica2;
	replica0 = server_id;
	replica1 = 1 * 100000 + server_id;
	replica2 = 2 * 100000 + server_id;

	// caut pozitia serverului original ca sa iau datele din el
	int pos = 0;
	for (uint i = 0; i < main->servers_count; i++) {
		if (main->servers[i]->server_id == replica0) {
			pos = i;
			break;
		}
	}
	uint info_size = 0;
	info **data = ht_get_all_data(main->servers[pos]->hashtable, &info_size);

	remove_server_from_array(main, replica0);
	remove_server_from_array(main, replica1);
	remove_server_from_array(main, replica2);

	// redistribuire date
	for (uint i = 0; i < info_size; i++) {
		int dummy = 0;
		loader_store(main, data[i]->key, data[i]->value, &dummy);
	}

	for (uint i = 0; i < info_size; i++) {
		free(data[i]->key);
		free(data[i]->value);
		free(data[i]);
	}
	free(data);
}

void loader_store(load_balancer *main, char *key, char *value, int *server_id) {
	// se va stoca obiectul pe primul server cu hash mai mare
	uint obj_hash = hash_function_key(key);
	uint what_srv;
	int ok = 0;
	for (uint i = 0; i < main->servers_count; i++) {
		uint hash = main->servers[i]->server_id;
		hash = hash_function_servers(&hash);
		if (hash > obj_hash) {
			what_srv = main->servers[i]->original_server;
			ok = 1;
			break;
		}
	}
	if (ok == 0)
		what_srv = main->servers[0]->original_server;

	int pos = 0;
	for (uint i = 0; i < main->servers_count; i++) {
		if (main->servers[i]->server_id == what_srv) {
			pos = i;
			break;
		}
	}
	server_store(main->servers[pos], key, value);
	*server_id = what_srv;
}

char *loader_retrieve(load_balancer *main, char *key, int *server_id) {
	uint obj_hash = hash_function_key(key);
	uint what_srv;
	int ok = 0, pos = 0;
	for (uint i = 0; i < main->servers_count; i++) {
		uint hash = main->servers[i]->server_id;
		hash = hash_function_servers(&hash);
		if (hash > obj_hash) {
			what_srv = main->servers[i]->original_server;
			ok = 1;
			break;
		}
	}
	if (ok == 0)
		what_srv = main->servers[0]->original_server;
	for (uint i = 0; i < main->servers_count; i++) {
		if (main->servers[i]->server_id == what_srv) {
			pos = i;
			break;
		}
	}
	*server_id = what_srv;
	return server_retrieve(main->servers[pos], key);
}

void free_load_balancer(load_balancer *main) {
	for (uint i = 0; i < main->servers_count; i++) {
		free_server_memory(main->servers[i]);
	}
	free(main->servers);
	free(main);
}
