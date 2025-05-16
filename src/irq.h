#ifndef IRQ_H
#define IRQ_H

/* Remap PIC, enable IRQ1 (keyboard), and sti */
void irq_install(void);

#endif /* IRQ_H */
