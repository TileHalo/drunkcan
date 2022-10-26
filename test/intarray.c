/* For license and copyright information see LICENSE file */
/* NOTE: This test for currently unused piece of code */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <linux/can.h>

#include <errno.h>
#include <cmocka.h>
#include <string.h>

#include "../src/util.h"
#include "../src/intarray.h"
#include "../src/protocol.h"

static int array_setup(void **state);
static int array_teardown(void **state);
static void array_push_test(void **state);
static void array_push_stress_test(void **state);
static void array_search_test(void **state);
static void array_remove_test(void **state);

static int
array_setup(void **state)
{

	if (!(*state = int_array_init(10))) {
		return -1;
	}

	return 0;
}

static int
array_teardown(void **state)
{
	IntArray arr;

	arr = *state;
	int_array_destroy(arr);

	return 0;
}

static void
array_push_test(void **state)
{
	IntArray arr;
	const int data[10] = {3, 100, -5, 16, 75, 8, 0, -555, 9, 10};
	int i, *j;

	arr = *state;

	for (i = 0; i < 10; i++) {
		j = int_array_push(arr, data[i]);
		assert_int_equal(*j, data[i]);
		assert_int_equal(int_array_len(arr), i + 1);
	}
}

static void
array_push_stress_test(void **state)
{
	IntArray arr;
	int i, *j;

	const int len = 2000000;

	arr = *state;

	for (i = 0; i < len; i++) {
		j = int_array_push(arr, i);
		assert_int_equal(*j, i);
		assert_int_equal(int_array_len(arr), i + 1);
	}
}

static void
array_search_test(void **state)
{
	IntArray arr;
	int i, j;

	const int data[10] = {3, 100, -5, 16, 75, 8, 0, -555, 9, 10};

	arr = *state;

	for (i = 0; i < 10; i++) {
		int_array_push(arr, data[i]);
	}

	for (i = 9; i > -1; i--) {
		j = int_array_search(arr, data[i]);
		assert_int_not_equal(j, -1);
		assert_int_equal(data[j], data[i]);
	}

	j = int_array_search(arr, 10000);
	assert_int_equal(j, -1);
}

static void
array_remove_test(void **state)
{
	IntArray arr;
	int i;

	const int data[10] = {3, 100, -5, 16, 75, 8, 0, -555, 9, 10};

	arr = *state;

	for (i = 0; i < 10; i++) {
		int_array_push(arr, data[i]);
	}
	assert_int_equal(int_array_remove(arr, 8), 9);
	assert_int_equal(int_array_remove(arr, 8), -1);
	assert_int_equal(*int_array_push(arr, 8), 8);

}


int
main(void)
{
	int arr;
	const struct CMUnitTest array_tests[] = {
		cmocka_unit_test_setup_teardown(array_push_test,
						array_setup,
						array_teardown),
		cmocka_unit_test_setup_teardown(array_push_stress_test,
						array_setup,
						array_teardown),
		cmocka_unit_test_setup_teardown(array_search_test, array_setup,
						array_teardown),
		cmocka_unit_test_setup_teardown(array_remove_test, array_setup,
						array_teardown),
	};

	arr = cmocka_run_group_tests_name("Array tests",
					  array_tests, NULL, NULL);
	return (arr == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
