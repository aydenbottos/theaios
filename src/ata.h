#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// Must be called once at boot
void ata_init(void);

// Read exactly one 512‑byte sector from 'drive' (0=master, 1=slave),
// at absolute LBA.  Returns 0 on success, –1 on error.
int ata_read_sector(uint8_t drive, uint32_t lba, uint8_t *buffer);

// Write exactly one 512-byte sector.
int ata_write_sector(uint8_t drive, uint32_t lba, const uint8_t *buffer);

#endif /* ATA_H */
