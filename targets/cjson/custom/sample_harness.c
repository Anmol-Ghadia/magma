// This fuzz driver is generated for library cjson, aiming to fuzz the following functions:
// cJSON_CreateObject at cJSON.c:2567:23 in cJSON.h
// cJSON_CreateObject at cJSON.c:2567:23 in cJSON.h
// cJSON_CreateArray at cJSON.c:2556:23 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_CreateNull at cJSON.c:2419:23 in cJSON.h
// cJSON_CreateNull at cJSON.c:2419:23 in cJSON.h
// cJSON_CreateNull at cJSON.c:2419:23 in cJSON.h
// cJSON_CreateNull at cJSON.c:2419:23 in cJSON.h
// cJSON_CreateNull at cJSON.c:2419:23 in cJSON.h
// cJSON_AddItemToObject at cJSON.c:2077:26 in cJSON.h
// cJSON_AddItemToObject at cJSON.c:2077:26 in cJSON.h
// cJSON_AddItemToObjectCS at cJSON.c:2083:26 in cJSON.h
// cJSON_AddItemToObjectCS at cJSON.c:2083:26 in cJSON.h
// cJSON_AddItemToObjectCS at cJSON.c:2083:26 in cJSON.h
// cJSON_AddItemReferenceToArray at cJSON.c:2088:26 in cJSON.h
// cJSON_AddItemReferenceToArray at cJSON.c:2088:26 in cJSON.h
// cJSON_AddItemReferenceToObject at cJSON.c:2098:26 in cJSON.h
// cJSON_AddItemReferenceToObject at cJSON.c:2098:26 in cJSON.h
// cJSON_AddItemReferenceToObject at cJSON.c:2098:26 in cJSON.h
// cJSON_DeleteItemFromArray at cJSON.c:2262:20 in cJSON.h
// cJSON_DeleteItemFromObject at cJSON.c:2281:20 in cJSON.h
// cJSON_DeleteItemFromObject at cJSON.c:2281:20 in cJSON.h
// cJSON_DeleteItemFromObjectCaseSensitive at cJSON.c:2286:20 in cJSON.h
// cJSON_DeleteItemFromObjectCaseSensitive at cJSON.c:2286:20 in cJSON.h
// cJSON_Minify at cJSON.c:2882:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_Delete at cJSON.c:253:20 in cJSON.h
// cJSON_CreateNull at cJSON.c:2419:23 in cJSON.h
// cJSON_CreateBool at cJSON.c:2452:23 in cJSON.h
// cJSON_CreateNumber at cJSON.c:2463:23 in cJSON.h
// cJSON_CreateString at cJSON.c:2489:23 in cJSON.h
// cJSON_CreateArray at cJSON.c:2556:23 in cJSON.h
// cJSON_CreateObject at cJSON.c:2567:23 in cJSON.h
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

static char *make_mutable_string(const uint8_t *Data, size_t Size)
{
    char *buf = (char *)malloc(Size + 1);
    if (buf == NULL) {
        return NULL;
    }
    if (Size > 0) {
        memcpy(buf, Data, Size);
    }
    buf[Size] = '\0';
    return buf;
}

static char *make_key(const uint8_t *Data, size_t Size, size_t *offset, size_t max_len)
{
    size_t i, len;
    char *key;

    if (max_len == 0) {
        max_len = 1;
    }

    if (*offset >= Size) {
        len = 0;
    } else {
        len = Data[*offset] % (max_len + 1);
        (*offset)++;
        if (len > Size - *offset) {
            len = Size - *offset;
        }
    }

    key = (char *)malloc(len + 1);
    if (key == NULL) {
        return NULL;
    }

    for (i = 0; i < len; i++) {
        unsigned char c = Data[*offset + i];
        if (c == '\0') {
            c = 'A';
        }
        key[i] = (char)c;
    }
    key[len] = '\0';
    *offset += len;
    return key;
}

