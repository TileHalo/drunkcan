/* See LICENSE file for license and copyright details */
#ifndef DRUNKCAN_BINTREE_H
#define DRUNKCAN_BINTREE_H

#define UNIX_NAMESIZE 109

/* Binary tree nodes that can search with both fd and id */
typedef struct btree_node *Node;
typedef struct btree *BinTree;
// typedef struct int_array IntArray;

BinTree btree_init(int size, const char *prefix);
Node btree_insert(BinTree tree, int id, int fd);
Node btree_root(const BinTree tree);
Node btree_search_fd(const BinTree tree, int fd);
Node btree_search_id(const BinTree tree, int id);
Node btree_remove_fd(BinTree tree, int fd);
Node btree_remove_id(BinTree tree, int id);
void btree_set_protocol(BinTree tree, struct protocol_conf conf);
void btree_destroy(BinTree tree);
void btree_apply(BinTree tree, void (*func)(Node node, void *dat), void *dat);
struct protocol_conf *tree_protocol(BinTree tree);
const char *tree_prefix(const BinTree tree);
void *tree_status(BinTree tree);

int node_id(const Node node);
int node_fd(const Node node);
IntArray node_clients(Node node);
void *node_status(Node node);

#endif
