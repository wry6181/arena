#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef float f32;
typedef i32 b32;
typedef i8 b8;

#define Byte(n) (u64)(n)
#define KByte(n) ((u64)(n) << 10)
#define MByte(n) ((u64)(n) << 20)
#define GByte(n) ((u64)(n) << 30)

#define ALIGN_UP_POW2(n, p) (((u64)(n) + ((u64)(p) - 1)) & (~((u64)(p) - 1)))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define AREANA_BASE_POS (sizeof(mem_arena))
#define ARENA_ALIGN (sizeof(void *))

typedef struct {
  u64 capacty;
  u64 pos;
} mem_arena;

typedef struct {
  u8 *str;
  u64 size;
} string_8;

#define STR8_FMT(str8) (int)(str8).size, (str8).str
#define STR8_LIT(str)                                                          \
  (string_8) { (u8 *)(str), (sizeof((str)) - 1) }

#define ARENA_MEM_PRINT(x) arena_print_mem(STR8_LIT(#x), x)

#define PUSH_STRUCT(arena, T) (T *)arena_push((arena), sizeof(T), false)
#define PUSH_STRUCT_ZERO(arena, T) (T *)arena_push((arena), sizeof(T), true)
#define PUSH_ARRAY(arena, T, n) (T *)arena_push((arena), sizeof(T) * n, false)
#define PUSH_ARRAY_ZERO(arena, T, n)                                           \
  (T *)arena_push((arena), sizeof(T) * n, true)

string_8 str8_substr(string_8 str, u64 start, u64 end) {
  end = MIN(end, str.size);
  start = MIN(start, end);
  return (string_8){.str = str.str + start, .size = end - start};
}

mem_arena *arena_create(u64 capacity) {
  mem_arena *arena = (mem_arena *)malloc(capacity);
  arena->capacty = capacity;
  arena->pos = AREANA_BASE_POS;
  return arena;
}
void arena_destroy(mem_arena *arena) { free(arena); }
void *arena_push(mem_arena *arena, u64 size, b32 non_zero) {
  u64 pos_align = ALIGN_UP_POW2(arena->pos, ARENA_ALIGN);
  u64 new_pos = pos_align + size;
  if (new_pos > arena->capacty) {
    return NULL;
  }
  arena->pos = new_pos;
  u8 *out = (u8 *)arena + pos_align;
  if (non_zero) {
    memset(out, 0, size);
  }
  return out;
}
void arena_pop(mem_arena *arena, u64 size) {
  size = MIN(size, arena->pos - AREANA_BASE_POS);
  arena->pos -= size;
}
int arena_clone(mem_arena *arena_from, mem_arena *arena_to) {
  u64 from_used = arena_from->pos - AREANA_BASE_POS;
  printf("from used %llu \n", from_used);
  u64 to_free = arena_to->capacty - arena_to->pos;
  printf("from free %llu \n", to_free);

  if (to_free < from_used) {
    return 1;
  }
  memcpy((u8 *)arena_to + arena_to->pos, (u8 *)arena_from + AREANA_BASE_POS,
         from_used);
  arena_to += from_used;
  return 0;
}
void arena_pop_to(mem_arena *arena, u64 pos) {
  u64 size = (pos < arena->pos) ? arena->pos - pos : 0;
  arena_pop(arena, size);
}
void arena_clear(mem_arena *arena) { arena_pop_to(arena, AREANA_BASE_POS); }
void arena_print_mem(string_8 name, mem_arena *arena) {
  u8 *base = (u8 *)arena;
  printf("this arena: %.*s \n", STR8_FMT(name));
  for (u64 i = 0; i < arena->capacty; ++i) {
    printf("%02X ", base[i]);
    if ((i + 1) % 8 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}
