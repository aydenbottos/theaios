#include "fs.h"
#include "ata.h"
#include "util.h"
#include "kheap.h"
#include <stddef.h>

#define SECTOR_SIZE 512

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint16_t fat_size;
    uint32_t fat_start;
    uint32_t root_dir_start;
    uint32_t data_start;
} fat12_info_t;

static fat12_info_t info;

/* ──────────────────────────────────────────────────────────── */
/* Utility helpers for FAT-12                                    */
/* ──────────────────────────────────────────────────────────── */

#define SECTOR_BUF() uint8_t sector[SECTOR_SIZE];

/* Read a raw sector into `sector` */
static void read_sector(uint32_t lba, uint8_t *sector) {
    ata_read_sector(0, lba, sector);
}

static void write_sector(uint32_t lba, const uint8_t *sector) {
    ata_write_sector(0, lba, sector);
}

/* Compute maximum number of clusters */
static uint16_t max_clusters(void) {
    return (uint16_t)((info.fat_size * SECTOR_SIZE * 2) / 3);
}

/* Return 12-bit FAT entry value for `cluster` */
static uint16_t fat_get(uint16_t cluster) {
    uint32_t byte_offset = cluster + (cluster / 2); /* 1.5 * cluster */
    uint32_t lba = info.fat_start + (byte_offset / SECTOR_SIZE);
    SECTOR_BUF();
    read_sector(lba, sector);
    int idx = byte_offset % SECTOR_SIZE;
    uint8_t low = sector[idx];
    uint8_t high = sector[idx + 1];

    uint16_t value;
    if (cluster & 1) {
        /* odd cluster */
        value = ((low >> 4) | (high << 4)) & 0x0FFF;
    } else {
        /* even cluster */
        value = (low | ((high & 0x0F) << 8)) & 0x0FFF;
    }
    return value;
}

/* Write 12-bit `value` into FAT entry `cluster` (updates both FATs) */
static void fat_set(uint16_t cluster, uint16_t value) {
    value &= 0x0FFF;
    uint32_t byte_offset = cluster + (cluster / 2);
    for (int fat = 0; fat < info.num_fats; fat++) {
        uint32_t lba = info.fat_start + fat * info.fat_size + (byte_offset / SECTOR_SIZE);
        SECTOR_BUF();
        read_sector(lba, sector);
        int idx = byte_offset % SECTOR_SIZE;
        if (cluster & 1) {
            /* odd cluster */
            /* 12 bits: high 4 bits in first byte, low 8 bits in second */
            sector[idx] = (sector[idx] & 0x0F) | ((value & 0x00F) << 4);
            sector[idx + 1] = (value >> 4) & 0xFF;
        } else {
            /* even cluster */
            sector[idx] = value & 0xFF;
            sector[idx + 1] = (sector[idx + 1] & 0xF0) | ((value >> 8) & 0x0F);
        }
        write_sector(lba, sector);
    }
}

/* Allocate a free cluster, mark it EOC (0xFFF), return number or 0 on full */
static uint16_t alloc_cluster(void) {
    uint16_t max = max_clusters();
    for (uint16_t c = 2; c < max; c++) {
        if (fat_get(c) == 0x000) {
            fat_set(c, 0xFFF);
            return c;
        }
    }
    return 0; /* disk full */
}

static void free_cluster_chain(uint16_t start) {
    uint16_t c = start;
    while (c >= 2 && c < 0xFF8) {
        uint16_t next = fat_get(c);
        fat_set(c, 0x000);
        c = next;
    }
}

/* ──────────────────────────────────────────────────────────── */
/* Directory helpers                                            */
/* ──────────────────────────────────────────────────────────── */

