/* See LICENSE file for license and copyright details */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "util.h"
#include "intarray.h"
#include "protocol.h"
#include "btree.h"

struct btree_node {
	int id; /* CANopen/etc node id. Max is 29 bits */
	int fd; /* epoll listen fd */
	int client;
	Node idl, idr;
	Node fdl, fdr;
	void *status;
};

/* Binary tree that can search with both fd and id */
struct btree {
	int size; /* Max 29 bits */
	int root;
	void *status;
	char prefix[UNIX_NAMESIZE]; /* Prefix for all fd names */
	struct protocol_conf protocol;
	Node tree; /* This is list by id */
};

static void btree_insert_fd(Node root, Node node);
static void btree_insert_id(Node root, Node node);

static void clean_tree(Node node, void *dat);
static void traverse(Node root, void (*func)(Node n, void *d), void *d);

static void
btree_insert_fd(Node root, Node node)
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
btree_insert_id(Node root, Node node)
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

static void
clean_tree(Node node, void *dat)
{
	char sock[UNIX_NAMESIZE + 11], *prefix;

	prefix = dat;


	if (node->status) {
		free(node->status);
	}
	sprintf(sock, "%s_%d", prefix, node->id);
	remove(sock);

}

static void
traverse(Node root, void (*func)(Node n, void *d), void *d)
{
	if (!root) {
		return;
	}
	traverse(root->idl, func, d);
	func(root, d);
	traverse(root->idr, func, d);

}

BinTree
btree_init(int size, const char *prefix)
{
	BinTree tree;


	if (!(tree = malloc(sizeof(struct btree)))) {
		return NULL;
	}

	if (!(tree->tree = calloc(size, sizeof(struct btree_node)))) {
		return NULL;
	}

	tree->size = size;
	tree->root = -1;

	if (prefix == NULL) {
		memset(tree->prefix, 0, UNIX_NAMESIZE);
	} else {
		strcpy(tree->prefix, prefix);
	}

	tree->status = NULL;

	return tree;
}


Node
btree_insert(BinTree tree, int id, int fd)
{
	struct btree_node node;

	if (id >= tree->size) {
		int i, osize;
		ptrdiff_t *old[4];

		for (i = 0; i < 4; i++) {
			if (!(old[i] = malloc(tree->size * sizeof(node)))) {
				return NULL;
			}
			memset(old[i], 1, sizeof(node) * tree->size);
		}
		for (i = 0; i < tree->size; i++) {
			if (tree->tree[i].fdl)
				old[0][i] = tree->tree - tree->tree[i].fdl;
			if (tree->tree[i].fdr)
				old[1][i] = tree->tree - tree->tree[i].fdr;
			if (tree->tree[i].idl)
				old[2][i] = tree->tree - tree->tree[i].idl;
			if (tree->tree[i].idr)
				old[3][i] = tree->tree - tree->tree[i].idr;
		}

		osize = tree->size;
		tree->size = MAX(id + 1, tree->size * 2);
		tree->tree = realloc(tree->tree,
				     sizeof(struct btree_node) * (tree->size));
		if (!tree->tree) {
			return NULL;
		}

		memset(tree->tree + osize, 0,
		       sizeof(node) * (tree->size - osize));

                /* Update all non-null pointers */
		for (i = 0; i < osize; i++) {
			tree->tree[i].fdl = NULL;
			tree->tree[i].fdr = NULL;
			tree->tree[i].idl = NULL;
			tree->tree[i].idr = NULL;
			if (old[0][i] < 0)
				tree->tree[i].fdl = tree->tree - old[0][i];
			if (old[1][i] < 0)
				tree->tree[i].fdr = tree->tree - old[1][i];
			if (old[2][i] < 0)
				tree->tree[i].idl = tree->tree - old[2][i];
			if (old[3][i] < 0)
				tree->tree[i].idr = tree->tree - old[3][i];
		}
		for (i = 0; i < 4; i++) {
			free(old[i]);
		}
	}

	node.id = id;
	node.fd = fd;
	node.idl = NULL;
	node.idr = NULL;
	node.fdl = NULL;
	node.fdr = NULL;
	node.client = -1;
	node.status = NULL;
	tree->tree[id] = node;

	if (tree->root < 0) {
		tree->root = id;
	} else {
		btree_insert_fd(&tree->tree[tree->root], &tree->tree[id]);
		btree_insert_id(&tree->tree[tree->root], &tree->tree[id]);
	}
	return &tree->tree[id];
}

Node
btree_root(const BinTree tree)
{
	if (tree->root < 0) {
		return NULL;
	}
	return &tree->tree[tree->root];
}

Node
btree_search_fd(const BinTree tree, int fd)
{
	Node node;

	if (tree->root < 0) {
		return NULL;
	}
	node = &tree->tree[tree->root];

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

Node
btree_search_id(const BinTree tree, int id)
{
	Node node;

	if (tree->root < 0) {
		return NULL;
	}
	node = &tree->tree[tree->root];

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

void
btree_set_protocol(BinTree tree, struct protocol_conf conf)
{
	tree->protocol = conf;
}

Node btree_remove_fd(BinTree tree, int fd);
Node btree_remove_id(BinTree tree, int id);

void
btree_destroy(BinTree tree)
{
	btree_apply(tree, clean_tree, tree->prefix);
	remove(tree->prefix);
	free(tree->tree);
	if (tree->status) {
		free(tree->status);
	}
	free(tree);
}

void
btree_apply (BinTree tree, void (*func)(Node node, void *dat), void *dat)
{
	traverse(&tree->tree[tree->root], func, dat);
}

struct protocol_conf *
tree_protocol(BinTree tree)
{
	return &tree->protocol;
}

const char *
tree_prefix(const BinTree tree)
{
	return tree->prefix;
}


int
node_id(const Node node)
{
	return node->id;
}

int
node_fd(const Node node)
{
	return node->fd;
}

int
node_set_client(Node node, int fd)
{
	node->client = fd;
	return 0;
}

int
node_set_fd(Node node, int fd)
{
	node->fd = fd;

	return 0;
}

int
node_client(const Node node)
{
	return node->client;
}
