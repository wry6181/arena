#include "base.h"
#include "types.h"
#include "thread_pool.h"

typedef struct cJSON {
    struct cJSON *next;     // Pointer to the next item in an array or object
    struct cJSON *child;    // Pointer to the first child (if this is an object/array)
    int type;               // Is this a string, number, or boolean?
    char *valuestring;      // The actual data if it's a string
    char *string;           // The "key" name
} cJSON;

static inline s8 json_parse_s8(mem_arena* arena, const char* path) {
    //thread_pool* parse_pool = thread_pool_create(arena, 2, arena->capacity);
    FILE* file = fopen(path, "rb");

    fclose(file);
}

static inline s8 json_parse_from_s8(mem_arena* arena, s8 json_str){


}