/* Search root directory for name. If found, outputs sector & offset. */
static int find_dir_entry(const char *fatname, uint32_t *out_lba, uint16_t *out_off) {
    SECTOR_BUF();
    int root_sectors = (info.root_entries * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    for (int s = 0; s < root_sectors; s++) {
        uint32_t lba = info.root_dir_start + s;
        read_sector(lba, sector);
        for (int off = 0; off < SECTOR_SIZE; off += 32) {
            uint8_t first = sector[off];
            if (first == 0x00) return -1; /* end of dir */
            if (first == 0xE5) continue;  /* deleted */
            if (!memcmp(&sector[off], fatname, 11)) {
                if (out_lba) *out_lba = lba;
                if (out_off) *out_off = off;
                return 0;
            }
        }
    }
    return -1;
}

/* Create or overwrite an entry. Returns pointer sector/out offset. */
static int create_dir_entry(const char *fatname, uint16_t first_cluster, uint32_t size) {
    SECTOR_BUF();
    int root_sectors = (info.root_entries * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    for (int s = 0; s < root_sectors; s++) {
        uint32_t lba = info.root_dir_start + s;
        read_sector(lba, sector);
        for (int off = 0; off < SECTOR_SIZE; off += 32) {
            uint8_t first = sector[off];
            if (first == 0x00 || first == 0xE5) {
                /* fill entry */
                memcpy(&sector[off], fatname, 11);
                sector[off + 11] = 0x20; /* ATTR_ARCHIVE */
                /* zero rest of fields */
                memset(&sector[off + 12], 0, 20);
                sector[off + 26] = first_cluster & 0xFF;
                sector[off + 27] = first_cluster >> 8;
                sector[off + 28] = size & 0xFF;
                sector[off + 29] = (size >> 8) & 0xFF;
                sector[off + 30] = (size >> 16) & 0xFF;
                sector[off + 31] = (size >> 24) & 0xFF;
                write_sector(lba, sector);
                return 0;
            }
        }
    }
    return -1; /* dir full */
}

static void delete_entry_at(uint32_t lba, uint16_t off) {
    SECTOR_BUF();
    read_sector(lba, sector);
    sector[off] = 0xE5;
    write_sector(lba, sector);
}

// Helper: uppercase & pad to 11 chars
static void make_fat_name(const char *in, char out[11]) {
    for (int i = 0; i < 11; i++) out[i] = ' ';
    int p = 0;
    // name up to dot or 8 chars
    for (; *in && *in != '.' && p < 8; in++, p++)
        out[p] = (*in >= 'a' && *in <= 'z') ? *in - 32 : *in;
    if (*in == '.') {
        in++;
        for (int j = 0; j < 3 && *in; j++, in++)
            out[8 + j] = (*in >= 'a' && *in <= 'z') ? *in - 32 : *in;
    }
}

void fs_init(void) {
    uint8_t bs[SECTOR_SIZE];
    ata_read_sector(0, 0, bs);
    info.bytes_per_sector    = bs[11] | (bs[12]<<8);
    info.sectors_per_cluster = bs[13];
    info.reserved_sectors    = bs[14] | (bs[15]<<8);
    info.num_fats            = bs[16];
    info.root_entries        = bs[17] | (bs[18]<<8);
    info.fat_size            = bs[22] | (bs[23]<<8);
    info.fat_start           = info.reserved_sectors;
    info.root_dir_start      = info.fat_start + info.num_fats * info.fat_size;
    info.data_start          = info.root_dir_start + ((info.root_entries * 32) + SECTOR_SIZE - 1)/SECTOR_SIZE;
}

int fs_read(const char *filename, uint8_t *buffer, uint32_t maxlen) {
    char fatname[11];
    make_fat_name(filename, fatname);

    uint8_t sector[SECTOR_SIZE];
    int root_sectors = (info.root_entries * 32 + SECTOR_SIZE - 1)/SECTOR_SIZE;

    // Scan root directory
    for (int s = 0; s < root_sectors; s++) {
        ata_read_sector(0, info.root_dir_start + s, sector);
        for (int off = 0; off < SECTOR_SIZE; off += 32) {
            if (sector[off] == 0x00) return -1;   // no more entries
            if ((sector[off] & 0xE5) == 0xE5) continue; // deleted
            if (!memcmp(sector + off, fatname, 11)) {
                uint16_t cluster = sector[off+26] | (sector[off+27]<<8);
                uint32_t filesize = sector[off+28] | (sector[off+29]<<8) |
                                    (sector[off+30]<<16)| (sector[off+31]<<24);
                uint32_t read = 0;
                while (cluster < 0xFF8 && read < filesize && read < maxlen) {
                    uint32_t lba = info.data_start + (cluster - 2)*info.sectors_per_cluster;
                    // read each sector of this cluster
                    for (int i = 0; i < info.sectors_per_cluster; i++) {
                        ata_read_sector(0, lba + i, sector);
                        uint32_t tocopy = (filesize - read < SECTOR_SIZE) ? filesize - read : SECTOR_SIZE;
                        memcpy(buffer + read, sector, tocopy);
                        read += tocopy;
                        if (read >= filesize || read >= maxlen) break;
                    }
                    // fetch next cluster from FAT
                    uint32_t fat_offset = info.fat_start*SECTOR_SIZE + cluster*2;
                    ata_read_sector(0, info.fat_start + (fat_offset/SECTOR_SIZE), sector);
                    cluster = sector[fat_offset%SECTOR_SIZE] | (sector[(fat_offset%SECTOR_SIZE)+1]<<8);
                }
                return read;
            }
        }
    }
    return -1;
}

/* ──────────────────────────────────────────────────────────── */
/* Public API                                                   */
/* ──────────────────────────────────────────────────────────── */

void fs_ls(fs_ls_callback cb) {
    if (!cb) return;
    SECTOR_BUF();
    int root_sectors = (info.root_entries * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE;
    for (int s = 0; s < root_sectors; s++) {
        uint32_t lba = info.root_dir_start + s;
        read_sector(lba, sector);
        for (int off = 0; off < SECTOR_SIZE; off += 32) {
            uint8_t first = sector[off];
            if (first == 0x00) return; /* done */
            if (first == 0xE5 || (sector[off+11] & 0x08)) continue; /* deleted or volume label */

            char name[13];
            int n = 0;
            for (int i = 0; i < 11; i++) {
                char c = sector[off + i];
                if (i == 8) {
                    /* insert dot if extension present */
                    if (c != ' ') name[n++] = '.';
                }
                if (c != ' ') name[n++] = c;
            }
            name[n] = '\0';
            uint32_t size = sector[off+28] | (sector[off+29]<<8) | (sector[off+30]<<16) | (sector[off+31]<<24);
            cb(name, size);
        }
    }
}

int fs_delete(const char *filename) {
    char fatname[11];
    make_fat_name(filename, fatname);
    uint32_t lba; uint16_t off;
    if (find_dir_entry(fatname, &lba, &off) < 0) return -1;

    /* fetch first cluster */
    SECTOR_BUF();
    read_sector(lba, sector);
    uint16_t first_cluster = sector[off+26] | (sector[off+27]<<8);
    if (first_cluster >= 2)
        free_cluster_chain(first_cluster);

    delete_entry_at(lba, off);
    return 0;
}

int fs_write(const char *filename, const uint8_t *data, uint32_t len) {
    char fatname[11];
    make_fat_name(filename, fatname);

    /* If file exists – delete it first */
    fs_delete(filename);

    if (len == 0) {
        /* create zero-length entry with cluster = 0 */
        return create_dir_entry(fatname, 0, 0);
    }

    uint32_t cluster_bytes = info.sectors_per_cluster * SECTOR_SIZE;
    uint32_t remaining = len;

    uint16_t first_cluster = 0;
    uint16_t prev_cluster = 0;

    const uint8_t *p = data;

    while (remaining > 0) {
        uint16_t c = alloc_cluster();
        if (c == 0) {
            /* out of space, cleanup */
            if (first_cluster) free_cluster_chain(first_cluster);
            return -1;
        }
        if (!first_cluster) first_cluster = c;
        if (prev_cluster) fat_set(prev_cluster, c);
        prev_cluster = c;

        /* write this cluster */
        uint32_t lba = info.data_start + (c - 2) * info.sectors_per_cluster;
        for (int s = 0; s < info.sectors_per_cluster; s++) {
            uint8_t sector_buf[SECTOR_SIZE];
            uint32_t tocopy = (remaining < SECTOR_SIZE) ? remaining : SECTOR_SIZE;
            memcpy(sector_buf, p, tocopy);
            if (tocopy < SECTOR_SIZE) memset(sector_buf + tocopy, 0, SECTOR_SIZE - tocopy);
            write_sector(lba + s, sector_buf);
            if (remaining > SECTOR_SIZE) {
                remaining -= SECTOR_SIZE;
                p += SECTOR_SIZE;
            } else {
                p += remaining;
                remaining = 0;
                break;
            }
        }
    }

    /* mark last cluster EOC */
    fat_set(prev_cluster, 0xFFF);

    /* directory entry */
    if (create_dir_entry(fatname, first_cluster, len) < 0) {
        free_cluster_chain(first_cluster);
        return -1;
    }
    return 0;
}

/* Append support: naive – reload old content and re-write completely. */
int fs_append(const char *filename, const uint8_t *data, uint32_t len) {
    /* read existing size */
    uint32_t old_size = 0;
    char fatname[11];
    make_fat_name(filename, fatname);
    uint32_t lba; uint16_t off;
    if (find_dir_entry(fatname, &lba, &off) == 0) {
        SECTOR_BUF();
        read_sector(lba, sector);
        old_size = sector[off+28] | (sector[off+29]<<8) | (sector[off+30]<<16) | (sector[off+31]<<24);
    }

    uint32_t new_size = old_size + len;
    uint8_t *buf = kmalloc(new_size);
    if (!buf) return -1;
    if (old_size) {
        if (fs_read(filename, buf, old_size) != (int)old_size) {
            return -1;
        }
    }
    memcpy(buf + old_size, data, len);
    int res = fs_write(filename, buf, new_size);
    // Note: no free; simple bump allocator
    return res;
}

int fs_rename(const char *oldname, const char *newname) {
    char fat_old[11]; char fat_new[11];
    make_fat_name(oldname, fat_old);
    make_fat_name(newname, fat_new);

    /* ensure destination does not exist */
    if (find_dir_entry(fat_new, NULL, NULL) == 0) return -1;

    uint32_t lba; uint16_t off;
    if (find_dir_entry(fat_old, &lba, &off) < 0) return -1;

    SECTOR_BUF();
    read_sector(lba, sector);
    memcpy(&sector[off], fat_new, 11);
    write_sector(lba, sector);
    return 0;
}

uint32_t fs_free_space(void) {
    uint16_t max = max_clusters();
    uint32_t free_clusters = 0;
    for (uint16_t c = 2; c < max; c++) {
        if (fat_get(c) == 0x000) free_clusters++;
    }
    return free_clusters * info.sectors_per_cluster * SECTOR_SIZE;
}
