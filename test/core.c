#ifndef TEST 
   #define TEST
#endif
#include <stdio.h>
#include <check.h>
#include <string.h>
#include <core/client/client.h>
#include <core/util/utils.h>
#include <core/jsmn/jsmnutil.h>

void runRequests(char *name)
{
        in3_t *c = in3_new();
        char temp[500];
        sprintf(temp, "test/testdata/%s.json", name);

        FILE *file = fopen(temp, "r");
        ck_assert_ptr_nonnull(file);
        if (!file)
                return;
        size_t allocated = 1024;
        size_t len = 0;
        char *buffer = _malloc(1024);
        size_t r;

        while (1)
        {
                r = fread(buffer + len, 1, allocated - len, file);
                len += r;
                if (feof(file))
                        break;
                buffer = _realloc(buffer, allocated * 2, allocated);
                allocated *= 2;
        }

        fclose(file);

        int tokc, res, i;
        jsmntok_t *t = NULL;
        jsmntok_t *tokens = NULL;

        // parse
        res = jsmnutil_parse_json_range(buffer, len, &tokens, &tokc);
        if (res < 0 || tokc == 0)
                fail("error parsing the request data");

        

        

        _free(tokens);

}

START_TEST(init_in3)
{
        in3_t *c = in3_new();
        ck_assert_msg(c->key == NULL, "must create");

        //   ck_abort_msg("I fail, and tell you that I do");
}
END_TEST

void testR(char *name)
{
        tcase_fn_start(name, __FILE__, __LINE__);
        ck_assert_msg(name[5] != '3', "must be dummy5");
}

START_TEST(init_all)
printf("testing----\n");
   mem_reset(1);


   char* p = _malloc(100);
   printf("before_free %i\n",mem_get_memleak_cnt());

   _free(p);

   printf("after_free %i\n",mem_get_memleak_cnt());
   printf("max_mem_used : %zu bytes",mem_get_max_heap() );



END_TEST

Suite *str_suite(void)
{
        Suite *in3 = suite_create("in3");

        TCase *eth = tcase_create("eth");
        suite_add_tcase(in3, eth);

        tcase_add_test(eth, init_in3);
        tcase_add_test(eth, init_all);
        return in3;
}

int main(int argc, char *argv[])
{
        int number_failed;
        Suite *suite = str_suite();
        SRunner *runner = srunner_create(suite);
        srunner_run_all(runner, CK_NORMAL);
        number_failed = srunner_ntests_failed(runner);
        srunner_free(runner);
        return number_failed;
}