# Integer Security: The Broken Game Shop

Work individually or in pairs. Your shop uses whole-number credits. A purchase, top-up, or stock update must preserve the intended value and follow the rules below.

## First practical activity: find the shop bug

Your teacher gives you `shop_demo.c`. An item costs 100 credits and the player has 1000 credits.

```bash
gcc -std=c11 -Wall -Wextra -Wconversion shop_demo.c -o shop_demo
./shop_demo 3
./shop_demo 11
```

Run both examples, then try to make the shop accept an order whose real cost exceeds the balance. Your teacher can provide hints if needed.

Record:

- Your chosen quantity.
- The mathematical cost and the cost printed by the program.
- Whether the order was accepted, and the remaining balance.
- A one-sentence explanation of why the payment check failed.

One or two volunteers will show what they did in the final three minutes of this activity. You do not need slides.

## Coding activity: start

Keep `starter.c` and `lab_checks.h` in the same folder. Compile and run:

```bash
gcc -std=c11 -Wall -Wextra -Wconversion starter.c -o lab
./lab
```

Some checks will fail because the functions are unfinished. Complete the three TODO sections in `starter.c`. You do not need an input parser, menu, or actual shop interface.

Use about **35 minutes for reading and implementation, 10 minutes for testing and comments, and 5 minutes for teacher feedback**. Your teacher may ask you to predict an input while you work.

The supplied output pointers always refer to valid writable objects. Every function must return `true` and write the result on success. On rejection, return `false` and leave the output unchanged.

## Task 1: buy safely

Complete `buy(quantity, price, balance, remaining)`.

- Quantity and price must both be greater than zero.
- The mathematical product must fit in `uint32_t`. Check before multiplying.
- The cost must not exceed the balance. Check before subtracting.
- On success, store the remaining balance.

Examples: buying 3 items at 100 credits with a balance of 1000 leaves 700; buying 11 must be rejected. The exploit input you used must also be rejected when it produces an unrepresentable cost. Rejecting only one particular attack quantity is insufficient.

**Hint if needed:** for positive price, think about how many times it fits into `UINT32_MAX`.

## Task 2: top up safely

Complete `top_up(balance, amount, updated)`.

- The mathematical sum must fit in `uint32_t`. Check before adding.
- Zero is a valid top-up amount.
- On success, store the new balance.

Examples: 900 + 100 becomes 1000; `UINT32_MAX + 1` must be rejected; reaching `UINT32_MAX` exactly is valid.

**Hint if needed:** calculate the room left below the maximum.

## Task 3: store stock safely

Complete `pack_stock(count, packed)`.

- The input is signed `int32_t`; the destination is unsigned `uint16_t`.
- Only values from 0 through 65535 are valid.
- Check both bounds before casting, then store the value on success.

Examples: -1 must be rejected; 0 and 65535 are valid; 65536 must be rejected.

## Test and prepare

1. Make all 20 supplied checks pass. Keep the checks and their expected outcomes intact.
2. Add three independent cases to `main()` in `lab_checks.h`: one per function. Include at least one new exact-boundary or just-outside-boundary case.
3. Take one screenshot showing compilation and the checks passing.
4. Add a short comment to each function explaining the possible failure and why its validation prevents it.
5. For each added case, record the expected acceptance or rejection, the expected successful output, and the reason for choosing it. Rejected operations must leave the previous output unchanged.

## Brief explanation while working

There is no presentation required from every student. Be ready to explain your code briefly if your teacher visits your desk:

- Which unsafe input did you prevent?
- Which check prevents it, and why is the check itself safe?
- What happens at the exact limit and just beyond it?
- Why does adding a cast or changing to unsigned not automatically fix the problem?

If you work in a pair, both partners should understand the checks. Your comments and explanation count toward the work, whether or not you volunteer for the class demonstration.

## Optional extensions

Explain this example on a system with 32-bit `int`:

```c
uint8_t x = 250;
int y = x + 10;
uint8_t z = x + 10;
```

What are `y` and `z`? At which point does the value change, and why?

If you still have time, choose one:

- Write a function that validates a signed index for an array with ten elements. Test -1, 0, 9, and 10 before performing any array access.
- Calculate the purchase cost using a sufficiently wide intermediate. Explain why you must widen an operand before multiplication and check the result before narrowing.
