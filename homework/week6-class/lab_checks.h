#ifndef INTEGER_SECURITY_LAB_CHECKS_H
#define INTEGER_SECURITY_LAB_CHECKS_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int checks = 0;
static int failures = 0;

static void report(const char *label, bool pass)
{
    ++checks;
    if (!pass) {
        ++failures;
    }
    printf("%s: %s\n", pass ? "PASS" : "FAIL", label);
}

static void check_buy(const char *label, uint32_t quantity, uint32_t price,
                      uint32_t balance, bool expected_ok, uint32_t expected)
{
    uint32_t out = UINT32_C(123);
    bool ok = buy(quantity, price, balance, &out);
    bool pass = ok == expected_ok &&
                out == (expected_ok ? expected : UINT32_C(123));
    report(label, pass);
    if (!pass) {
        printf("  actual: accepted=%d, result=%" PRIu32 "\n", (int)ok, out);
    }
}

static void check_top_up(const char *label, uint32_t balance, uint32_t amount,
                         bool expected_ok, uint32_t expected)
{
    uint32_t out = UINT32_C(123);
    bool ok = top_up(balance, amount, &out);
    bool pass = ok == expected_ok &&
                out == (expected_ok ? expected : UINT32_C(123));
    report(label, pass);
    if (!pass) {
        printf("  actual: accepted=%d, result=%" PRIu32 "\n", (int)ok, out);
    }
}

static void check_stock(const char *label, int32_t count,
                        bool expected_ok, uint16_t expected)
{
    uint16_t out = UINT16_C(123);
    bool ok = pack_stock(count, &out);
    bool pass = ok == expected_ok &&
                out == (expected_ok ? expected : UINT16_C(123));
    report(label, pass);
    if (!pass) {
        printf("  actual: accepted=%d, result=%" PRIu16 "\n", (int)ok, out);
    }
}

int main(void)
{
    check_buy("ordinary purchase", 3, 100, 1000, true, 700);
    check_buy("exactly enough money", 10, 100, 1000, true, 0);
    check_buy("insufficient money", 11, 100, 1000, false, 0);
    check_buy("zero quantity", 0, 100, 1000, false, 0);
    check_buy("zero price", 1, 0, 1000, false, 0);
    check_buy("demo attack must be rejected", UINT32_C(42949673), 100,
              1000, false, 0);
    check_buy("largest quantity at price 100 that fits", UINT32_C(42949672),
              100, UINT32_MAX, true, 95);
    check_buy("next quantity at price 100 does not fit", UINT32_C(42949673),
              100, UINT32_MAX, false, 0);
    check_buy("maximum representable cost", UINT32_MAX, 1,
              UINT32_MAX, true, 0);
    check_buy("maximum quantity times two", UINT32_MAX, 2,
              UINT32_MAX, false, 0);

    check_top_up("ordinary top-up", 900, 100, true, 1000);
    check_top_up("exactly reaches maximum", UINT32_MAX - 1, 1,
                 true, UINT32_MAX);
    check_top_up("would exceed maximum", UINT32_MAX, 1, false, 0);
    check_top_up("zero top-up is valid", UINT32_MAX, 0, true, UINT32_MAX);

    check_stock("negative count", -1, false, 0);
    check_stock("zero stock is valid", 0, true, 0);
    check_stock("ordinary stock", 100, true, 100);
    check_stock("largest 16-bit unsigned stock", INT32_C(65535),
                true, UINT16_MAX);
    check_stock("one above 16-bit limit", INT32_C(65536), false, 0);
    check_stock("larger signed count", INT32_MAX, false, 0);

    /* Add three independent cases here, one per function. Include at least
       one new exact-boundary or just-outside-boundary case. */

    printf("\n%d/%d checks passed.\n", checks - failures, checks);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif
