#include <stdint.h>
#include "util.h"
#include "pit.h"
#include "fs.h"
#include "shell.h"
#include "memory.h"
#include "elf.h"
#include "task.h"
#include "serial.h"
#include "syscall.h"
#include "kheap.h"
#include "gui.h"

#define MAX_LINE    128
#define MAX_HISTORY  10

static char linebuf[MAX_LINE];
static uint32_t idx;

static char history[MAX_HISTORY][MAX_LINE];
static uint32_t hist_count = 0;

// forward
static void prompt(void);
static void execute(void);
static void ls_callback(const char *name, uint32_t size);

#define USER_STACK_SIZE 0x1000   // 4 KiB per user task
#define MAX_FILE_SIZE 16384   // adjust as you like

// called by keyboard.c for each ASCII char
void shell_feed(char c) {
    if (c == '\n' || c == '\r') {
        putc('\n',7);
        execute();
        idx = 0;
        prompt();
    }
    else if (c == '\t') {
        // Insert 4 spaces
        for (int i = 0; i < 4 && idx < MAX_LINE-1; i++) {
            linebuf[idx++] = ' ';
            putc(' ', 7);
        }
    }
    else if (c == '\b') {
        if (idx) {
            idx--;
            putc('\b',7);
        }
    }
    else if (idx < MAX_LINE-1) {
        linebuf[idx++] = c;
        putc(c,7);
    }
}

static void ls_callback(const char *name, uint32_t size) {
    puts(name);
    puts("  ");
    char num[12];
    itoa(size, num, 10);
    puts(num);
    puts(" bytes\n");
}

void shell_init(void) {
    idx = 0;
    /* Display MOTD if present */
    uint8_t *motd = kmalloc(1024);
    int motd_len = fs_read("MOTD.TXT", motd, 1024);
    if (motd_len > 0) {
        for (int i = 0; i < motd_len; i++) putc(motd[i], 7);
        if (motd[motd_len-1] != '\n') putc('\n',7);
    }
    prompt();
}

static void prompt(void) {
    uint32_t s = pit_get_ticks() / 100;
    char buf[12];
    itoa(s, buf, 10);
    puts("[");
    puts(buf);
    puts("] myos> ");
}

