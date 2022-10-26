/* For license and copyright information see LICENSE file */
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/can.h>

#include <cmocka.h>

#include "../src/util.h"
#include "../src/protocol.h"
#include "../src/intarray.h"
#include "../src/btree.h"

static int btree_setup(void **state);
static int btree_teardown(void **state);
static void btree_insert_test(void **state);
static void btree_search_fd_test(void **state);
static void btree_search_id_test(void **state);
static void btree_remove_fd_test(void **state);
static void btree_remove_id_test(void **state);

int __wrap_remove(const char *__filename);

static int ids[11] = { 1, 10, 12, 122, 24, 8, 77, 9, 15, 20, 123 };
static int fds[11] = { 7, 80, 44, 731, 20, 1, 53, 8, 35, 88, 242 };

static int
btree_setup(void **state)
{
	BinTree tree;
	int i;

	if (!(tree = btree_init(20, NULL))) {
		return -1;
	}

	for (i = 0; i < 11; i++){
		if (!btree_insert(tree, ids[i], fds[i])) {
			return -1;
		}
	}

	*state = tree;

	return 0;
}
static int
btree_teardown(void **state)
{
	BinTree tree;

	tree = *state;

	btree_destroy(tree);
	return 0;
}

static void
btree_insert_test(void **state)
{
	BinTree tree;


	tree = *state;

	/* Correct root */
	assert_int_equal(node_id(btree_root(tree)), ids[0]);

	assert_non_null(btree_insert(tree, 2000, 2000));

}

static void
btree_search_fd_test(void **state)
{
	BinTree tree;
	Node n;
	int i;

	tree = *state;

	for (i = 0; i < 11; i++) {
		n = btree_search_fd(tree, fds[i]);
		assert_non_null(n);
		assert_int_equal(node_id(n), ids[i]);
		assert_int_equal(node_fd(n), fds[i]);
	}

	assert_null(btree_search_fd(tree, 0));
	assert_null(btree_search_fd(tree, -5));
	assert_null(btree_search_fd(tree, 1000));
	assert_null(btree_search_fd(tree, 2));
	assert_null(btree_search_fd(tree, 5));
}

static void
btree_search_id_test(void **state)
{
	BinTree tree;
	Node n;
	int i;

	tree = *state;

	for (i = 0; i < 11; i++) {
		n = btree_search_id(tree, ids[i]);
		assert_non_null(n);
		assert_int_equal(node_id(n), ids[i]);
		assert_int_equal(node_fd(n), fds[i]);
	}

	assert_null(btree_search_id(tree, 0));
	assert_null(btree_search_id(tree, -5));
	assert_null(btree_search_id(tree, 1000));
	assert_null(btree_search_id(tree, 2));
	assert_null(btree_search_id(tree, 5));

}

static void
btree_remove_fd_test(void **state)
{
	BinTree tree;
	(void) tree;

	tree = *state;
	warn("Test not implemented");
}

static void
btree_remove_id_test(void **state)
{
	BinTree tree;

	(void) tree;

	tree = *state;
	warn("Test not implemented");
}


int
__wrap_remove(const char *__filename)
{
	(void) __filename;
	return 0;
}


int
main(void)
{
	int btree;
	const struct CMUnitTest btree_tests[] = {
		cmocka_unit_test_setup_teardown(btree_insert_test,
						btree_setup, btree_teardown),
		cmocka_unit_test_setup_teardown(btree_search_fd_test,
						btree_setup, btree_teardown),
		cmocka_unit_test_setup_teardown(btree_search_id_test,
						btree_setup, btree_teardown),
		cmocka_unit_test_setup_teardown(btree_remove_fd_test,
						btree_setup, btree_teardown),
		cmocka_unit_test_setup_teardown(btree_remove_id_test,
						btree_setup, btree_teardown),
	};

	btree = cmocka_run_group_tests_name("Btree ID tests",
					 btree_tests, NULL, NULL);
	return (btree == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
