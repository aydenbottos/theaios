#ifndef IRQ_H
#define IRQ_H

#include "util.h"

/* IRQ handler function type */
typedef void (*irq_handler_t)(void);

/* Remap PIC, enable IRQs, and sti */
void irq_install(void);

/* Install/uninstall IRQ handlers */
void irq_install_handler(int irq, irq_handler_t handler);
void irq_uninstall_handler(int irq);

/* Main IRQ handler (called from assembly) */
void irq_handler(regs_t *r);

#endif /* IRQ_H */
