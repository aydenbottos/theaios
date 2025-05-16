// src/task.c

#include "task.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * Globals for our simple round‑robin scheduler
 * Declared in task.h:
 *
 *   extern task_t tasks[MAX_TASKS];
 *   extern int    current_task;
 *   extern int    task_count;
 */
task_t tasks[MAX_TASKS];
int    current_task = 0;
int    task_count   = 0;

/*
 * Helper to grab the current kernel ESP so we can save it
 * before switching away from the current task.
 */
static inline uint32_t get_current_esp(void) {
    uint32_t esp;
    __asm__ volatile("movl %%esp, %0" : "=r"(esp));
    return esp;
}

/*
 * Create a new user‐mode task:
 *   entry_point   = the EIP where the user code begins (e.g. ELF e_entry)
 *   user_stack_top= the top of that task’s stack (must be in identity map)
 *
 * Returns the new task ID (0..MAX_TASKS‑1), or ‑1 on overflow.
 */
int task_create_user(uint32_t entry_point, uint32_t user_stack_top) {
    if (task_count >= MAX_TASKS) {
        return -1;
    }
    tasks[task_count].entry_point = entry_point;
    tasks[task_count].esp         = user_stack_top;
    return task_count++;
}

/*
 * schedule(): Round‑robin between tasks[0..task_count‑1].
 * Saves the current kernel ESP, picks the next task,
 * and switches into it via context_switch_user().
 */
void schedule(void) {
    if (task_count <= 1) {
        return;  // nothing to switch to
    }

    /* Save outgoing task’s kernel ESP */
    tasks[current_task].esp = get_current_esp();

    /* Pick next, wrap around */
    int next = (current_task + 1) % task_count;

    /* Grab its entry & stack */
    uint32_t entry = tasks[next].entry_point;
    uint32_t sp    = tasks[next].esp;

    /* Update current_task before switching */
    current_task = next;

    /* Ring‑change via call‑gate into user mode */
    context_switch_user(entry, sp);
}

/*
 * task_yield(): voluntary yield for the current task.
 */
void task_yield(void) {
    schedule();
}

/* Remove task `tid` from the task list. Very simple – shifts array. */
void task_kill(int tid) {
    if (tid < 0 || tid >= task_count) {
        return;
    }

    /* If killing the currently running task, mark for switch */
    bool need_switch = (tid == current_task);

    /* Shift entries left to fill the gap */
    for (int i = tid; i < task_count - 1; i++) {
        tasks[i] = tasks[i + 1];
    }
    task_count--;
    if (task_count == 0) {
        current_task = 0;
    } else if (current_task >= task_count) {
        current_task = 0;
    }

    if (need_switch) {
        schedule();
    }
}
