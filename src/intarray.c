#include <stdlib.h>
#include <string.h>

#include "intarray.h"

struct int_array
int_array_init(int size)
{
	struct int_array arr;

	arr.size = size;
	arr.i = 0;

	if (!(arr.data = malloc(sizeof(int) * arr.size))) {
		arr.size = 0;
		return arr;
	}

	return arr;
}

int *
int_array_push(struct int_array *arr, int data)
{
	if (arr->i == arr->size) {
		arr->size *= 2;
		if (!(arr->data = realloc(arr->data, sizeof(int) * arr->size))) {
			return NULL;
		}
	}

	arr->data[arr->i] = data;
	arr->i++;
	return &arr->data[arr->i - 1];

}
int
int_array_search(const struct int_array arr, int data)
{
	int i;
	for (i = 0; (unsigned int)i < arr.i; i++) {
		if (arr.data[i] == data) {
			return i;
		}
	}
	return -1;
}

int
int_array_remove(struct int_array *arr, int data)
{
	unsigned int i;
	for (i = 0; i < arr->i; i++) {
		if (arr->data[i] == data) {
			memcpy(&arr->data[i], &arr->data[i + 1],
				arr->size - i);
			arr->i--;
			return arr->i;
		}
	}

	return -1;
}

void
int_array_destroy(struct int_array arr)
{
	free(arr.data);
}
