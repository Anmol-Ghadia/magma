// This fuzz driver is generated for library libtiff, aiming to fuzz the following functions:
// TIFFVGetField at tif_dir.c:1624:5 in tiffio.h
// TIFFVGetFieldDefaulted at tif_aux.c:308:5 in tiffio.h
// TIFFGetFieldDefaulted at tif_aux.c:477:5 in tiffio.h
// TIFFClose at tif_close.c:155:6 in tiffio.h
// TIFFOpen at tif_unix.c:233:7 in tiffio.h
// TIFFOpen at tif_unix.c:233:7 in tiffio.h
// TIFFReadEXIFDirectory at tif_dirread.c:5560:5 in tiffio.h
// TIFFReadGPSDirectory at tif_dirread.c:5568:5 in tiffio.h
// TIFFVSetField at tif_dir.c:1226:5 in tiffio.h
// TIFFVSetField at tif_dir.c:1226:5 in tiffio.h
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <tiffio.h>

static void write_dummy_file(const uint8_t *Data, size_t Size) {
    FILE *fp = fopen("./dummy_file", "wb");
    if (fp) {
        fwrite(Data, 1, Size, fp);
        fclose(fp);
    }
}

static uint32_t get_uint32_from_data(const uint8_t *Data, size_t Size, size_t *offset) {
    uint32_t val = 0;
    if (*offset + 4 <= Size) {
        val = ((uint32_t)Data[*offset]) |
              ((uint32_t)Data[*offset+1] << 8) |
              ((uint32_t)Data[*offset+2] << 16) |
              ((uint32_t)Data[*offset+3] << 24);
        *offset += 4;
    }
    return val;
}

static toff_t get_toff_t_from_data(const uint8_t *Data, size_t Size, size_t *offset) {
    toff_t val = 0;
    if (*offset + sizeof(toff_t) <= Size) {
        memcpy(&val, Data + *offset, sizeof(toff_t));
        *offset += sizeof(toff_t);
    }
    return val;
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 8)
        return 0;

    write_dummy_file(Data, Size);

    TIFF *tif = TIFFOpen("./dummy_file", "r+b");
    if (!tif) {
        tif = TIFFOpen("./dummy_file", "rb");
    }
    if (!tif) {
        return 0;
    }

    size_t offset = 0;

    // --- 1. TIFFReadEXIFDirectory ---
    if (offset + sizeof(toff_t) <= Size) {
        toff_t exif_diroff = get_toff_t_from_data(Data, Size, &offset);
        (void)TIFFReadEXIFDirectory(tif, exif_diroff);
    }

    // --- 2. TIFFReadGPSDirectory ---
    if (offset + sizeof(toff_t) <= Size) {
        toff_t gps_diroff = get_toff_t_from_data(Data, Size, &offset);
        (void)TIFFReadGPSDirectory(tif, gps_diroff);
    }

    // --- 3. TIFFVSetField ---
    // Pick a tag and value from fuzz data
    if (offset + 8 <= Size) {
        uint32_t tag = get_uint32_from_data(Data, Size, &offset);
        uint32_t value = get_uint32_from_data(Data, Size, &offset);

        // For test, treat value as uint32_t and pass as va_list
        va_list ap;
        va_list ap_copy;
        // Use a hack to initialize va_list with a single value
        // This is not portable, but for fuzzing is acceptable
        // Use a struct to pass as va_list
        struct {
            uint32_t v;
        } hack;
        hack.v = value;
        memcpy(&ap, &hack, sizeof(hack));
        memcpy(&ap_copy, &hack, sizeof(hack));
        (void)TIFFVSetField(tif, tag, ap);
        (void)TIFFVSetField(tif, tag, ap_copy);
    }

    // --- 4. TIFFVGetField ---
    if (offset + 4 <= Size) {
        uint32_t tag = get_uint32_from_data(Data, Size, &offset);

        // Prepare a pointer to store the result
        uint32_t out_val = 0;
        va_list ap;
        va_list ap_copy;
        // Hack: va_list is just a pointer to our variable
        void *args[1];
        args[0] = &out_val;
        memcpy(&ap, &args, sizeof(args));
        memcpy(&ap_copy, &args, sizeof(args));
        (void)TIFFVGetField(tif, tag, ap);
        (void)TIFFVGetFieldDefaulted(tif, tag, ap_copy);
    }

    // --- 5. TIFFGetFieldDefaulted ---
    if (offset + 4 <= Size) {
        uint32_t tag = get_uint32_from_data(Data, Size, &offset);
        uint32_t out_val = 0;
        (void)TIFFGetFieldDefaulted(tif, tag, &out_val);
    }

    TIFFClose(tif);

    return 0;
}