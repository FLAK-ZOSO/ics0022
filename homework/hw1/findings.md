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

It was the same each time, but it could vary because the value of an uninitialized integer variable is undefined.

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

## 1. Following symbolic link

### Location

program.c at line 18

### Weakness class

[CWE-61](https://cwe.mitre.org/data/definitions/61.html) UNIX Symbolic Link (Symlink) Following

### Mechanism

The `fopen` call can follow a symlink and writes to the file to which the link results instead.

### Evidence

Additional test here.

```bash
touch /tmp/sensitive-configuration-file
ln -s /tmp/sensitive-configuration-file users_innocent-file.txt
```

```bash
flak-zoso@fedora:~/Repositories/ics0022/homework/hw1$ ./account
=== Account Manager ===
1. Create account
2. Exit
Choice: 1
Username: innocent-file
Age: 30
Account successfully created: users_innocent-file.txt
flak-zoso@fedora:~/Repositories/ics0022/homework/hw1$ cat /tmp/sensitive-configuration-file 
Username: innocent-file
Age: 30
```

### Impact

A file accessible by the program but not by the user (e.g. if running with the setuid bit) could be overwritten fatally for the system (e.g. `/etc/shadow`).

### Remediation

Using appropriate flags (i.e. `O_NOFOLLOW` in `open`) to prevent following the symlink. An alternative solution is using an equivalent to `O_EXCL` that fixes both the file overwriting issue and the link following one.

## 2. NULL pointer dereference

### Location

program.c at line 20 (then again at lines 21 and 23)

### Weakness class

[CWE-476](https://cwe.mitre.org/data/definitions/476.html) NULL Pointer Dereference

### Mechanism

`fopen` tries to return a pointer to a `FILE` representing a stream, but when it fails it sets `file` to `NULL`. Then, `fprintf` tries to write to that stream but where the argument passed to it is `NULL`.

### Evidence

Experiment from Part A, plus experiment 3 from Part B

### Impact

It often crashes the process but sometimes (some systems) it may also write to a memory address `0x0`, in which case the username would be written into valid memory (arbitrary code execution possibly, but probably one would get SIGILL just by "Username:" being copied first).

### Remediation

Validating and cleanly exiting on `NULL == file` before using the pointer.

## 3. Buffer overflow

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

## 4. Unchecked integer value input

### Location

program.c at line 36

### Weakness class

[CWE-252](https://cwe.mitre.org/data/definitions/252.html) Unchecked Return Value

### Mechanism

`scanf` is called to read `choice` as an integer, but the return code is not checked and thus the variable could be uninitialized when input such as "abc" is provided and no integer can be scanned.

### Evidence

Experiment 6 from Part B

### Impact

A conditional jump at line 38 is based on a possibly uninitialized value, an undefined behaviour that in this case reaches the user.

### Remediation

As per the [Beginner's Guide away from scanf](https://sekrit.de/webdocs/c/beginners-guide-away-from-scanf.html), I would use `fgets` on a buffer and then `stoi` plus range validation, or `strtol` and read pointers validation and range validation on top.

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

# Part D

## Patches

### 1. Symlink issue

```diff
diff --git a/homework/hw1/secure_account.c b/homework/hw1/secure_account.c
index fb32812..384d89f 100644
--- a/homework/hw1/secure_account.c
+++ b/homework/hw1/secure_account.c
@@ -1,6 +1,10 @@
+// gcc -Wall -Wextra -g secure_account.c -o secure_account 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
+#include <fcntl.h>
+#include <errno.h>
+#include <unistd.h>
 
 void create_account() {
     char username[16];
@@ -15,12 +19,29 @@ void create_account() {
 
     sprintf(filename, "users_%s.txt", username);
 
-    FILE *file = fopen(filename, "w");
+    int fd = open(filename,
+        O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW,
+        S_IRUSR | S_IWUSR
+    );
 
-    fprintf(file, "Username: %s\n", username);
-    fprintf(file, "Age: %u\n", age);
+    if (fd == -1) {
+        if (errno == ELOOP) {
+            // Failure specifically due to O_NOFOLLOW
+            fprintf(stderr, "[FATAL] %s existed already, and as a symlink\n", filename);
+            return;
+        }
+        fprintf(stderr, "[FATAL] Failure in opening %s\n", filename);
+        return;
+    }
+
+    dprintf(fd, "Username: %s\n", username);
+    dprintf(fd, "Age: %u\n", age);
 
-    fclose(file);
+    if (close(fd) == -1) {
+        fprintf(stderr, "[FATAL] Could not close %s\n", filename);
+        // close(2): "close() should not be retried after an error."
+        return;
+    }
 
     printf("Account successfully created: %s\n", filename);
 }
```

```bash
flak-zoso@fedora:~/Repositories/ics0022/homework/hw1$ ./secure_account 
=== Account Manager ===
1. Create account
2. Exit
Choice: 1
Username: innocent-file
Age: 40
[FATAL] users_innocent-file.txt existed already, and as a symlink
```

### 2. NULL pointer dereference

The patch to this came with the same changes from the previous one, as we are now performing error handling for `open` that we weren't for `fopen` previously.

```bash
flak-zoso@fedora:~/Repositories/ics0022/homework/hw1$ ./secure_account 
=== Account Manager ===
1. Create account
2. Exit
Choice: 1
Username: a/b
Age: 50
[FATAL] Failure in opening users_a/b.txt
```

### 3. Buffer overflow

```diff
diff --git a/homework/hw1/secure_account.c b/homework/hw1/secure_account.c
index 35cd654..7e73a29 100644
--- a/homework/hw1/secure_account.c
+++ b/homework/hw1/secure_account.c
@@ -12,7 +12,19 @@ void create_account() {
     unsigned int age;
 
     printf("Username: ");
-    scanf("%s", username);
+    if (!fgets(username, 16, stdin)) {
+        fprintf(stderr, "[FATAL] could not read username\n");
+        return;
+    }
+    char* newline = strchr(username, '\n');
+    if (newline) {
+        *newline = '\0';
+    } else {
+        char c; // This is needed to consume the input in excess
+        while((c = getchar()) != '\n' && c != EOF);
+        fprintf(stderr, "[FATAL] username too long, 14 characters is the maximum\n");
+        return;
+    }
 
     printf("Age: ");
     scanf("%u", &age);
@@ -55,6 +67,8 @@ int main() {
     printf("Choice: ");
 
     scanf("%d", &choice);
+    char c; // This is needed to consume the input in excess
+    while((c = getchar()) != '\n' && c != EOF);
 
     if (choice == 1) {
         create_account();
```

```bash
=== Account Manager ===
1. Create account
2. Exit
Choice: 1
Username: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbb
Age: 40
Account successfully created: users_aaaaaaaaaaaaaaa.txt
```

```
Username: aaaaaaaaaaaaaaa
Age: 40
```

## Fragile patch

I believe the `O_NOFOLLOW` patch to be the most fragile: it is required by the POSIX.1 standard only since the 2008 version, and someone wanting to port this program to an older platform could delete this option for backwards compatibility.

# AI declaration

I asked GPT this question about the "unchecked integer value input" weakness.

> What is the CWE here? Cannot quite track it.
