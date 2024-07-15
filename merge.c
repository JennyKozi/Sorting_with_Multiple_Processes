#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"

// Algorithm that merges two arrays
void merge(Record **voters, int begin, int middle, int end) {

	int i = 0, j = 0, h = begin;
	int first_size = middle - begin + 1; // Size of first array
	int second_size = end - middle; // Size of second array

	// Create temp arrays
	Record *first_array, *second_array;
	CHECK_MALLOC_NULL(first_array = (Record *)malloc(first_size * sizeof(Record)));
	CHECK_MALLOC_NULL(second_array = (Record *)malloc(second_size * sizeof(Record)));

	// Copy data to temp arrays
	for (i = 0; i < first_size; i++) {
		first_array[i] = (*voters)[begin + i];
	}
	for (i = 0; i < second_size; i++) {
		second_array[i] = (*voters)[middle + i + 1];
	}
	i = 0;

	// Sort the records
	while (i < first_size && j < second_size) {
		if (strcmp(first_array[i].last_name, second_array[j].last_name) < 0) {
			(*voters)[h] = first_array[i];
			i++;
		}
		// Same last name
		else if (strcmp(first_array[i].last_name, second_array[j].last_name) == 0) {
			if (strcmp(first_array[i].first_name, second_array[j].first_name) < 0) {
				(*voters)[h] = first_array[i];
				i++;
			}
			// Same last name and first name
			else if (strcmp(first_array[i].first_name, second_array[j].first_name) == 0) {
				if (first_array[i].voter_id < second_array[j].voter_id) {
					(*voters)[h] = first_array[i];
					i++;
				}
				else {
					(*voters)[h] = second_array[j];
					j++;
				}
			}
			else {
				(*voters)[h] = second_array[j];
				j++;
			}
		}
		else {
			(*voters)[h] = second_array[j];
			j++;
		}
		h++;
	}

	// Copy remaining voters from first array (if there are any)
	while (i < first_size) {
		(*voters)[h] = first_array[i];
		i++;
		h++;
	}
	// Copy remaining voters from second array (if there are any)
	while (j < second_size) {
		(*voters)[h] = second_array[j];
		j++;
		h++;
	}

	free(first_array);
	free(second_array);
}
