/* For license and copyright information see LICENSE file */
/* NOTE: This test for currently unused piece of code */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <cmocka.h>

struct protocol_conf {
	int _i;
};

#include "../src/util.h"
#include "../src/workqueue.h"

#define DATASIZE sizeof(int)

static int socketmap_setup(void **state);
static int socketmap_teardown(void **state);
static void socketmap_add_test(void **state);
static void socketmap_find_test(void **state);

static int
socketmap_setup(void **state)
{

	if (!(*state = socketmap_init(5))) {
		return -1;
	}

	return 0;
}

static int
socketmap_teardown(void **state)
{
	SocketMap map;

	map = *state;
	socketmap_destroy(map);

	return 0;
}

static void
socketmap_add_test(void **state)
{
	SocketMap map;
	int data[13] = {1, 8, 10, 12, 100, 7, 18, 20, 33, 537, 1000, 77, 8624};
	int i;

	map = *state;
	for (i = 0; i < 13; i++) {
		assert_non_null(socketmap_add(map, DATASIZE, data[i], i));
	}
}

static void
socketmap_find_test(void **state)
{
	SocketMap map;
	Queue q;
	int data[13] = {1, 8, 10, 12, 100, 7, 18, 20, 33, 537, 1000, 77, 8624};
	int i;

	map = *state;

	for (i = 0; i < 13; i++) {
		q = socketmap_add(map, DATASIZE, data[i], i);
		assert_non_null(q);
		assert_int_not_equal(-1, queue_enque(q, data + i));
	}
	for (i = 0; i < 13; i++) {
		q = socketmap_find(map, data[i]);
		assert_non_null(q);
		assert_non_null(queue_peek(q));
		// assert_int_equal(*(int *)queue_peek(q), data[i]);
	}

}

int
main(void)
{
	int map;
	const struct CMUnitTest socketmap_tests[] = {
		cmocka_unit_test_setup_teardown(socketmap_add_test, socketmap_setup,
						socketmap_teardown),
		cmocka_unit_test_setup_teardown(socketmap_find_test, socketmap_setup,
						socketmap_teardown),
	};

	map = cmocka_run_group_tests_name("Queue tests",
					  socketmap_tests, NULL, NULL);
	return (map == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
