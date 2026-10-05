# Part A

Well, only the last one of the three produces warnings for me.

```bash
flak-zoso@fedora:~/Repositories/ics0022/homework/hw1$ gcc -Wall -Wextra -g program.c -o account
flak-zoso@fedora:~/Repositories/ics0022/homework/hw1$ gcc -Wall -Wextra -O2 program.c -o account_o2 
flak-zoso@fedora:~/Repositories/ics0022/homework/hw1$ gcc -Wall -Wextra -fanalyzer -c program.c -o /dev/null 
program.c: In function ‘create_account’:
program.c:20:5: warning: use of possibly-NULL ‘file’ where non-null expected [CWE-690] [-Wanalyzer-possible-null-argument]
   20 |     fprintf(file, "Username: %s\n", username);
      |     ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ‘create_account’: events 1-2
   18 |     FILE *file = fopen(filename, "w");
      |                  ^~~~~~~~~~~~~~~~~~~~
      |                  |
      |                  (1) this call could return NULL
   19 | 
   20 |     fprintf(file, "Username: %s\n", username);
      |     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      |     |
      |     (2) ⚠️  argument 1 (‘file’) from (1) could be NULL where non-null expected
In file included from program.c:1:
/usr/include/stdio.h:364:12: note: argument 1 of ‘fprintf’ must be non-null
  364 | extern int fprintf (FILE *__restrict __stream,
      |            ^~~~~~~
flak-zoso@fedora:~/Repositories/ics0022/homework/hw1$ 
```

My version.

```bash
gcc (GCC) 16.2.1 20260819 (Red Hat 16.2.1-2)
Copyright (C) 2026 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

I will work with what I have, and I will still guess that compiler optimization can lead to different memory layouts and machine code ordering to an extent, thus making some undefined behaviour concretize for the worst.

The third command reports [CWE-690](https://cwe.mitre.org/data/definitions/690.html), that refers to when a value that could be `NULL` (in this case the return value of `fopen`, that opens a stream but may fail) is used in a subsequent function call without conditional checks; specifically, it is passed as such an argument that dereferencing will be attempted on it. In this case, at line 18.

What I suspect but has not been caught by the compiler is that `scanf` is used to read a string to a buffer of very limited size (16 bytes/characters), and the man page `scanf(3)` stresses a lot how hard it is to use the function properly, as repeated in the `BUGS` section too. This is at line 11.

# Part B

I like to use "AAAA", but when I can I use [cyclic](https://docs.pwntools.com/en/stable/util/cyclic.html) for easier identification of the buffer overflow.

2. Output below

```bash
=== Account Manager ===
1. Create account
2. Exit
Choice: 1
Username: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
Age: 30
Account successfully created: users_AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.txt
Illegal instruction        (core dumped) ./account
```

```bash
=== Account Manager ===
1. Create account
2. Exit
Choice: 1
Username: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
=================================================================
==140703==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7becf68f0050 at pc 0x7fecf9876b11 bp 0x7ffe3be4f550 sp 0x7ffe3be4ed10
WRITE of size 41 at 0x7becf68f0050 thread T0
    #0 0x7fecf9876b10 in scanf_common(void*, int, bool, char const*, __va_list_tag*) (/lib64/libasan.so.8+0x76b10) (BuildId: 61e8165ca90f7a4f06b73f064a938c76deaae079)
    #1 0x7fecf98b6811 in __isoc23_vscanf (/lib64/libasan.so.8+0xb6811) (BuildId: 61e8165ca90f7a4f06b73f064a938c76deaae079)
    #2 0x7fecf98b6ed4 in __isoc23_scanf (/lib64/libasan.so.8+0xb6ed4) (BuildId: 61e8165ca90f7a4f06b73f064a938c76deaae079)
    #3 0x0000004005fb in create_account /home/flak-zoso/Repositories/ics0022/homework/hw1/program.c:11
    #4 0x00000040087d in main /home/flak-zoso/Repositories/ics0022/homework/hw1/program.c:39
    #5 0x7fecf8e0a680 in __libc_start_call_main (/lib64/libc.so.6+0x3680) (BuildId: 85075e2ae2e13352a1413d41c327dacf6461526c)
    #6 0x7fecf8e0a797 in __libc_start_main@GLIBC_2.2.5 (/lib64/libc.so.6+0x3797) (BuildId: 85075e2ae2e13352a1413d41c327dacf6461526c)
    #7 0x000000400484 in _start (/home/flak-zoso/Repositories/ics0022/homework/hw1/account_asan+0x400484) (BuildId: e3a5d0a35eb5972a4d13f60aecae408da8518166)

Address 0x7becf68f0050 is located in stack of thread T0 at offset 80 in frame
    #0 0x000000400555 in create_account /home/flak-zoso/Repositories/ics0022/homework/hw1/program.c:5

  This frame has 3 object(s):
    [48, 52) 'age' (line 8)
    [64, 80) 'username' (line 6)
    [96, 128) 'filename' (line 7) <== Memory access at offset 80 partially underflows this variable
HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontext or vfork
      (longjmp and C++ exceptions *are* supported)
