/* Please see the file LICENSE for license and copyright information */
#include <stdlib.h>
#include <check.h>

START_TEST(empty_test)
{
	ck_assert(1);
}
END_TEST

Suite * money_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Empty");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, empty_test);
    suite_add_tcase(s, tc_core);
    return s;
}

int
main(void)
{
	int number_failed;
	Suite *s;
	SRunner *sr;

	s = money_suite();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
