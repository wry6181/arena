#pragma once

#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN_UP_POW2(n, p) (((u64)(n) + ((u64)(p) - 1)) & (~((u64)(p) - 1)))
#define AREANA_BASE_POS (sizeof(mem_arena))
#define ARENA_ALIGN (sizeof(void *))

typedef struct mem_arena mem_arena;
static inline void *arena_push(mem_arena *arena, u64 size, b32 clear);

typedef struct {
  u8 *data;
  u64 size;
} s8;

#define STR8_FMT(str8) (int)(str8).size, (str8).data
#define STR8_LIT(str)                                                          \
  (s8) { (u8 *)(str), (sizeof((str)) - 1) }

static inline s8 str8_substr(s8 *str, u64 start, u64 end) {
  end = MIN(end, str->size);
  start = MIN(start, end);
  return (s8){.data = str->data + start, .size = end - start};
}

static inline s8 str8_append(mem_arena *arena, s8 a, s8 b) {
  s8 out = {0};
  out.size = a.size + b.size;
  out.data = (u8 *)arena_push(arena, out.size, false);

  if (!out.data) {
    return out;
  }
  memcpy(out.data, a.data, a.size);
  memcpy(out.data + a.size, b.data, b.size);
  return out;
}

static inline s8 str8_to_cstr(mem_arena *arena, s8 a) {
  s8 out = {0};
  out.size = a.size + 1;
  out.data = (u8 *)arena_push(arena, out.size, false);
  if (!out.data) {
    return out;
  }
  memcpy(out.data, a.data, a.size);
  out.data[a.size] = '\0';
  return out;
}

static inline void str8_cut_left(s8 *str, u64 amount) {
  if (str->size < amount) {
    return;
  }
  str->data += amount;
  str->size -= amount;
}

static inline void str8_cut_right(s8 *str, u64 amount) {
  if (str->size < amount) {
    return;
  }
  str->size -= amount;
}

static inline void str8_trim(s8 *str) {
  while (0 < str->size && str->data[0] == ' ') {
    str8_cut_left(str, 1);
  }
  while (0 < str->size && str->data[str->size - 1] == ' ') {
    str8_cut_right(str, 1);
  }
}

static inline s8 str8_slice_at(s8 *str, char c) {
  s8 result = {0};
  for (u64 i = 0; i < str->size; ++i) {
    if (str->data[i] == c) {
      result.data = &str->data[0];
      result.size = i;
      str8_cut_left(str, i + 1);
      return result;
    }
  }
  return *str;
}

static inline b32 str8_eq(s8 a, s8 b) {
  if (a.size != b.size)
    return false;
  if (a.size == 0)
    return true;
  return memcmp(a.data, b.data, a.size) == 0;
}

static inline s8 str8_copy(mem_arena *arena, s8 s) {
  s8 out = {0};
  out.data = (u8 *)arena_push(arena, s.size, false);
  if (!out.data)
    return out;

  out.size = s.size;
  memcpy(out.data, s.data, s.size);
  return out;
}

/// Arena
///
struct mem_arena {
  u64 capacity;
  u64 pos;
  mem_arena *next;
};

#define ARENA_MEM_PRINT(x) arena_print_mem(STR8_LIT(#x), x)

#define PUSH_STRUCT(arena, T) (T *)arena_push((arena), sizeof(T), false)
#define PUSH_STRUCT_ZERO(arena, T) (T *)arena_push((arena), sizeof(T), true)
#define PUSH_ARRAY(arena, T, n) (T *)arena_push((arena), sizeof(T) * n, false)
#define PUSH_ARRAY_ZERO(arena, T, n)                                           \
  (T *)arena_push((arena), sizeof(T) * n, true)

static inline void *arena_push(mem_arena *arena, u64 size, b32 clear);

static inline mem_arena *arena_create(u64 capacity) {
  mem_arena *arena = (mem_arena *)malloc(capacity);
  arena->capacity = capacity;
  arena->pos = AREANA_BASE_POS;
  arena->next = NULL;
  return arena;
}

static inline void arena_destroy(mem_arena *arena) {
  while (arena) {
    mem_arena *next = arena->next;
    free(arena);
    arena = next;
  }
}

static inline void *arena_push(mem_arena *arena, u64 size, b32 clear) {

  if (arena == NULL) {
    fprintf(stderr, "Fatal: arena_push called with NULL arena!\n");
    return NULL;
  }

  while (arena->next != NULL) {
    arena = arena->next;
  }

  u64 pos_align = ALIGN_UP_POW2(arena->pos, ARENA_ALIGN);
  u64 new_pos = pos_align + size;

  if (new_pos > arena->capacity) {
    fprintf(stderr, "Warning: Arena capacity exceeded!\n");
    arena->next = arena_create(arena->capacity);
    u8 *out = (u8 *)arena_push(arena->next, size, clear);
    return out;
  }

  arena->pos = new_pos;
  u8 *out = (u8 *)arena + pos_align;
  if (clear) {
    memset(out, 0, size);
  }
  return out;
}

static inline void arena_pop(mem_arena *arena, u64 size) {
  size = MIN(size, arena->pos - AREANA_BASE_POS);
  arena->pos -= size;
}

static inline void arena_pop_to(mem_arena *arena, u64 pos) {
  u64 size = (pos < arena->pos) ? arena->pos - pos : 0;
  arena_pop(arena, size);
}

static inline void arena_clear(mem_arena *arena) {
  arena_pop_to(arena, AREANA_BASE_POS);
}

static inline void arena_print_mem(s8 name, mem_arena *arena) {
  u8 *base = (u8 *)arena;
  printf("this arena: %.*s \n", STR8_FMT(name));
  for (u64 i = 0; i < arena->pos; ++i) {
    printf("%02X ", base[i]);
    if ((i + 1) % 8 == 0) {
      printf("\n");
    }
  }
  printf("\n");
  if (arena->next != NULL) {
    s8 next_name = STR8_LIT("next -> \n");
    arena_print_mem(next_name, arena->next);
  }
}
