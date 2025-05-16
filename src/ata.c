#include "ata.h"
#include "util.h"

// I/O ports for primary ATA
#define ATA_DATA      0x1F0
#define ATA_ERROR     0x1F1
#define ATA_SECTOR_CNT 0x1F2
#define ATA_LBA_LOW   0x1F3
#define ATA_LBA_MID   0x1F4
#define ATA_LBA_HIGH  0x1F5
#define ATA_DRIVE     0x1F6
#define ATA_COMMAND   0x1F7
#define ATA_CONTROL   0x3F6

#define ATA_CMD_READ  0x20
#define ATA_CMD_WRITE 0x30

void ata_init(void) {
    // nothing to do for PIO‑only
}

int ata_read_sector(uint8_t drive, uint32_t lba, uint8_t *buffer) {
    // 1) Select drive & LBA
    outb(ATA_CONTROL, 0);
    outb(ATA_DRIVE, 0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    // 2) Issue READ command
    outb(ATA_COMMAND, ATA_CMD_READ);
    // 3) Wait for BSY=0
    uint8_t status;
    do { status = inb(ATA_COMMAND); } while (status & 0x80);
    // 4) Wait for DRQ=1
    while (!(inb(ATA_COMMAND) & 0x08));
    // 5) Read 256 words
    for (int i = 0; i < 256; i++) {
        uint16_t w = inw(ATA_DATA);
        buffer[2*i + 0] = w & 0xFF;
        buffer[2*i + 1] = w >> 8;
    }
    return 0;
}

int ata_write_sector(uint8_t drive, uint32_t lba, const uint8_t *buffer) {
    // Select drive & LBA
    outb(ATA_CONTROL, 0);
    outb(ATA_DRIVE, 0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    // Issue WRITE SECTORS command
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    // Wait for BSY clear
    uint8_t status;
    do { status = inb(ATA_COMMAND); } while (status & 0x80);

    // Wait for DRQ set (device ready to accept data)
    while (!(inb(ATA_COMMAND) & 0x08));

    // Write 256 words (512 bytes)
    for (int i = 0; i < 256; i++) {
        uint16_t w = ((uint16_t)buffer[2*i + 1] << 8) | buffer[2*i];
        asm volatile("outw %%ax, %%dx" :: "a"(w), "d"(ATA_DATA));
    }

    // Final wait for device to finish write (BSY clear, DRQ clear)
    do { status = inb(ATA_COMMAND); } while (status & 0x80);
    while (inb(ATA_COMMAND) & 0x08);

    return 0;
}
