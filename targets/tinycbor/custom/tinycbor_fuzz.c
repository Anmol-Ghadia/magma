/*
 * fuzz_cbor.c — libFuzzer-style harness for tinycbor, driven by AFL++.
 *
 * The LLVMFuzzerTestOneInput entry point is called by libAFLDriver.a, which
 * provides main(), persistent-mode looping, and shared-memory coverage bitmap
 * updates — no libFuzzer runtime required.
 *
 * Build:  make
 * Run:    make corpus && make run
 *         or manually: afl-fuzz -m none -i corpus -o output -- ./fuzz_cbor @@
 *
 * Surface covered:
 *   1. cbor_parser_init + full recursive value traversal (all CBOR types)
 *   2. cbor_value_validate (basic / canonical / strict / strictest)
 *   3. cbor_value_to_pretty_stream (two flag combos)
 *   4. cbor_value_to_json_advance (two flag combos)
 *   5. cbor_value_map_find_value
 *   6. cbor_value_skip_tag / cbor_value_advance
 *   7. String chunk iteration (indefinite-length strings)
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "cbor.h"
#include "cborjson.h"

/* Maximum container nesting we will recurse into.
 * The tinycbor parser itself limits to CBOR_PARSER_MAX_RECURSIONS (1024). */
#define MAX_DEPTH 32

/* -------------------------------------------------------------------------
 * Discard-sink stream for cbor_value_to_pretty_stream.
 * We only care about exercising code paths, not capturing output.
 * ------------------------------------------------------------------------- */
static CborError discard_stream(void *token, const char *fmt, ...)
{
    (void)token;
    (void)fmt;
    return CborNoError;
}

/* -------------------------------------------------------------------------
 * traverse() — walk every value in an already-initialised iterator,
 * calling all relevant getter APIs on each type.
 *
 * Returns true if the iterator was fully consumed (normal exit or error
 * that consumed the remaining input), false if we bailed early (e.g.
 * depth limit).  Callers that get false must also stop iterating,
 * because the parent iterator position is undefined.
 * ------------------------------------------------------------------------- */
static bool traverse(CborValue *it, int depth);

/* Returns true if the container was fully entered, traversed, and left.
 * Returns false on enter/leave failure or if the inner traversal stopped
 * early; in that case *it must be treated as invalid by the caller. */
static bool traverse_container(CborValue *it, int depth)
{
    size_t len = 0;

    if (cbor_value_is_array(it))
        cbor_value_get_array_length(it, &len);
    else
        cbor_value_get_map_length(it, &len);

    CborValue child;
    if (cbor_value_enter_container(it, &child) != CborNoError)
        return false;

    bool done = traverse(&child, depth + 1);

    /* cbor_value_leave_container asserts child.type == CborInvalidType,
     * which is only true when the inner iterator was fully consumed.
     * If traverse() stopped early (depth guard), skip leave_container and
     * signal the caller to stop too. */
    if (!done || child.type != CborInvalidType)
        return false;

    if (cbor_value_leave_container(it, &child) != CborNoError)
        return false;

    return true;
}

