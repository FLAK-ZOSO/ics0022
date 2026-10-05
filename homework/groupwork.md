
# Compiler-generated runtime checks

Especially in languages like Rust, we rely a lot on compiler checks in order to catch possible integer conversion errors. They sometimes create overreliance on the compiler from the developers, but for the rest I see no real downsides as it is a compile-time check (thus: a cost paid only once from the point of view of the consumer) with relatively few false positives (the rules are well known, just as the architecture is to the compiler).

```c
void function(int);

int main(int argc, char* argv[]) {
    double d = 5.0;

    function(d); // This is unsafe
    function(static_cast<int>(d)); // This is safe...
    // ...but only because you know that 5.0 is safe as integer
}
```

Unless you use `-Wno-narrowing-conversion`, GCC will warn you about this (how many times I have ignored that... especially from `int` to `unsigned short`).

If the previous initialization is done with `double d { 5.0 };` in curly braces, it won't even compile.

# Range checking with precondition or postcondition check

This method consists in using conditional statements (i.e. `if`) to detect whether a supplied argument goes outside of the range stipulated in the contract/signature, or more generally holds a value that the function is unable to correctly process; a well written function defines a return code for this case.

```c
// gcc -Wextra -Wall something.c -c -g -o something.o

struct Coords {
    unsigned short y;
    unsigned short x;
};

struct Coords function(int y, int x) {
    if (y > 65535 || x > 65535) {
        // fail
    }
    if (y < 0 || x < 0) {
        // fail
    }

    // continue processing

    struct Coords value = { y, x };
    return value;
}
```

Some languages also support precondition and postcondition checks: some built-in (mainly functional languages to my knowledge) and some via libraries or extended functionality (e.g. C++ with Boost/templates).
