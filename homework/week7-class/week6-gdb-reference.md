# Week 6: GDB reference

Compile with debug information and without optimization, then open the normal demo binary:

```bash
gcc -std=c11 -g -O0 -Wall -Wextra -fno-omit-frame-pointer role_demo.c -o role_demo
gdb ./role_demo
```

The commands below are typed at the `(gdb)` prompt. They use source variables and functions, so you do not need to copy a machine-specific address.

## Observe the role change

```text
set pagination off
set disable-randomization off
break vulnerable_copy
run AAAAAAAAADMIN
print sizeof s->name
print s->role
print &s->name
print &s->role
x/16bx s
next
print s->role
x/16bx s
backtrace
info registers rsp rbp rip
continue
quit
```

At the breakpoint, execution is paused before `strcpy` runs. The role should be USER. `next` runs that source line; the role should then be ADMIN. Exact addresses depend on the run and platform. If GDB is stopped on an earlier source line, use `list` and advance until the copy line before taking the first observation.

`set disable-randomization off` asks GDB to leave address randomization enabled for this process. We inspect addresses dynamically rather than requiring fixed ones.

| Command | What it means |
| --- | --- |
| `break vulnerable_copy` | Pause when this function is reached. A file-and-line breakpoint is also possible. |
| `run ARGUMENT` | Start or restart the program with an argument. |
| `next` | Execute the current source line; run called functions without stepping through their source lines. |
| `step` | Execute a source step, entering a called function when source/debugging information allows it. |
| `continue` | Resume execution until another stop or program exit. |
| `print expression` | Evaluate an expression using the current program state. |
| `print &variable` | Show the address of an object. |
| `x/16bx s` | Examine 16 one-byte units at the address s, displayed in hexadecimal. |
| `backtrace` or `bt` | Show the active function-call chain. |
| `info registers` | Show CPU registers. Values will differ between machines and stops. |
| `list` | Show source code near the current location. |
| `quit` | Leave the debugger. |

## The three main x86 registers

| Purpose | x86-64 | x86-32 | Interpretation |
| --- | --- | --- | --- |
| Stack pointer | RSP | ESP | Current stack-top address |
| Frame pointer | RBP | EBP | Frame anchor when the function uses a frame pointer |
| Instruction pointer | RIP | EIP | Execution location; at a normal debugger stop, the instruction to execute next |

`info registers rsp rbp rip` is for x86-64. For an x86-32 executable, use `info registers esp ebp eip`. Other architectures use different register names. The frame pointer is not mandatory in every optimized program; the supplied compile flag makes it easier to study.

A saved return address is data used later by a return instruction. It is not the same thing as the current instruction-pointer register. This demo modifies the role data, not the return address.

## Optional watchpoint

On a fresh run, stop at `vulnerable_copy` before the copy:

```text
set $session = s
watch $session->role[0]
continue
```

GDB stops when the watched byte changes. It may stop inside the C library rather than your source file. Use `print $session->role` and `backtrace` to interpret the change. This watchpoint is an optional extension; the core walkthrough only requires `next`.

## Investigate the incident-board implementation

Build the revised starter as lab with `-g -O0`, then select an individual case. The Python runner is convenient for the suite; GDB should inspect the C binary itself.

### Received bytes versus claimed field lengths

```bash
gdb ./lab
```

```text
set disable-randomization off
break board_import
run --case import_truncated
print packet_len
print (unsigned int)packet[0]
print (unsigned int)packet[1]
x/10bx packet
print board->used
watch board->used
continue
```

For this supplied case, 10 bytes are readable, while the two counts are both 5. Calculate the size of a complete packet, including its header. The watchpoint can expose an update to the note count before validation is complete. A corrected rejected import must preserve that count.

Use `next`, `finish`, and `print` to inspect the appropriate scope; a watchpoint based on a local parameter can be deleted when that parameter leaves scope. Prefer bounded byte views for the raw packet: its fields are not C strings. Do not ask GDB to read it as a NUL-terminated string.

### Pointer size versus output capacity

On a fresh debugger run:

```text
break board_format
run --case format_normal
print capacity
print sizeof out
list
```

The caller's output capacity is 64, while sizeof out is 8 on the target. Use next to reach and execute the snprintf source line, then inspect:

```text
print written
print out
```

Compare the reported number of characters required with the output actually stored. Check the contract before deciding whether a truncated prefix counts as success. Repeat with `format_small` when investigating what rejection must preserve.

### Records versus bytes during deletion

```text
break board_remove
run --case remove_first
print index
print board->used
print sizeof board->notes[0]
print board->notes[0]
print board->notes[1]
print board->notes[2]
```

Determine how many records need to move, how many bytes that represents, and whether the source and destination ranges overlap. Step through the operation and compare the records again. The correct result after removing alpha is beta followed by gamma, with the unused record zeroed.

### General commands while repairing

`list` shows source; `next` advances a source line; `step` enters a call when source is available; `backtrace` shows the current call chain; `continue` resumes; `run --case NAME` starts the chosen case again. Exit and restart GDB when switching examples if old breakpoints or watchpoints make the session confusing.

Use a prepared VM or classroom machine that permits process tracing. A blocked ptrace operation is an environment limitation, not evidence that your buffer checks are correct or incorrect.

