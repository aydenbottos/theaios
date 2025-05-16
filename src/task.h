// src/task.h
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 16

typedef struct {
    uint32_t entry_point;  /* user EIP */
    uint32_t esp;          /* user stack pointer */
    /* …other fields… */
} task_t;

extern task_t tasks[MAX_TASKS];
extern int    current_task;
extern int    task_count;

/* scheduler */
void schedule(void);
void task_yield(void);

/* context switch into Ring 3 via call‑gate */
void context_switch_user(uint32_t entry_point, uint32_t user_stack);

/* create a new user task, returns its TID */
int task_create_user(uint32_t entry_point, uint32_t user_stack);

/* Terminate a task by TID (no-op for invalid or kernel task) */
void task_kill(int tid);

#endif /* TASK_H */
