/* See LICENSE file for license and copyright details */
#ifndef DRUNKCAN_INTARRAY_H
#define DRUNKCAN_INTARRAY_H

typedef struct int_array *IntArray;

IntArray int_array_init(int size);
int *int_array_push(IntArray arr, int data);
int int_array_search(const IntArray arr, int data);
int int_array_remove(IntArray arr, int data);
int *int_array_at(const IntArray arr, int id);
int int_array_len(const IntArray arr);
void int_array_destroy(IntArray arr);

#endif
