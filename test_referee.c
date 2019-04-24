#define SWEET_NOCOLOUR
#include "sweet.h"
#include "stdlib.h"
#include "stdio.h"
#include "refcount.h"
#define REFCOUNT_TEST
#include "refcount.h"

#define struct(t) \
struct t;\
typedef struct t t;\
struct t

struct (Tester) {
	int i;
	float f;
	char *s;
} Test_Zero = {0};

int main()
{
	/* Tester test = { 2, 3.3, "hello" }; */

	printf("1\n");
	TestGroup("Reference counting")
	{
		TestGroup("init")
		{
			TestGroup("new")
			{
				Rc rc_ = {0}, *rc = &rc_;
				Tester *val = rc_new(rc, sizeof(*val), 0);
				Test(rc_info(rc, val)->refcount == 0);
				Test(rc_info(rc, val)->el_size  == sizeof(Tester));
				Test(rc_info(rc, val)->el_n     == 1);

				Test(rc_inc(rc, val) == val);
			}

			printf("2\n");
			TestGroup("add/remove")
			{
				Rc rc_ = {0}, *rc = &rc_;
				Tester *val  = malloc(sizeof(*val)),
					   *vals = malloc(sizeof(*vals) * 8);
				TestGroup("add")
				{
					rc_add(rc, vals, sizeof(*val), 0);
					rc_add_n(rc, vals, sizeof(*vals), 8, 0);

					Test(rc_info(rc, val )->el_size == sizeof(*val));
					Test(rc_info(rc, vals)->el_size == sizeof(*vals) * 8);
					Test(rc_info(rc, vals + 1) == 0);
				}

				printf("3\n");
				TestGroup("remove")
				{
					Test(val == rc_remove(rc, val));
				}

	printf("4\n");
				free(val);
				free(vals);
			}
		}

		TestGroup("inc/dec/count")
		{
			Rc rc_ = {0}, *rc = &rc_;
			Tester *val = rc_new(rc, sizeof(*val), 0);

			Test(rc_info(rc, val)->refcount == 0);
			Test(rc_count(rc, val) == 0);
			rc_inc(rc, val);
			Test(rc_info(rc, val)->refcount == 1);
			Test(rc_count(rc, val) == 1);
			rc_inc(rc, val);
			Test(rc_info(rc, val)->refcount == 2);
			Test(rc_count(rc, val) == 2);

			rc_inc_c(rc, val, 3);
			Test(rc_info(rc, val)->refcount == 5);
			Test(rc_count(rc, val) == 5);

			rc_dec_c(rc, val, 2);
			Test(rc_info(rc, val)->refcount == 3);
			Test(rc_count(rc, val) == 3);
			rc_dec(rc, val);
			Test(rc_info(rc, val)->refcount == 2);
			Test(rc_count(rc, val) == 2);

			rc_dec_c(rc, val, 64);
			Test(rc_info(rc, val)->refcount == 0);
			Test(rc_count(rc, val) == 0);
			rc_dec(rc, val);
			Test(rc_info(rc, val)->refcount == 0);
			Test(rc_count(rc, val) == 0);
		}

		TestGroup("purge")
		{
			Rc rc_ = {0}, *rc = &rc_;
			Tester *val = rc_new(rc, sizeof(*val), 0);
			Test(rc_info(rc, val)); 
			Test(rc_purge(rc) == 1);
			Test(! rc_info(rc, val)); 
		}

		TestGroup("")
		{

		}
	}

	PrintTestResults(sweetCONTINUE);
	return 0;
}