static cJSON *make_item_from_data(const uint8_t *Data, size_t Size, size_t *offset)
{
    unsigned char selector = 0;

    if (*offset < Size) {
        selector = Data[*offset];
        (*offset)++;
    }

    switch (selector % 6) {
        case 0:
            return cJSON_CreateNull();
        case 1:
            return cJSON_CreateBool((selector & 1) ? 1 : 0);
        case 2:
            return cJSON_CreateNumber((double)(selector));
        case 3:
        {
            char *s = make_key(Data, Size, offset, 16);
            cJSON *item = cJSON_CreateString(s ? s : "");
            free(s);
            return item;
        }
        case 4:
            return cJSON_CreateArray();
        default:
            return cJSON_CreateObject();
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    size_t offset = 0;
    char *key1 = NULL, *key2 = NULL;
    char *delkey1 = NULL, *delkey2 = NULL;
    char *delkey3 = NULL, *delkey4 = NULL;
    char *minify_buf = NULL;

    cJSON *obj1 = cJSON_CreateObject();
    cJSON *obj2 = cJSON_CreateObject();
    cJSON *arr1 = cJSON_CreateArray();

    cJSON *item1 = NULL, *item2 = NULL, *item3 = NULL, *item4 = NULL, *item5 = NULL;

    static const char *const_key1 = "const_key_1";
    static const char *const_key2 = "CONST_KEY_2";
    static const char *const_key3 = "CaseKey";

    if (obj1 == NULL || obj2 == NULL || arr1 == NULL) {
        cJSON_Delete(obj1);
        cJSON_Delete(obj2);
        cJSON_Delete(arr1);
        return 0;
    }

    key1 = make_key(Data, Size, &offset, 16);
    key2 = make_key(Data, Size, &offset, 16);
    delkey1 = make_key(Data, Size, &offset, 16);
    delkey2 = make_key(Data, Size, &offset, 16);
    delkey3 = make_key(Data, Size, &offset, 16);
    delkey4 = make_key(Data, Size, &offset, 16);

    item1 = make_item_from_data(Data, Size, &offset);
    item2 = make_item_from_data(Data, Size, &offset);
    item3 = make_item_from_data(Data, Size, &offset);
    item4 = make_item_from_data(Data, Size, &offset);
    item5 = make_item_from_data(Data, Size, &offset);

    if (item1 == NULL) item1 = cJSON_CreateNull();
    if (item2 == NULL) item2 = cJSON_CreateNull();
    if (item3 == NULL) item3 = cJSON_CreateNull();
    if (item4 == NULL) item4 = cJSON_CreateNull();
    if (item5 == NULL) item5 = cJSON_CreateNull();

    cJSON_AddItemToObject(obj1, key1 ? key1 : "", item1);
    item1 = NULL;

    cJSON_AddItemToObject(obj1, key2 ? key2 : "", item2);
    item2 = NULL;

    cJSON_AddItemToObjectCS(obj1, const_key1, item3);
    item3 = NULL;

    cJSON_AddItemToObjectCS(obj1, const_key2, item4);
    item4 = NULL;

    cJSON_AddItemToObjectCS(obj1, const_key3, item5);
    item5 = NULL;

    cJSON_AddItemReferenceToArray(arr1, obj1->child);
    cJSON_AddItemReferenceToArray(arr1, obj1);

    cJSON_AddItemReferenceToObject(obj2, "ref1", obj1);
    cJSON_AddItemReferenceToObject(obj2, "ref2", obj1->child);
    cJSON_AddItemReferenceToObject(obj2, "ref3", arr1);

    {
        int which = 0;
        if (offset < Size) {
            which = (int)((int8_t)Data[offset]);
            offset++;
        }
        cJSON_DeleteItemFromArray(arr1, which);
    }

    cJSON_DeleteItemFromObject(obj1, delkey1 ? delkey1 : "");
    cJSON_DeleteItemFromObject(obj1, delkey2 ? delkey2 : "");
    cJSON_DeleteItemFromObjectCaseSensitive(obj1, delkey3 ? delkey3 : "");
    cJSON_DeleteItemFromObjectCaseSensitive(obj1, delkey4 ? delkey4 : "");

    minify_buf = make_mutable_string(Data, Size);
    cJSON_Minify(minify_buf);

    cJSON_Delete(obj2);
    cJSON_Delete(arr1);
    cJSON_Delete(obj1);

    free(minify_buf);
    free(key1);
    free(key2);
    free(delkey1);
    free(delkey2);
    free(delkey3);
    free(delkey4);

    cJSON_Delete(item1);
    cJSON_Delete(item2);
    cJSON_Delete(item3);
    cJSON_Delete(item4);
    cJSON_Delete(item5);

    return 0;
}