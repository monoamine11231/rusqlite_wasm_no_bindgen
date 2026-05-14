#include "sqlite3.h"
#include <stdint.h>
#include <string.h>


// All data lives in a malloc'd buffer per-file
typedef struct MemFile {
    sqlite3_file base;
    uint8_t *data;
    int size;
} MemFile;

static int memClose(sqlite3_file *pFile) {
    MemFile *p = (MemFile*)pFile;
    sqlite3_free(p->data);
    return SQLITE_OK;
}
static int memRead(sqlite3_file *pFile, void *buf, int amt, sqlite3_int64 off) {
    MemFile *p = (MemFile*)pFile;
    if (off + amt > p->size) return SQLITE_IOERR_SHORT_READ;
    memcpy(buf, p->data + off, amt);
    return SQLITE_OK;
}
static int memWrite(sqlite3_file *pFile, const void *buf, int amt, sqlite3_int64 off) {
    MemFile *p = (MemFile*)pFile;
    if (off + amt > p->size) {
        uint8_t *newdata = sqlite3_realloc(p->data, off + amt);
        if (!newdata) return SQLITE_NOMEM;
        p->data = newdata;
        p->size = off + amt;
    }
    memcpy(p->data + off, buf, amt);
    return SQLITE_OK;
}
static int memTruncate(sqlite3_file *pFile, sqlite3_int64 size) {
    MemFile *p = (MemFile*)pFile;
    p->data = sqlite3_realloc(p->data, size);
    p->size = size;
    return SQLITE_OK;
}
static int memSync(sqlite3_file *pFile, int flags) { return SQLITE_OK; }
static int memFileSize(sqlite3_file *pFile, sqlite3_int64 *pSize) {
    MemFile *p = (MemFile*)pFile;
    *pSize = p->size;
    return SQLITE_OK;
}
static int memLock(sqlite3_file *pFile, int lock)   { return SQLITE_OK; }
static int memUnlock(sqlite3_file *pFile, int lock) { return SQLITE_OK; }
static int memCheckReservedLock(sqlite3_file *pFile, int *pResOut) {
    *pResOut = 0;
    return SQLITE_OK;
}
static int memFileControl(sqlite3_file *pFile, int op, void *pArg) {
    return SQLITE_NOTFOUND;
}
static int memSectorSize(sqlite3_file *pFile)    { return 512; }
static int memDeviceCharacteristics(sqlite3_file *pFile) {
    return SQLITE_IOCAP_ATOMIC |
           SQLITE_IOCAP_SAFE_APPEND |
           SQLITE_IOCAP_SEQUENTIAL |
           SQLITE_IOCAP_POWERSAFE_OVERWRITE;
}

static sqlite3_io_methods mem_io_methods = {
    .iVersion               = 1,
    .xClose                 = memClose,
    .xRead                  = memRead,
    .xWrite                 = memWrite,
    .xTruncate              = memTruncate,
    .xSync                  = memSync,
    .xFileSize              = memFileSize,
    .xLock                  = memLock,
    .xUnlock                = memUnlock,
    .xCheckReservedLock     = memCheckReservedLock,
    .xFileControl           = memFileControl,
    .xSectorSize            = memSectorSize,
    .xDeviceCharacteristics = memDeviceCharacteristics,
};

static int memOpen(sqlite3_vfs *vfs, const char *name, sqlite3_file *pFile,
                   int flags, int *pOutFlags) {
    MemFile *p = (MemFile*)pFile;
    p->base.pMethods = &mem_io_methods;
    p->data = 0;
    p->size = 0;
    if (pOutFlags) *pOutFlags = flags;
    return SQLITE_OK;
}
static int memDelete(sqlite3_vfs *vfs, const char *name, int sync) { return SQLITE_OK; }
static int memAccess(sqlite3_vfs *vfs, const char *name, int flags, int *pRes) {
    *pRes = 0;
    return SQLITE_OK;
}
static int memFullPathname(sqlite3_vfs *vfs, const char *name, int nOut, char *zOut) {
    sqlite3_snprintf(nOut, zOut, "%s", name);
    return SQLITE_OK;
}
static int memRandomness(sqlite3_vfs *vfs, int nByte, char *zOut) {
    memset(zOut, 0, nByte);
    return SQLITE_OK;
}
static int memSleep(sqlite3_vfs *vfs, int microseconds) { return microseconds; }
static int memCurrentTime(sqlite3_vfs *vfs, double *pTime) {
    *pTime = 0.0;
    return SQLITE_OK;
}

static sqlite3_vfs mem_vfs = {
    .iVersion    = 1,
    .szOsFile    = sizeof(MemFile),
    .mxPathname  = 256,
    .zName       = "memvfs",
    .xOpen            = memOpen,
    .xDelete          = memDelete,
    .xAccess          = memAccess,
    .xFullPathname    = memFullPathname,
    .xRandomness      = memRandomness,
    .xSleep           = memSleep,
    .xCurrentTime     = memCurrentTime,
};

int sqlite3_os_init(void) {
    sqlite3_vfs_register(&mem_vfs, 1);
    return SQLITE_OK;
}
int sqlite3_os_end(void) { return SQLITE_OK; }
