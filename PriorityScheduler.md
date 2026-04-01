# Priority-Based Scheduler in xv6-riscv

## 1. Why is this change needed?
The default scheduling algorithmic in xv6-riscv is a simple **Round-Robin** scheduler. While fair in distributing CPU time identically across all `RUNNABLE` processes, it lacks the flexibility needed in true multi-tasking and real-time environments.

A **Priority-Based Scheduler** is needed to allow system administrators and the kernel to prioritize specific processes (such as I/O-bound processes, system daemons, or latency-critical tasks) over less important background processes (like batch jobs or low-priority background computations). This ensures that critical tasks receive CPU time immediately when they are ready to run, improving overall responsiveness and system throughput.

## 2. How it works?
Each process is assigned an integer `priority` value ranging from `0` to `100`. In this design, a **lower numerical value represents a higher priority** (e.g., `0` is the highest priority, `100` is the lowest).

When a new process is created, it inherits a default priority of `50`. A custom system call, `setpriority(pid, priority)`, allows processes with the proper permissions (or in our simplified model, any process) to dynamically update the priority of an existing process.

The custom scheduler logic operates in multi-passes continuously:
1.  **Selection Phase**: The scheduler scans all processes to identify the lowest numerical `priority` value among all `RUNNABLE` processes.
2.  **Execution Phase**: The scheduler scans the process list again, executing **only those processes** that match the highest selected priority. It schedules them in a Round-Robin pattern to prevent starvation among processes of the *same* priority level.

When a process is granted a priority higher than the currently running process, the `setpriority` system call calls `yield()` independently to cede the CPU. This triggers the scheduler loop to immediately re-evaluate the highest-priority runnable process and preempt the lower-priority process currently executing.

## 3. Why this works?
This implementation works specifically because it avoids deadlock semantics typically introduced algorithmically when designing schedulers:
*   Instead of maintaining the lock on a `best_p` process and subsequently attempting to lock other processes to compare priority values (which inherently triggers deadlocks natively in xv6 logic), our scheduler first comprehensively determines the best global priority safely and drops process locks sequentially.
*   Once established, the scheduler only assigns the CPU to processes containing that exact priority, implicitly running them safely without overlapping context locks.
*   Since the global priority calculation re-occurs upon every trap returning to the scheduler, processes naturally yield when higher-priority workloads become `RUNNABLE`.

## 4. Implementation Details

We introduced the following critical changes across the xv6-riscv codebase to implement the priority scheduler:

1.  **Process Definition (`struct proc`)**
    *   File: `kernel/proc.h`
    *   Modified `struct proc` to include a new integer field `int priority;`.
2.  **Process Allocation (`allocproc`)**
    *   File: `kernel/proc.c`
    *   Defaulted `p->priority = 50;` ensuring any initialized process doesn't starve naturally.
3.  **Scheduler Loop (`scheduler`)**
    *   File: `kernel/proc.c`
    *   Replaced the singular loop running all processes sequentially.
    *   Engineered a two-pass sweep structure: the first determines `best_priority`, and the second iteration only grants `RUNNING` status via the `swtch` function to processes where `p->priority == best_priority`.
4.  **The System Call (`setpriority`)**
    *   File: `kernel/sysproc.c`, `kernel/proc.c`
    *   Added tracking macros in `kernel/syscall.h` (`SYS_setpriority 26`).
    *   Hooked function `sys_setpriority` utilizing `argint` properties.
    *   The `setpriority` function manipulates the specific struct instance by fetching `p->pid`, configuring the new priority, and issuing a core `yield()` command if the priority dictates preemption over the active stack.
5.  **User-Space Stubs**
    *   File: `user/user.h`, `user/usys.pl`
    *   Exported `setpriority` prototype functionality mapped to system call index 26.
6.  **User-Space Test Program**
    *   File: `user/testpriority.c`
    *   Demonstrates the functionality by forking two processes and artificially assigning priority `5` to one, and priority `20` to the other, yielding observable computing distribution skews favoring the higher priority child wrapper.
