/* See LICENSE file for license and copyright details */
#ifndef DRUNKCAN_INTARRAY_H
#define DRUNKCAN_INTARRAY_H

struct int_array {
	unsigned int size;
	unsigned int i;
	int *data;
};

struct int_array int_array_init(int size);
int *int_array_push(struct int_array *arr, int data);
int int_array_search(const struct int_array arr, int data);
int int_array_remove(struct int_array *arr, int data);
void int_array_destroy(struct int_array arr);

#endif
