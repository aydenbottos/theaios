My-OS – Development Progress Overview
====================================

Last updated: $(date +%Y-%m-%d)

This document is a quick, high-level overview of where the project
stands right now so that new or returning contributors can orient
themselves quickly.

────────────────────────────────────────────────────────────
Core boot & kernel
────────────────────────────────────────────────────────────
• Multiboot-compliant 32-bit kernel image built with GCC 14 and NASM.
• Real-mode bootstrap → switched to protected mode with paging (4 MiB
  identity map via PSE).
• GDT with kernel/user segments, TSS, and a call-gate for user↔kernel.
• IDT with ISRs (0-31), IRQs (remapped 32-47) and a DPL-3 syscall gate
  (int 0x80).
• Basic drivers:
    – Serial (COM1) output.
    – PIT (100 Hz timer) + IRQ0 handler.
    – PS/2 keyboard IRQ1 handler.
    – ATA-PIO driver (read + write 512-byte sectors).

────────────────────────────────────────────────────────────
Memory
────────────────────────────────────────────────────────────
• Physical-memory manager (bump allocator).
• Kernel heap (simple bump allocator) exposed via kmalloc().

────────────────────────────────────────────────────────────
Tasking / scheduling
────────────────────────────────────────────────────────────
• Fixed-size task table (MAX_TASKS = 16) with round-robin scheduler.
• context_switch_user() does ring-transitions via IRET.
• Basic syscalls (write/exit).  ps / kill shell commands added.

────────────────────────────────────────────────────────────
File system
────────────────────────────────────────────────────────────
• Full read-write FAT-12 support on a 1.44 MiB floppy image.
    – Cluster allocation, FAT12 12-bit entry logic.
    – Directory search / create / delete / rename.
    – High-level ops: read, write (overwrite), append, delete, rename,
      copy (shell helper), free-space query.

────────────────────────────────────────────────────────────
Shell (boot-time user interface)
────────────────────────────────────────────────────────────
Built-in commands (2025-05):

  echo TEXT              – print text
  help                   – this list
  clear/cls              – clear screen
  reboot | halt          – power control
  uptime                 – seconds since boot
  history / !n           – command history recall
  sleep N                – busy-wait sleep

  ls                     – list files + sizes
  cat FILE               – dump file
  write  FILE TEXT       – create/overwrite
  append FILE TEXT       – append
  rm FILE                – delete
  rename A B             – rename
  cp SRC DST             – copy file
  df                     – show free disk space

  run ELF                – load ELF into memory as new task
  ps                     – show tasks
  kill TID               – terminate task

  malloc N               – test kmalloc & show ptr
  rand [N]               – pseudo-random 0..N-1 (default 32 768)

Startup extras:
• MOTD.TXT (if present) auto-printed on shell start.

────────────────────────────────────────────────────────────
Build & run
────────────────────────────────────────────────────────────
• make              – builds kernel.bin (ELF) with LD script.
• make iso          – bundles kernel + GRUB into myos.iso.
• make run          – boots ISO in qemu-system-i386.

────────────────────────────────────────────────────────────
Immediate TODO / ideas
────────────────────────────────────────────────────────────
1. More syscalls (open/read/write/close) to let user programs interact
   with FS without shell assistance.
2. Pre-emptive multitasking (time-slice + task status fields).
3. Switch heap & task structs to dynamic allocations.
4. VGA cursor HW update and scrolling optimisation.
5. Build pipeline improvements: CI, unit tests that run under QEMU.

Feel free to pick up one of these or open an issue with your own idea!
