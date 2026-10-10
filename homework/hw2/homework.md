
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

At this point I have understood that the default behaviour is not to have a stack protector, so I recompiled

```bash
gcc -g -Wall -Wextra -fstack-protector-strong program.c -o program
```

```bash
Dump of assembler code for function login:
   0x00000000004004cb <+0>:	push   %rbp
   0x00000000004004cc <+1>:	mov    %rsp,%rbp
   0x00000000004004cf <+4>:	sub    $0x30,%rsp
   0x00000000004004d3 <+8>:	mov    %rdi,-0x28(%rbp)
   0x00000000004004d7 <+12>:	mov    %rsi,-0x30(%rbp)
   0x00000000004004db <+16>:	mov    %fs:0x28,%rax
   0x00000000004004e4 <+25>:	mov    %rax,-0x8(%rbp)
   0x00000000004004e8 <+29>:	xor    %eax,%eax
   0x00000000004004ea <+31>:	movl   $0x0,-0x18(%rbp)
   0x00000000004004f1 <+38>:	mov    -0x30(%rbp),%rdx
   0x00000000004004f5 <+42>:	lea    -0x14(%rbp),%rax
   0x00000000004004f9 <+46>:	mov    %rdx,%rsi
   0x00000000004004fc <+49>:	mov    %rax,%rdi
   0x00000000004004ff <+52>:	call   0x400370 <strcpy@plt>
=> 0x0000000000400504 <+57>:	lea    -0x14(%rbp),%rax
   0x0000000000400508 <+61>:	mov    $0x4012ba,%esi
   0x000000000040050d <+66>:	mov    %rax,%rdi
   0x0000000000400510 <+69>:	call   0x4003b0 <strcmp@plt>
   0x0000000000400515 <+74>:	test   %eax,%eax
   0x0000000000400517 <+76>:	jne    0x400520 <login+85>
   0x0000000000400519 <+78>:	movl   $0x1,-0x18(%rbp)
   0x0000000000400520 <+85>:	cmpl   $0x0,-0x18(%rbp)
   0x0000000000400524 <+89>:	je     0x40053e <login+115>
   0x0000000000400526 <+91>:	mov    $0x4012c4,%edi
   0x000000000040052b <+96>:	call   0x400380 <puts@plt>
   0x0000000000400530 <+101>:	mov    -0x28(%rbp),%rax
   0x0000000000400534 <+105>:	mov    %rax,%rdi
   0x0000000000400537 <+108>:	call   0x4004a6 <show_status>
   0x000000000040053c <+113>:	jmp    0x400548 <login+125>
   0x000000000040053e <+115>:	mov    $0x4012d3,%edi
   0x0000000000400543 <+120>:	call   0x400380 <puts@plt>
   0x0000000000400548 <+125>:	mov    -0x18(%rbp),%eax
   0x000000000040054b <+128>:	mov    -0x8(%rbp),%rdx
   0x000000000040054f <+132>:	sub    %fs:0x28,%rdx
   0x0000000000400558 <+141>:	je     0x40055f <login+148>
   0x000000000040055a <+143>:	call   0x400390 <__stack_chk_fail@plt>
   0x000000000040055f <+148>:	leave
   0x0000000000400560 <+149>:	ret
End of assembler dump.
```
*Don't mind the different addresses in other answers, because they were written before I noticed this detail*

### Questions

1. Password is at a lower address (rbp-0x10) than the authenticated variable (rbp-0x04)
2. Yes, in this build strcpy called to write to `password` can overwrite `authenticated`
3. The protective value was set to protect the return value and the stack frame, but the password can reach first the `authenticated` variable

```bash
Breakpoint 1, login (username=0x7fffffffe00a "alice", input=0x7fffffffe010 "secure1234567") at program.c:14
14	    if (strcmp(password, "secure123") == 0) {
(gdb) info locals
password = "secure123456"
authenticated = 55
(gdb) print &authenticated
$1 = (int *) 0x7fffffffdacc
(gdb) print &password
$2 = (char (*)[12]) 0x7fffffffdac0
```

# Part C

```bash
00000000004004bb <login>:
  4004bb:       push   rbp
  4004bc:       mov    rbp,rsp
  4004bf:       sub    rsp,0x20
  4004c3:       mov    QWORD PTR [rbp-0x18],rdi
  4004c7:       mov    QWORD PTR [rbp-0x20],rsi
  4004cb:       mov    DWORD PTR [rbp-0x4],0x0
  4004d2:       mov    rdx,QWORD PTR [rbp-0x20]
  4004d6:       lea    rax,[rbp-0x10]
  4004da:       mov    rsi,rdx
  4004dd:       mov    rdi,rax
  4004e0:       call   400370 <strcpy@plt>
  4004e5:       lea    rax,[rbp-0x10]
  4004e9:       mov    esi,0x401262
  4004ee:       mov    rdi,rax
  4004f1:       call   4003a0 <strcmp@plt>
```

---

<!--
I stopped doing the homework when I read this.

> Do not skip the wording of the last question. A student who writes that the stack protector fixed the bug has drawn precisely the wrong conclusion from the evidence

Like, I am demoralized when I see that an homework has been made up by a clanker and not even proofread. As a student I have taken >5 hours to complete the other homework cleanly by hand, and while I fully understand that at the scale of >100 students it would be unrealistic to expect manual grading, the authorship of the questions...

**Nothing personal, really: I won't mention this to anyone** and it's just between us if this will ever be read, but I'd rather not complete the homework at these conditions :(
-->