static bool traverse(CborValue *it, int depth)
{
    if (depth > MAX_DEPTH)
        return false;

    while (!cbor_value_at_end(it)) {
        CborError  err;
        CborType   type = cbor_value_get_type(it);

        switch (type) {

        /* --- containers -------------------------------------------------- */
        case CborArrayType:
        case CborMapType:
            if (!traverse_container(it, depth))
                return false;  /* inner traversal stopped early; propagate */
            continue;  /* leave_container already advanced it */

        /* --- integers ----------------------------------------------------- */
        case CborIntegerType: {
            uint64_t u64 = 0;
            int64_t  i64 = 0;
            int      i   = 0;
            cbor_value_get_raw_integer(it, &u64);
            cbor_value_get_int64(it, &i64);
            cbor_value_get_int64_checked(it, &i64);
            cbor_value_get_int_checked(it, &i);
            (void)cbor_value_is_unsigned_integer(it);
            (void)cbor_value_is_negative_integer(it);
            break;
        }

        /* --- byte strings ------------------------------------------------- */
        case CborByteStringType: {
            size_t    n    = 0;
            size_t    calc = 0;
            uint8_t  *buf  = NULL;

            cbor_value_get_string_length(it, &n);
            cbor_value_calculate_string_length(it, &calc);

            err = cbor_value_dup_byte_string(it, &buf, &n, it);
            if (err == CborNoError) {
                free(buf);
                continue;  /* dup successfully advanced it */
            }
            /* On error, dup may not have advanced it — stop to avoid looping */
            return false;
        }

        /* --- text strings ------------------------------------------------- */
        case CborTextStringType: {
            size_t  n    = 0;
            size_t  calc = 0;
            char   *buf  = NULL;
            bool    eq   = false;

            cbor_value_get_string_length(it, &n);
            cbor_value_calculate_string_length(it, &calc);

            /* text_string_equals takes const CborValue* — does not advance */
            cbor_value_text_string_equals(it, "", &eq);
            cbor_value_text_string_equals(it, "test", &eq);
            cbor_value_text_string_equals(it, "key", &eq);

            err = cbor_value_dup_text_string(it, &buf, &n, it);
            if (err == CborNoError) {
                free(buf);
                continue;  /* dup successfully advanced it */
            }
            /* On error, dup may not have advanced it — stop to avoid looping */
            return false;
        }

        /* --- tags --------------------------------------------------------- */
        case CborTagType: {
            CborTag tag = 0;
            cbor_value_get_tag(it, &tag);
            /*
             * advance_fixed moves past the tag header only, so the next
             * loop iteration will see and process the tagged value.
             * This matches the simplereader example and exercises the
             * tag-header parsing path without hiding the tagged payload.
             */
            break;
        }

        /* --- booleans ----------------------------------------------------- */
        case CborBooleanType: {
            bool val = false;
            cbor_value_get_boolean(it, &val);
            break;
        }

        /* --- simple types ------------------------------------------------- */
        case CborSimpleType: {
            uint8_t sval = 0;
            cbor_value_get_simple_type(it, &sval);
            break;
        }

        /* --- half-precision float ----------------------------------------- */
        case CborHalfFloatType: {
            uint16_t hval = 0;
            float    fval = 0.0f;
            cbor_value_get_half_float(it, &hval);
            cbor_value_get_half_float_as_float(it, &fval);
            break;
        }

        /* --- single-precision float --------------------------------------- */
        case CborFloatType: {
            float fval = 0.0f;
            cbor_value_get_float(it, &fval);
            break;
        }

        /* --- double-precision float --------------------------------------- */
        case CborDoubleType: {
            double dval = 0.0;
            cbor_value_get_double(it, &dval);
            break;
        }

        case CborNullType:
        case CborUndefinedType:
            break;

        case CborInvalidType:
            return false;
        }

        /* Advance past fixed-size (non-container, non-string) values.
         * For containers/strings we 'continue' above after the call that
         * already advances the iterator. */
        err = cbor_value_advance_fixed(it);
        if (err != CborNoError)
            return false;
    }
    return true;
}

/* -------------------------------------------------------------------------
 * Exercise indefinite-length string chunk iteration API.
 * ------------------------------------------------------------------------- */
static void exercise_string_chunks(const uint8_t *data, size_t size)
{
    CborParser parser;
    CborValue  it;

    if (cbor_parser_init(data, size, 0, &parser, &it) != CborNoError)
        return;

    if (cbor_value_at_end(&it))
        return;

    CborType type = cbor_value_get_type(&it);
    if (type != CborTextStringType && type != CborByteStringType)
        return;

    /* Only indefinite-length strings have meaningful chunk iteration */
    if (cbor_value_is_length_known(&it))
        return;

    CborValue chunked = it;
    if (cbor_value_begin_string_iteration(&chunked) != CborNoError)
        return;

    while (!cbor_value_string_iteration_at_end(&chunked)) {
        const void *chunk_ptr = NULL;
        size_t      chunk_len = 0;

        if (type == CborTextStringType) {
            CborValue next = chunked;
            if (_cbor_value_get_string_chunk(&chunked,
                                             &chunk_ptr, &chunk_len,
                                             &next) != CborNoError)
                break;
            chunked = next;
        } else {
            CborValue next = chunked;
            if (_cbor_value_get_string_chunk(&chunked,
                                             &chunk_ptr, &chunk_len,
                                             &next) != CborNoError)
                break;
            chunked = next;
        }
    }
}

/* -------------------------------------------------------------------------
 * Exercise cbor_value_skip_tag and cbor_value_advance on a fresh iterator.
 * ------------------------------------------------------------------------- */
static void exercise_skip_and_advance(const uint8_t *data, size_t size)
{
    CborParser parser;
    CborValue  it;

    if (cbor_parser_init(data, size, 0, &parser, &it) != CborNoError)
        return;

    /* Walk the top level using the high-level cbor_value_advance(), which
     * skips entire containers and tagged pairs in one call. */
    while (!cbor_value_at_end(&it)) {
        CborError err;

        if (cbor_value_is_tag(&it)) {
            /* skip_tag positions us after the tagged value */
            err = cbor_value_skip_tag(&it);
        } else {
            err = cbor_value_advance(&it);
        }

        if (err != CborNoError)
            break;
    }
}

