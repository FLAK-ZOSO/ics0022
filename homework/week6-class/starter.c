#include <stdbool.h>
#include <stdint.h>

/* Output pointers in this exercise always refer to valid writable objects.
   Return true on success and write the result. On rejection, return false
   and leave the output object unchanged. */

bool buy(uint32_t quantity, uint32_t price, uint32_t balance,
         uint32_t *remaining)
{
    /* TODO: reject zero quantity and zero price.
       Check whether the product fits in uint32_t BEFORE multiplying.
       Reject a cost above the balance BEFORE subtracting.
       On success, store the remaining balance and return true. */
    (void)quantity;
    (void)price;
    (void)balance;
    (void)remaining;
    return false;
}

bool top_up(uint32_t balance, uint32_t amount, uint32_t *updated)
{
    /* TODO: check whether the sum fits in uint32_t BEFORE adding.
       A zero top-up is valid. On success, store the sum and return true. */
    (void)balance;
    (void)amount;
    (void)updated;
    return false;
}

bool pack_stock(int32_t count, uint16_t *packed)
{
    /* TODO: only counts from 0 through UINT16_MAX are valid.
       Reject negative values and values too large for uint16_t.
       Validate BEFORE casting; store the converted value only on success. */
    (void)count;
    (void)packed;
    return false;
}

/* The checks are supplied. You do not need to write a menu or input parser.
   The (void) lines above only suppress warnings in the unfinished version;
   remove them when your implementation uses the corresponding parameters. */
#include "lab_checks.h"
