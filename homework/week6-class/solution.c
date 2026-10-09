#include <stdbool.h>
#include <stdint.h>

/* Contract: output pointers refer to valid writable objects. Rejecting an
   operation leaves its output unchanged. */

bool buy(uint32_t quantity, uint32_t price, uint32_t balance,
         uint32_t *remaining)
{
    /* These are the shop's domain rules. Checking price first also means
       the division in the representability check cannot divide by zero. */
    if (quantity == 0 || price == 0) {
        return false;
    }

    /* Instead of calculating an unsafe product, divide the maximum value.
       For positive price, the product fits exactly when quantity is no
       greater than floor(UINT32_MAX / price). */
    if (quantity > UINT32_MAX / price) {
        return false;
    }
    uint32_t cost = quantity * price;

    /* The product fits, but the customer still needs sufficient funds.
       Checking this before subtraction prevents unsigned underflow. */
    if (cost > balance) {
        return false;
    }
    *remaining = balance - cost;
    return true;
}

bool top_up(uint32_t balance, uint32_t amount, uint32_t *updated)
{
    /* Maximum minus balance cannot wrap: balance is already within the
       uint32_t range. The difference is the available room for a top-up. */
    if (amount > UINT32_MAX - balance) {
        return false;
    }
    *updated = balance + amount;
    return true;
}

bool pack_stock(int32_t count, uint16_t *packed)
{
    /* Test the lower bound before converting a signed value to unsigned.
       Short-circuit evaluation skips the cast when count is negative.
       Positive counts must also fit the smaller destination type. */
    if (count < 0 || (uint32_t)count > UINT16_MAX) {
        return false;
    }
    /* The cast is safe because the range was checked beforehand. */
    *packed = (uint16_t)count;
    return true;
}

#include "../student/lab_checks.h"