/* -------------------------------------------------------------------------
 * LLVMFuzzerTestOneInput — called for every fuzz input by libAFLDriver.a.
 * Must return 0; returning non-zero tells the driver the input is invalid
 * and should not be added to the corpus (we always return 0 here to let
 * AFL++ explore all mutants, valid or not).
 * ------------------------------------------------------------------------- */
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    CborParser parser;
    CborValue  it;

    /* JSON output sink: fixed-size in-memory buffer avoids syscalls.
     * Opened once; rewound before each call so it never "fills up". */
    static char  json_buf[65536];
    static FILE *json_fp = NULL;
    if (!json_fp)
        json_fp = fmemopen(json_buf, sizeof(json_buf), "w");

    /* -----------------------------------------------------------------
     * 1. Parse + full recursive traversal (exercises all getter APIs)
     * ----------------------------------------------------------------- */
    if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError)
        traverse(&it, 0);

    /* -----------------------------------------------------------------
     * 2. Validation — basic (structural check only)
     * ----------------------------------------------------------------- */
    if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError)
        cbor_value_validate_basic(&it);

    /* -----------------------------------------------------------------
     * 3. Validation — canonical CBOR (deterministic encoding rules)
     * ----------------------------------------------------------------- */
    if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError)
        cbor_value_validate(&it, CborValidateCanonicalFormat);

    /* -----------------------------------------------------------------
     * 4. Validation — strict mode (UTF-8, tag correctness, etc.)
     * ----------------------------------------------------------------- */
    if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError)
        cbor_value_validate(&it, CborValidateStrictMode);

    /* -----------------------------------------------------------------
     * 5. Validation — strictest (all flags on)
     * ----------------------------------------------------------------- */
    if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError)
        cbor_value_validate(&it, CborValidateStrictest);

    /* -----------------------------------------------------------------
     * 6. Pretty-print to discard stream — default flags
     * ----------------------------------------------------------------- */
    if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError)
        cbor_value_to_pretty_stream(discard_stream, NULL, &it,
                                    CborPrettyDefaultFlags);

    /* -----------------------------------------------------------------
     * 7. Pretty-print — verbose flags (numeric indicators, fragments)
     * ----------------------------------------------------------------- */
    if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError)
        cbor_value_to_pretty_stream(discard_stream, NULL, &it,
                                    CborPrettyNumericEncodingIndicators |
                                    CborPrettyIndicateIndeterminateLength |
                                    CborPrettyIndicateOverlongNumbers   |
                                    CborPrettyShowStringFragments);

    /* -----------------------------------------------------------------
     * 8. JSON conversion — default flags
     * ----------------------------------------------------------------- */
    if (json_fp) {
        rewind(json_fp);
        if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError)
            cbor_value_to_json_advance(json_fp, &it, CborConvertDefaultFlags);
    }

    /* -----------------------------------------------------------------
     * 9. JSON conversion — all metadata / stringify flags
     * ----------------------------------------------------------------- */
    if (json_fp) {
        rewind(json_fp);
        if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError)
            cbor_value_to_json_advance(json_fp, &it,
                                       CborConvertAddMetadata           |
                                       CborConvertTagsToObjects          |
                                       CborConvertStringifyMapKeys       |
                                       CborConvertByteStringsToBase64Url);
    }

    /* -----------------------------------------------------------------
     * 10. Map key lookup (exercises map traversal / key comparison)
     * ----------------------------------------------------------------- */
    if (cbor_parser_init(data, size, 0, &parser, &it) == CborNoError) {
        if (cbor_value_is_map(&it)) {
            CborValue found;
            cbor_value_map_find_value(&it, "",     &found);
            cbor_value_map_find_value(&it, "key",  &found);
            cbor_value_map_find_value(&it, "test", &found);
            cbor_value_map_find_value(&it, "a",    &found);
        }
    }

    /* -----------------------------------------------------------------
     * 11. Chunk iteration for indefinite-length strings
     * ----------------------------------------------------------------- */
    exercise_string_chunks(data, size);

    /* -----------------------------------------------------------------
     * 12. cbor_value_skip_tag + cbor_value_advance (top-level walk)
     * ----------------------------------------------------------------- */
    exercise_skip_and_advance(data, size);

    return 0;
}
