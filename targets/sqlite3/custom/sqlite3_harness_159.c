// This fuzz driver is generated for library sqlite3, aiming to fuzz the following functions:
// sqlite3_memory_highwater at sqlite3.c:17467:26 in sqlite3.h
// sqlite3_memory_used at sqlite3.c:17456:26 in sqlite3.h
// sqlite3_open at sqlite3.c:175794:16 in sqlite3.h
// sqlite3_open at sqlite3.c:175794:16 in sqlite3.h
// sqlite3_open_v2 at sqlite3.c:175801:16 in sqlite3.h
// sqlite3_close at sqlite3.c:173433:16 in sqlite3.h
// sqlite3_db_status at sqlite3.c:11196:16 in sqlite3.h
// sqlite3_status at sqlite3.c:10929:16 in sqlite3.h
// sqlite3_status64 at sqlite3.c:10904:16 in sqlite3.h
// sqlite3_db_config at sqlite3.c:173019:16 in sqlite3.h
// sqlite3_db_config at sqlite3.c:173019:16 in sqlite3.h
// sqlite3_db_config at sqlite3.c:173019:16 in sqlite3.h
// sqlite3_db_config at sqlite3.c:173019:16 in sqlite3.h
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>

static void write_dummy_file(const uint8_t *Data, size_t Size) {
    FILE *fp = fopen("./dummy_file", "wb");
    if (fp) {
        fwrite(Data, 1, Size, fp);
        fclose(fp);
    }
}

static int get_int(const uint8_t *Data, size_t Size, size_t *offset) {
    int val = 0;
    if (*offset + 4 <= Size) {
        val = (int)((Data[*offset] << 24) | (Data[*offset + 1] << 16) |
                    (Data[*offset + 2] << 8) | Data[*offset + 3]);
        *offset += 4;
    } else if (*offset < Size) {
        // Use remaining bytes
        size_t rem = Size - *offset;
        for (size_t i = 0; i < rem; i++) {
            val = (val << 8) | Data[*offset + i];
        }
        *offset = Size;
    }
    return val;
}

static int get_bool(const uint8_t *Data, size_t Size, size_t *offset) {
    if (*offset < Size) {
        return Data[(*offset)++] & 1;
    }
    return 0;
}

static void fuzz_sqlite3_db_status(sqlite3 *db, const uint8_t *Data, size_t Size, size_t *offset) {
    int op = get_int(Data, Size, offset);
    int cur = 0, hiwtr = 0;
    int resetFlg = get_bool(Data, Size, offset);
    (void)sqlite3_db_status(db, op, &cur, &hiwtr, resetFlg);
}

static void fuzz_sqlite3_status(const uint8_t *Data, size_t Size, size_t *offset) {
    int op = get_int(Data, Size, offset);
    int cur = 0, hiwtr = 0;
    int resetFlg = get_bool(Data, Size, offset);
    (void)sqlite3_status(op, &cur, &hiwtr, resetFlg);
}

static void fuzz_sqlite3_status64(const uint8_t *Data, size_t Size, size_t *offset) {
    int op = get_int(Data, Size, offset);
    sqlite3_int64 cur = 0, hiwtr = 0;
    int resetFlg = get_bool(Data, Size, offset);
    (void)sqlite3_status64(op, &cur, &hiwtr, resetFlg);
}

static void fuzz_sqlite3_db_config(sqlite3 *db, const uint8_t *Data, size_t Size, size_t *offset) {
    int op = get_int(Data, Size, offset);
    // Try a few common config verbs with dummy arguments
    switch (op % 4) {
        case 0: {
            // SQLITE_DBCONFIG_LOOKASIDE
            char buf[32];
            memset(buf, 0, sizeof(buf));
            int sz = get_int(Data, Size, offset) & 0xFF;
            int cnt = get_int(Data, Size, offset) & 0xFF;
            (void)sqlite3_db_config(db, SQLITE_DBCONFIG_LOOKASIDE, buf, sz, cnt);
            break;
        }
        case 1: {
            // SQLITE_DBCONFIG_ENABLE_FKEY
            int enable = get_bool(Data, Size, offset);
            int old = 0;
            (void)sqlite3_db_config(db, SQLITE_DBCONFIG_ENABLE_FKEY, enable, &old);
            break;
        }
        case 2: {
            // SQLITE_DBCONFIG_ENABLE_TRIGGER
            int enable = get_bool(Data, Size, offset);
            int old = 0;
            (void)sqlite3_db_config(db, SQLITE_DBCONFIG_ENABLE_TRIGGER, enable, &old);
            break;
        }
        case 3: {
            // Pass random op and random arguments
            int arg1 = get_int(Data, Size, offset);
            int arg2 = get_int(Data, Size, offset);
            (void)sqlite3_db_config(db, op, arg1, arg2);
            break;
        }
    }
}

static void fuzz_sqlite3_memory_highwater(const uint8_t *Data, size_t Size, size_t *offset) {
    int resetFlg = get_bool(Data, Size, offset);
    (void)sqlite3_memory_highwater(resetFlg);
}

static void fuzz_sqlite3_memory_used(void) {
    (void)sqlite3_memory_used();
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 4) {
        return 0;
    }

    // Write the input to a dummy file for possible DB use
    write_dummy_file(Data, Size);

    sqlite3 *db = NULL;
    size_t offset = 0;

    // Choose open mode
    int mode = get_int(Data, Size, &offset) % 3;
    int rc = 0;

    if (mode == 0) {
        // Open in-memory DB
        rc = sqlite3_open(":memory:", &db);
    } else if (mode == 1) {
        // Open dummy file as DB
        rc = sqlite3_open("./dummy_file", &db);
    } else {
        // Open with open_v2, random flags
        int flags = get_int(Data, Size, &offset);
        rc = sqlite3_open_v2("./dummy_file", &db, flags, NULL);
    }
    if (rc != SQLITE_OK || db == NULL) {
        // Still fuzz the global APIs
        fuzz_sqlite3_status(Data, Size, &offset);
        fuzz_sqlite3_status64(Data, Size, &offset);
        fuzz_sqlite3_memory_highwater(Data, Size, &offset);
        fuzz_sqlite3_memory_used();
        return 0;
    }

    // Fuzz the APIs multiple times to explore states
    for (int i = 0; i < 2 && offset < Size; i++) {
        fuzz_sqlite3_db_status(db, Data, Size, &offset);
        fuzz_sqlite3_status(Data, Size, &offset);
        fuzz_sqlite3_status64(Data, Size, &offset);
        fuzz_sqlite3_db_config(db, Data, Size, &offset);
        fuzz_sqlite3_memory_highwater(Data, Size, &offset);
        fuzz_sqlite3_memory_used();
    }

    // Cleanup
    sqlite3_close(db);

    return 0;
}