static void execute(void) {
    linebuf[idx] = '\0';
    if (!idx) return;

    // save in history (overwrite oldest if full)
    memcpy(history[hist_count % MAX_HISTORY], linebuf, idx+1);
    hist_count++;

    // built‑in commands
    if (strncmp(linebuf, "echo ", 5) == 0) {
        puts(&linebuf[5]); putc('\n',7);
    }
    else if (strcmp(linebuf, "help") == 0) {
        puts("Built-ins: echo, help, clear, reboot, halt, uptime, history, !n,\n");
        puts("           ls, cat, write, append, rm, rename, cp, df, ps, kill, cls, rand, malloc,\n");
        puts("           gui, sleep, free, run\n");
    }
    else if (strcmp(linebuf, "clear") == 0) {
        clear_screen();
    }
    else if (strcmp(linebuf, "cls") == 0) {
        clear_screen();
    }
    else if (strcmp(linebuf, "reboot") == 0) {
        outb(0x64, 0xFE);
    }
    else if (strcmp(linebuf, "gui") == 0) {
        puts("Starting GUI mode...\n");
        gui_init();
        gui_main_loop();
        // GUI has exited, back to shell
        puts("Returned to text mode\n");
    }
    else if (strncmp(linebuf, "sleep ", 6) == 0) {
        int secs = atoi(&linebuf[6]);
        uint32_t target = pit_get_ticks() + secs * 100;
        // busy‑wait with HLT for power efficiency
        while (pit_get_ticks() < target) {
            asm volatile("hlt");
        }
        putc('\n',7);
    }
    else if (strcmp(linebuf, "free") == 0) {
        uint32_t freeb = get_free_mem();
        char num[12];
        itoa(freeb, num, 10);
        puts("Free heap: ");
        puts(num);
        puts(" bytes\n");
    }
    else if (strncmp(linebuf, "run ", 4) == 0) {
        const char *fname = &linebuf[4];
        // read entire file into heap
        uint8_t *img = kmalloc(64 * 1024);       // reserve 64 KiB
        int sz = fs_read(fname, img, 64*1024);
        if (sz < 0) {
            puts("File not found\n"); return;
        }
        // attempt to load ELF; returns entrypoint or 0
        uint32_t eip = load_elf((void*)img);
        if (!eip) {
            puts("Invalid ELF\n"); return;
        }
        // spawn a user‑mode task
        uint32_t stack_top = (uint32_t)kmalloc(USER_STACK_SIZE) + USER_STACK_SIZE;
        int tid = task_create_user(eip, stack_top);
        if (tid < 0) {
            puts("Too many tasks\n"); return;
        }
        puts("Started "); puts(fname); putc('\n',7);
    }  
    else if (strcmp(linebuf, "halt") == 0) {
        puts("Halting...\n"); asm volatile("cli; hlt");
    }
    else if (strcmp(linebuf, "uptime") == 0) {
        // PIT runs at 100Hz
        uint32_t s = pit_get_ticks() / 100;
        char tmp[16];
        itoa(s, tmp, 10);
        puts(tmp); puts(" seconds\n");
    }
    else if (strcmp(linebuf, "history") == 0) {
        uint32_t start = (hist_count > MAX_HISTORY) ? hist_count - MAX_HISTORY : 0;
        for (uint32_t i = start; i < hist_count; i++) {
            char num[4];
            itoa(i, num, 10);
            puts(num); puts(": "); puts(history[i % MAX_HISTORY]); putc('\n',7);
        }
    }
    else if (linebuf[0] == '!' && idx > 1) {
        uint32_t cmd = atoi(&linebuf[1]);
        uint32_t start = (hist_count > MAX_HISTORY) ? hist_count - MAX_HISTORY : 0;
        if (cmd >= start && cmd < hist_count) {
            strcpy(linebuf, history[cmd % MAX_HISTORY]);
            idx = strlen(linebuf);
            puts(linebuf); putc('\n',7);
            execute();
            return;
        }
        puts("Invalid history index\n");
    }
    else if (strcmp(linebuf, "ls") == 0) {
        fs_ls(ls_callback);
    }
    else if (strncmp(linebuf, "cat ", 4) == 0) {
    	const char *fname = &linebuf[4];
    	// allocate a temporary buffer
    	uint8_t *filebuf = kmalloc(MAX_FILE_SIZE);
    	if (!filebuf) {
            puts("Out of memory\n");
    	} else {
        	int len = fs_read(fname, filebuf, MAX_FILE_SIZE);
        	if (len < 0) {
            	    puts("File not found\n");
        	} else {
            	    for (int i = 0; i < len; i++) {
                	putc(filebuf[i], 7);
            	    }
            	    putc('\n', 7);
        	}
    	}
    }	
    else if (strncmp(linebuf, "rm ", 3) == 0) {
        const char *fname = &linebuf[3];
        if (fs_delete(fname) == 0) {
            puts("Deleted\n");
        } else {
            puts("File not found\n");
        }
    }

    else if (strncmp(linebuf, "write ", 6) == 0) {
        /* syntax: write FILE TEXT... */
        const char *args = &linebuf[6];
        /* find first space */
        const char *space = args;
        while (*space && *space != ' ') space++;
        if (*space == '\0') {
            puts("Usage: write <file> <text>\n");
        } else {
            char filename[32];
            int fnlen = space - args;
            if (fnlen >= (int)sizeof(filename)) fnlen = sizeof(filename)-1;
            memcpy(filename, args, fnlen);
            filename[fnlen] = '\0';
            const char *text = space + 1;
            if (fs_write(filename, (const uint8_t*)text, strlen(text)) == 0) {
                puts("Wrote\n");
            } else {
                puts("Write failed\n");
            }
        }
    }

    else if (strncmp(linebuf, "append ", 7) == 0) {
        const char *args = &linebuf[7];
        const char *space = args;
        while (*space && *space != ' ') space++;
        if (*space == '\0') {
            puts("Usage: append <file> <text>\n");
        } else {
            char filename[32]; int fnlen = space - args; if (fnlen >= 31) fnlen = 31;
            memcpy(filename, args, fnlen); filename[fnlen] = '\0';
            const char *text = space + 1;
            if (fs_append(filename, (const uint8_t*)text, strlen(text)) == 0) {
                puts("Appended\n");
            } else {
                puts("Append failed\n");
            }
        }
    }

    else if (strncmp(linebuf, "rename ", 7) == 0) {
        const char *args = &linebuf[7];
        const char *space = args;
        while (*space && *space != ' ') space++;
        if (*space == '\0') { puts("Usage: rename <old> <new>\n"); }
        else {
            char old[32]; char newn[32];
            int len1 = space - args; if (len1 >=31) len1=31; memcpy(old,args,len1); old[len1]='\0';
            const char *second = space+1;
            int len2 = strlen(second); if (len2>=31) len2=31; memcpy(newn,second,len2); newn[len2]='\0';
            if (fs_rename(old,newn)==0) puts("Renamed\n"); else puts("Rename failed\n");
        }
    }

    else if (strncmp(linebuf, "cp ", 3) == 0) {
        const char *args = &linebuf[3];
        const char *space = args; while (*space && *space!=' ') space++;
        if (*space=='\0') { puts("Usage: cp <src> <dst>\n"); }
        else {
            char src[32]; int len1 = space-args; if(len1>=31) len1=31; memcpy(src,args,len1); src[len1]='\0';
            const char *dstptr = space+1;
            char dst[32]; int len2 = strlen(dstptr); if(len2>=31) len2=31; memcpy(dst,dstptr,len2); dst[len2]='\0';
            uint8_t *buf = kmalloc(MAX_FILE_SIZE);
            int sz = fs_read(src, buf, MAX_FILE_SIZE);
            if (sz<0) { puts("Source not found\n"); }
            else if (fs_write(dst, buf, sz)==0) puts("Copied\n"); else puts("Copy failed\n");
        }
    }

    else if (strcmp(linebuf, "df") == 0) {
        uint32_t freeb = fs_free_space();
        char num[16]; itoa(freeb, num, 10);
        puts("Free space: "); puts(num); puts(" bytes\n");
    }
    else if (strncmp(linebuf, "rand", 4) == 0) {
        int max = 32768;
        if (linebuf[4]==' ') max = atoi(&linebuf[5]);
        if (max <= 0) max = 1;
        uint32_t r = rand32() % max;
        char num[12]; itoa(r, num, 10);
        puts(num); putc('\n',7);
    }

    else if (strcmp(linebuf, "ps") == 0) {
        puts("TID   EIP      ESP\n");
        for (int i = 0; i < task_count; i++) {
            char num[4]; itoa(i, num, 10);
            puts(num); puts("  0x");
            char hex[9]; utohex(tasks[i].entry_point, hex); puts(hex);
            puts("  0x"); utohex(tasks[i].esp, hex); puts(hex);
            if (i == current_task) puts("  *\n"); else putc('\n',7);
        }
    }

    else if (strncmp(linebuf, "kill ", 5) == 0) {
        int tid = atoi(&linebuf[5]);
        task_kill(tid);
        puts("kill sent\n");
    }
    else if (strncmp(linebuf, "malloc ", 7) == 0) {
        int n = atoi(&linebuf[7]);
        void* p = kmalloc(n);
        puts("ptr=0x");
        char hex[9];
        utohex((uintptr_t)p, hex);
        puts(hex); putc('\n',7);
    }
    else {
        puts("Unknown\n");
    }
}
