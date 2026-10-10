
# Part A

```bash
gcc -g -Wall -Wextra program.c -o program
gcc -Wall -Wextra -fanalyzer -c program.c -o /dev/null
gcc -g -fsanitize=address,undefined program.c -o program_asan
```

None of them produced any warning or error for me.

```bash
./program alice secure1234560
```
*This call is the shortest one that changes behaviour, with **13 characters** in the password*

```bash
flak-zoso@fedora:~/Repositories/ics0022/homework/hw2$ ./program alice secure123450000
ACCESS GRANTED
Logged in as: alice
flak-zoso@fedora:~/Repositories/ics0022/homework/hw2$ ./program alice secure1234500000
ACCESS GRANTED
Logged in as: alice

^C
flak-zoso@fedora:~/Repositories/ics0022/homework/hw2$ ./program alice secure12345000000
ACCESS GRANTED
Logged in as: alice
Segmentation fault         (core dumped) ./program alice secure12345000000
```
*These are the cases for which we have a segmentation fault (access to memory outside of the one allocated for the current process, at **17 characters***

```bash
./program_asan alice secure123456
```
*This call is the shortest one for which the sanitizer errors, with **12 characters** in the password*

```
=================================================================
==1774991==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7b17190f002c at pc 0x7f171c283722 bp 0x7ffec85dc1a0 sp 0x7ffec85db960
WRITE of size 13 at 0x7b17190f002c thread T0
    #0 0x7f171c283721 in strcpy.part.0 (/lib64/libasan.so.8+0x83721) (BuildId: 61e8165ca90f7a4f06b73f064a938c76deaae079)
    #1 0x0000004005eb in login /home/flak-zoso/Repositories/ics0022/homework/hw2/program.c:12
    #2 0x0000004007d4 in main /home/flak-zoso/Repositories/ics0022/homework/hw2/program.c:35
    #3 0x7f171b80a680 in __libc_start_call_main (/lib64/libc.so.6+0x3680) (BuildId: 85075e2ae2e13352a1413d41c327dacf6461526c)
    #4 0x7f171b80a797 in __libc_start_main@GLIBC_2.2.5 (/lib64/libc.so.6+0x3797) (BuildId: 85075e2ae2e13352a1413d41c327dacf6461526c)
    #5 0x000000400464 in _start (/home/flak-zoso/Repositories/ics0022/homework/hw2/program_asan+0x400464) (BuildId: 1ebccf8b3d27c52d3b11708327269b1f30bfd599)

Address 0x7b17190f002c is located in stack of thread T0 at offset 44 in frame
    #0 0x00000040055a in login /home/flak-zoso/Repositories/ics0022/homework/hw2/program.c:8

  This frame has 1 object(s):
    [32, 44) 'password' (line 9) <== Memory access at offset 44 overflows this variable
HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontextor vfork
      (longjmp and C++ exceptions *are* supported)
SUMMARY: AddressSanitizer: stack-buffer-overflow /home/flak-zoso/Repositories/ics0022/homework/hw2/program.c:12 in login
Shadow bytes around the buggy address:
  0x7b17190efd80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7b17190efe00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7b17190efe80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7b17190eff00: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7b17190eff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x7b17190f0000: f1 f1 f1 f1 00[04]f3 f3 00 00 00 00 00 00 00 00
  0x7b17190f0080: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7b17190f0100: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7b17190f0180: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7b17190f0200: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  0x7b17190f0280: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
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
==1774991==ABORTING
```
*The output of the AddressSanitizer for that input*

### Questions

1. The password can be of at most 11 characters, but the buffer is of 12: this mismatch is due to the null termination character that by C (and C++) specifications is '\0'
2. The sanitizer immediately notices when memory is accessed outside of a buffer, while the normal program just behaves differently when the overflow is in fact affeccting it, that is not the case when a zero is written on top of a zero
3. "Segmentation fault         (core dumped)" means that memory was accessed outside of that allocated for/by the process, and that a core (~snapshot of the process memory) was dumped to the filesystem for later inspection; this however happened later in the process than the printf call
4. A static analyzer knows that a return value from a function is possible (`NULL` in the case of homework 1) but it doesn't predict what inputs will a user dynamically provide

# Part B

```bash
00000000004004bb <login>:
  4004bb:	push   rbp
  4004bc:	mov    rbp,rsp
  4004bf:	sub    rsp,0x20
  4004c3:	mov    QWORD PTR [rbp-0x18],rdi
  4004c7:	mov    QWORD PTR [rbp-0x20],rsi
  4004cb:	mov    DWORD PTR [rbp-0x4],0x0
  4004d2:	mov    rdx,QWORD PTR [rbp-0x20]
  4004d6:	lea    rax,[rbp-0x10]
  4004da:	mov    rsi,rdx
  4004dd:	mov    rdi,rax
  4004e0:	call   400370 <strcpy@plt>
  4004e5:	lea    rax,[rbp-0x10]
  4004e9:	mov    esi,0x401262
  4004ee:	mov    rdi,rax
  4004f1:	call   4003a0 <strcmp@plt>
  4004f6:	test   eax,eax
  4004f8:	jne    400501 <login+0x46>
  4004fa:	mov    DWORD PTR [rbp-0x4],0x1
  400501:	cmp    DWORD PTR [rbp-0x4],0x0
  400505:	je     40051f <login+0x64>
  400507:	mov    edi,0x40126c
  40050c:	call   400380 <puts@plt>
  400511:	mov    rax,QWORD PTR [rbp-0x18]
  400515:	mov    rdi,rax
  400518:	call   400496 <show_status>
```

### Questions

1. Password is at a lower address (rbp-0x10) than the authenticated variable (rbp-0x04)
2. Yes, in this build strcpy called to write to `password` can overwrite `authenticated`
3. 
