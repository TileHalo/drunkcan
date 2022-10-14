/* See LICENSE file for license and copyright details */

#ifndef DRUNKCAN_BINTREE_H
#define DRUNKCAN_BINTREE_H

#define UNIX_NAMESIZE 109

struct int_array {
	unsigned int size;
	unsigned int i;
	int *data;
};

/* Binary tree nodes that can search with both fd and id */
struct btree_node {
	int id; /* CANopen/etc node id. Max is 29 bits */
	int fd; /* epoll listen fd */
	struct int_array clients; /* epoll client fds */
	struct btree_node *idl, *idr;
	struct btree_node *fdl, *fdr;
};

/* Binary tree that can search with both fd and id */
struct btree {
	int size; /* Max 29 bits */
	int root;
	char prefix[UNIX_NAMESIZE]; /* Prefix for all fd names */
	struct protocol_conf protocol;
	struct btree_node *tree; /* This is list by id */
};

struct int_array int_array_init(int size);
int *int_array_push(struct int_array *arr, int data);
int *int_array_search(const struct int_array arr, int data);
int int_array_remove(struct int_array *arr, int data);
void int_array_destroy(struct int_array arr);

struct btree btree_init(int size);
struct btree_node *btree_insert(struct btree *tree, int id, int fd);
struct btree_node *btree_search_fd(const struct btree tree, int fd);
struct btree_node *btree_search_id(const struct btree tree, int id);
struct btree_node btree_remove_fd(struct btree *tree, int fd);
struct btree_node btree_remove_id(struct btree *tree, int id);
void btree_destroy(struct btree tree);

#endif