SUMMARY: AddressSanitizer: stack-buffer-overflow /home/flak-zoso/Repositories/ics0022/homework/hw1/program.c:11 in create_account
Shadow bytes around the buggy address:
  0x7becf68efd80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7becf68efe00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7becf68efe80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7becf68eff00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7becf68eff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x7becf68f0000: f1 f1 f1 f1 f1 f1 04 f2 00 00[f2]f2 00 00 00 00
  0x7becf68f0080: f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3 f3
  0x7becf68f0100: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7becf68f0180: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7becf68f0200: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7becf68f0280: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07 
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
==140703==ABORTING
```

3. Output below

```bash
=== Account Manager ===
1. Create account
2. Exit
Choice: 1
Username: a/b
Age: 30
Segmentation fault         (core dumped) ./account
```

```bash
=== Account Manager ===
1. Create account
2. Exit
Choice: 1
Username: a/b
Age: 30
program.c:20:5: runtime error: null pointer passed as argument 1, which is declared to never be null
AddressSanitizer:DEADLYSIGNAL
=================================================================
==142177==ERROR: AddressSanitizer: SEGV on unknown address 0x0000000000c0 (pc 0x7f0aacc499df bp 0x7ffda91d30b0 sp 0x7ffda91d2f90 T0)
==142177==The signal is caused by a READ memory access.
==142177==Hint: address points to the zero page.
    #0 0x7f0aacc499df in __vfprintf_internal (/lib64/libc.so.6+0x429df) (BuildId: 85075e2ae2e13352a1413d41c327dacf6461526c)
    #1 0x7f0aad6b7182 in vfprintf (/lib64/libasan.so.8+0xb7182) (BuildId: 61e8165ca90f7a4f06b73f064a938c76deaae079)
    #2 0x7f0aad6b91cc in fprintf (/lib64/libasan.so.8+0xb91cc) (BuildId: 61e8165ca90f7a4f06b73f064a938c76deaae079)
    #3 0x000000400683 in create_account /home/flak-zoso/Repositories/ics0022/homework/hw1/program.c:20
    #4 0x00000040087d in main /home/flak-zoso/Repositories/ics0022/homework/hw1/program.c:39
    #5 0x7f0aacc0a680 in __libc_start_call_main (/lib64/libc.so.6+0x3680) (BuildId: 85075e2ae2e13352a1413d41c327dacf6461526c)
    #6 0x7f0aacc0a797 in __libc_start_main@GLIBC_2.2.5 (/lib64/libc.so.6+0x3797) (BuildId: 85075e2ae2e13352a1413d41c327dacf6461526c)
    #7 0x000000400484 in _start (/home/flak-zoso/Repositories/ics0022/homework/hw1/account_asan+0x400484) (BuildId: e3a5d0a35eb5972a4d13f60aecae408da8518166)

==142177==Register values:
rax = 0x0000000000000000  rbx = 0x00007ffda91d3118  rcx = 0x0000000000000000  rdx = 0x00007ffda91d3118  
rdi = 0x0000000000000000  rsi = 0x0000000000401820  rbp = 0x00007ffda91d30b0  rsp = 0x00007ffda91d2f90  
 r8 = 0x00000f61554de008   r9 = 0x0000000000000086  r10 = 0x0000000000000003  r11 = 0x00007b0aaa6f0043  
r12 = 0x0000000000000000  r13 = 0x00007b0aaa6f00a0  r14 = 0x00007ffda91d32e0  r15 = 0x00007ffda91d3200  
AddressSanitizer can not provide additional info.
SUMMARY: AddressSanitizer: SEGV (/lib64/libc.so.6+0x429df) (BuildId: 85075e2ae2e13352a1413d41c327dacf6461526c) in __vfprintf_internal
==142177==ABORTING
```

4. Output below

```bash
Age: 4294967295
```

5. Output below

```
Age: 0
```

It was the same each time, but I am guessing it could vary as this is probably UB. It is reading an unsigned integer.

6. Output below

```
Goodbye!
```

## Questions

1. The two buffers sit in memory in the (age), username, file order; username is overflowed into file; the sanitizer detects it (with a canary I suppose? I should read more on the sanitizer).
2. The first one is from an integer underflow for an unsigned type; the second one is from a wrong type being given as input.
3. Because scanning an int and being given some characters is UB for scanf.

# Part C

I am ordering them roughly from the most to the least severe.


## 2. Buffer overflow



### Location

program.c at line 11

### Weakness class

[CWE-120](https://cwe.mitre.org/data/definitions/120.html) Buffer Copy without Checking Size of Input ('Classic Buffer Overflow')

### Mechanism

A function reads to a buffer, but it reads more bytes than the buffer can contain, thus "overflowing" (writing past its end) it. Also, it can cause a second overflow as the same string is then written into the username, but 40+11 goes further than the capacity of 32.

### Evidence

Test 2 from part B

### Impact

User provided input can crash the program.

### Remediation

As per the [Beginner's Guide away from scanf](https://sekrit.de/webdocs/c/beginners-guide-away-from-scanf.html), I would use `fgets` on a buffer, or even just use scanf but specifying the amount of characters in the string (e.g. `%15s`).


## 5. Integer underflow

### Location

program.c at line 14

### Weakness class

[CWE-191](https://cwe.mitre.org/data/definitions/191.html) Integer Underflow

### Mechanism

An unsigned integer is taken as input through scanf, but scanf has no mechanism to enforce the type of its input and thus a small negative value will wrap around to be an obscenely large 32-bit positive value.

### Evidence

Test 4 from part B

### Impact

A user prefixing with a dash their own age will have an implausibly large age.

### Remediation

As per the [Beginner's Guide away from scanf](https://sekrit.de/webdocs/c/beginners-guide-away-from-scanf.html), I would use `fgets` on a buffer and then the appropriate one from the `ato*` family of functions.
