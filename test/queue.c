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

#include "../src/util.h"
#include "../src/workqueue.h"

static int queue_setup(void **state);
static int queue_teardown(void **state);
static void queue_enque_test(void **state);
static void queue_enque_stress_test(void **state);
static void queue_remove_test(void **state);
static void queue_peek_test(void **state);

static int
queue_setup(void **state)
{

	if (!(*state = queue_init(10, sizeof(int)))) {
		return -1;
	}

	return 0;
}

static int
queue_teardown(void **state)
{
	Queue q;

	q = *state;
	queue_destroy(q);

	return 0;
}

static void
queue_enque_test(void **state)
{
	Queue q;
	int data[10] = {3, 100, -5, 16, 75, 8, 0, -555, 9, 10};
	int i, res;

	q = *state;

	for (i = 0; i < 10; i++) {
		res = queue_enque(q, &data[i]);
		assert_int_equal(res, i + 1);
	}
}

static void
queue_enque_stress_test(void **state)
{
	Queue q;
	int i, res;

	const int len = 2000000;

	q = *state;

	for (i = 0; i < len; i++) {
		res = queue_enque(q, &i);
		assert_int_equal(res, i + 1);
	}
}

static void
queue_remove_test(void **state)
{
	Queue q;
	int i, *res;

	int data[10] = {3, 100, -5, 16, 75, 8, 0, -555, 9, 10};

	q = *state;

	assert_null(queue_deque(q));
	for (i = 0; i < 10; i++) {
		queue_enque(q, &data[i]);
	}
	for (i = 0; i < 10; i++) {
		assert_non_null((res = queue_deque(q)));
		assert_int_equal(*res, data[i]);
	}

}

static void
queue_peek_test(void **state)
{
	Queue q;
	int i, *res;

	int data[10] = {3, 100, -5, 16, 75, 8, 0, -555, 9, 10};

	q = *state;

	assert_null(queue_peek(q));
	for (i = 0; i < 10; i++) {
		queue_enque(q, &data[i]);
	}
	for (i = 0; i < 8; i++) {
		assert_non_null((res = queue_peek(q)));
		assert_int_equal(*res, data[i]);
		assert_non_null((res = queue_peek(q)));
		assert_int_equal(*res, data[i]);
		assert_non_null((res = queue_deque(q)));
		assert_int_equal(*res, data[i]);
		assert_non_null((res = queue_peek(q)));
		assert_int_not_equal(*res, data[i]);
	}

}


int
main(void)
{
	int q;
	const struct CMUnitTest queue_tests[] = {
		cmocka_unit_test_setup_teardown(queue_enque_test,
						queue_setup,
						queue_teardown),
		cmocka_unit_test_setup_teardown(queue_enque_stress_test,
						queue_setup,
						queue_teardown),
		cmocka_unit_test_setup_teardown(queue_remove_test, queue_setup,
						queue_teardown),
		cmocka_unit_test_setup_teardown(queue_peek_test, queue_setup,
						queue_teardown),
	};

	q = cmocka_run_group_tests_name("Queue tests",
					  queue_tests, NULL, NULL);
	return (q == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
