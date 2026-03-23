#include "base.h"
#include <assert.h>
#include <stdio.h>

typedef struct {
  s8 name;
  f32 x;
  f32 y;
} entity;

void test_arena(void);
void test_str8(void);
void test_file_parse(void);

int main(void) {
  // test_arena();
  test_str8();
  // test_file_parse();
  return 0;
}

void test_file_parse(void) {
  FILE *f = fopen(__FILE__, "rb");
  assert(f != NULL);
  mem_arena *file_arena = arena_create(MByte(1));

  u32 size = fread(file_arena, Byte(1), MByte(1), f);

  s8 file_str = {
      .data = (u8 *)file_arena,
      .size = size,
  };

  while (file_str.size > 0) {
    s8 line = str8_slice_at(&file_str, '\n');
    printf("%.*s\n", STR8_FMT(line));
  }
}

void test_str8(void) {
  s8 str = STR8_LIT("alma");
  str8_cut_left(&str, 1);
  str8_cut_right(&str, 1);
  printf("[%.*s]\n", STR8_FMT(str));

  s8 str1 = STR8_LIT("  Hello  ");
  str8_trim(&str1);
  printf("[%.*s]\n", STR8_FMT(str1));

  s8 str3 = STR8_LIT("Cica, kutya");
  s8 str4 = str8_slice_at(&str3, ',');
  printf("[%.*s]\n", STR8_FMT(str3));
  printf("[%.*s]\n", STR8_FMT(str4));
}

void test_arena(void) {
  mem_arena *arena = arena_create(Byte(64));

  entity *e = PUSH_STRUCT_ZERO(arena, entity);

  ARENA_MEM_PRINT(arena);

  e->name = STR8_LIT("alma");

  e->x = 0.0;
  e->y = 1.0;

  ARENA_MEM_PRINT(arena);

  entity *e2 = PUSH_STRUCT_ZERO(arena, entity);

  printf("%p\n", (void *)e2);

  e2->name = STR8_LIT("alma");

  e2->x = 0.0;
  e2->y = 1.0;

  ARENA_MEM_PRINT(arena);

  e2->name = STR8_LIT("dog");

  ARENA_MEM_PRINT(arena);

  (void)PUSH_STRUCT_ZERO(arena, entity);

  arena_destroy(arena);
}
