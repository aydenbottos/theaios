#ifndef FS_H
#define FS_H

#include <stdint.h>

// Initialize FS internals (reads BPB, computes offsets)
void fs_init(void);

// Read up to `maxlen` bytes of `filename` (8.3, uppercase, no path) into `buffer`.
// Returns number of bytes read, or –1 on error/not found.
int fs_read(const char *filename, uint8_t *buffer, uint32_t maxlen);

// Write `len` bytes from buffer into `filename`. Creates or overwrites.
// Returns 0 on success, –1 on failure (e.g. no space).
int fs_write(const char *filename, const uint8_t *data, uint32_t len);

// Delete a file. Returns 0 on success, –1 if not found.
int fs_delete(const char *filename);

// List root directory; the callback is invoked for every 8.3 filename.
typedef void (*fs_ls_callback)(const char *name, uint32_t size);
void fs_ls(fs_ls_callback cb);

// Append data to existing file (creates if not present).
int fs_append(const char *filename, const uint8_t *data, uint32_t len);

// Rename file; newname must be 8.3 format. Returns 0 on success.
int fs_rename(const char *oldname, const char *newname);

// Return free disk space (bytes) based on unused clusters.
uint32_t fs_free_space(void);

#endif /* FS_H */
