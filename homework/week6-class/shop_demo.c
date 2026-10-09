/* Intentionally vulnerable classroom demonstration: unsigned wraparound. */
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Make the arithmetic assumptions explicit for this desktop classroom demo. */
_Static_assert(UINT_MAX == UINT32_MAX, "Use a platform with 32-bit unsigned int");

int main(int argc, char **argv)
{
    if (argc != 2 || argv[1][0] == '\0') {
        fprintf(stderr, "Usage: %s QUANTITY\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Parsing is supplied so the demonstration focuses on arithmetic.
       Reject signs, whitespace, non-digits, zero, and out-of-range input. */
    for (const char *p = argv[1]; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            fputs("Quantity must contain decimal digits only.\n", stderr);
            return EXIT_FAILURE;
        }
    }
    errno = 0;
    char *end;
    unsigned long long parsed = strtoull(argv[1], &end, 10);
    if (errno == ERANGE || *end != '\0' || parsed == 0 || parsed > UINT32_MAX) {
        fputs("Quantity must be between 1 and UINT32_MAX.\n", stderr);
        return EXIT_FAILURE;
    }

    uint32_t quantity = (uint32_t)parsed;
    uint32_t price = UINT32_C(100);
    uint32_t balance = UINT32_C(1000);

    /* BUG: the operands fit individually, but their product might not fit.
       The unsigned 32-bit multiplication can wrap before the payment check. */
    uint32_t total = quantity * price;

    printf("Quantity: %" PRIu32 "\n", quantity);
    printf("Price per item: %" PRIu32 " credits\n", price);
    printf("Calculated total: %" PRIu32 " credits\n", total);

    if (total > balance) {
        puts("Purchase rejected: insufficient credits.");
        return EXIT_SUCCESS;
    }

    balance -= total;
    puts("Purchase accepted.");
    printf("Items granted: %" PRIu32 "\n", quantity);
    printf("Remaining balance: %" PRIu32 " credits\n", balance);
    return EXIT_SUCCESS;
}
