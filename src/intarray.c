#include <stdlib.h>
#include <string.h>

#include "intarray.h"

struct int_array {
	unsigned int size;
	unsigned int i;
	int *data;
};

IntArray
int_array_init(int size)
{
	IntArray arr;

	arr = malloc(sizeof(struct int_array));
	if (!arr) {
		return arr;
	}
	arr->size = size;
	arr->i = 0;

	if (!(arr->data = malloc(sizeof(int) * arr->size))) {
		arr->size = 0;
		free(arr);
		return NULL;
	}

	return arr;
}

int *
int_array_push(IntArray arr, int data)
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
int_array_search(const IntArray arr, int data)
{
	int i;
	for (i = 0; (unsigned int)i < arr->i; i++) {
		if (arr->data[i] == data) {
			return i;
		}
	}
	return -1;
}

int
int_array_remove(IntArray arr, int data)
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

int *
int_array_at(const IntArray arr, int id)
{
	if ((unsigned int)id > arr->i) {
		return NULL;
	}

	return &arr->data[id];
}

int int_array_len(const IntArray arr)
{
	return arr->i;
}

void
int_array_destroy(IntArray arr)
{
	free(arr->data);
	free(arr);
}
