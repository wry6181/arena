#include "base.h"
#include "thread_pool.h"
#include "types.h"
#include "error.h"
#include "log.h"
#include <cstdio>

typedef struct cJSON {
  struct cJSON *next; // Pointer to the next item in an array or object
  struct cJSON
      *child;        // Pointer to the first child (if this is an object/array)
  int type;          // Is this a string, number, or boolean?
  char *valuestring; // The actual data if it's a string
  char *string;      // The "key" name
} cJSON;

static inline s8 json_parse_s8(mem_arena *arena, const char *path) {
  // thread_pool* parse_pool = thread_pool_create(arena, 2, arena->capacity);
  //
  s8 result = {0};
  FILE *file = fopen(path, "rb");

  if (!file) {
      ERROR_EMIT("Json parse open file error", OPEN_FILE_ERROR);
      return result;
  }

  fseek(file, 0, SEEK_END);
  i32 size = ftell(file);
  fseek(file, 0, SEEK_SET);

  u8* buffer = PUSH_ARRAY_ZERO(arena, u8, size);



  fclose(file);
}

static inline s8 json_parse_from_s8(mem_arena *arena, s8 json_str) {}
