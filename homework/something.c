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