/* See LICENSE file for license and copyright details */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "util.h"
#include "intarray.h"
#include "protocol.h"
#include "btree.h"


static void btree_insert_fd(struct btree_node *root, struct btree_node *node);
static void btree_insert_id(struct btree_node *root, struct btree_node *node);

static void
btree_insert_fd(struct btree_node *root, struct btree_node *node)
{
	while (root->fd != node->fd) {
		if (root->fd < node->fd) {
			if (!root->fdr) {
				root->fdr = node;
			} else {
				root = root->fdr;
			}
		} else if (root->fd > node->fd) {
			if (!root->fdl) {
				root->fdl = node;
				root = node;
			} else {
				root = root->fdl;
			}
		}
	}
}

static void
btree_insert_id(struct btree_node *root, struct btree_node *node)
{
	while (root->id != node->id) {
		if (root->id < node->id) {
			if (!root->idr) {
				root->idr = node;
			} else {
				root = root->idr;
			}
		} else if (root->id > node->id) {
			if (!root->idl) {
				root->idl = node;
				root = node;
			} else {
				root = root->idl;
			}
		}
	}
}


struct btree
btree_init(int size)
{
	struct btree tree;

	tree.size = size;

	tree.root = -1;

	memset(tree.prefix, 0, UNIX_NAMESIZE);

	if (!(tree.tree = calloc(size, sizeof(struct btree_node)))) {
		tree.size = 0;
		return tree;
	}

	return tree;
}


struct btree_node *
btree_insert(struct btree *tree, int id, int fd)
{
	struct btree_node node;

	if (id >= tree->size) {
		int i, osize;
		struct btree_node *old;

		if (!(old = calloc(tree->size, sizeof(struct btree_node)))) {
			return NULL;
		}
		osize = tree->size;
		memcpy(old, tree->tree, tree->size);
		tree->size = MAX(id + 1, tree->size * 2);
		tree->tree = realloc(tree->tree,
		       sizeof(struct btree_node)*(tree->size));
		if (!tree->tree) {
			return NULL;
		}

		/* Update all non-null pointers */
		for (i = 0; i < osize; i++) {
			if (old[i].fdl != NULL) {
				tree->tree[i].fdl = tree->tree + (old[i].fdl - old);
			}
			if (old[i].fdr != NULL) {
				tree->tree[i].fdr = tree->tree + (old[i].fdr - old);
			}
			if (old[i].idl != NULL) {
				tree->tree[i].idl = tree->tree + (old[i].idl - old);
			}
			if (old[i].idr != NULL) {
				tree->tree[i].idr = tree->tree + (old[i].idr - old);
			}
		}
		free(old);
	}

	node.id = id;
	node.fd = fd;
	node.idl = NULL;
	node.idr = NULL;
	node.fdl = NULL;
	node.fdr = NULL;
	node.clients = int_array_init(10);
	tree->tree[id] = node;

	if (tree->root < 0) {
		tree->root = id;
	} else {
		btree_insert_fd(&tree->tree[tree->root], &tree->tree[id]);
		btree_insert_id(&tree->tree[tree->root], &tree->tree[id]);
	}
	return &tree->tree[id];
}

struct btree_node *
btree_search_fd(const struct btree tree, int fd)
{
	struct btree_node *node;

	if (tree.root < 0) {
		return NULL;
	}
	node = &tree.tree[tree.root];

	while (node->fd != fd && node) {
		if (node->fd < fd) {
			if (!node->fdr) {
				return NULL;
			} else {
				node = node->fdr;
			}
		} else if (node->fd > fd) {
			if (!node->fdl) {
				return NULL;
			} else {
				node = node->fdl;
			}
		} else { /* Should not reach */
			return node;
		}
	}
	return node;

}

struct btree_node *
btree_search_id(const struct btree tree, int id)
{
	struct btree_node *node;

	if (tree.root < 0) {
		return NULL;
	}
	node = &tree.tree[tree.root];

	while (node->id != id) {
		if (node->id < id) {
			if (!node->idr) {
				return NULL;
			} else {
				node = node->idr;
			}
		} else if (node->id > id) {
			if (!node->idl) {
				return NULL;
			} else {
				node = node->idl;
			}
		} else { /* Should not reach */
			return node;
		}
	}
	return node;
}
struct btree_node btree_remove_fd(struct btree *tree, int fd);
struct btree_node btree_remove_id(struct btree *tree, int id);

void
btree_destroy(struct btree tree)
{
	int i;
	char sock[UNIX_NAMESIZE + 11];

	for (i = 0; i < tree.size; i++) {
		if (tree.tree[i].id > 0) {
			int_array_destroy(tree.tree[i].clients);
			sprintf(sock, "%s_%d", tree.prefix, tree.tree[i].id);
			remove(sock);
		}
	}
	int_array_destroy(tree.tree[0].clients); /* Destroy zeroth clients */
	remove(tree.prefix);
	free(tree.tree);
}
