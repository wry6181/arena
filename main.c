#include "arena.h"


typedef struct {
    s8 name;
    f32 x;
    f32 y;
} entity;


int main(void) {
    mem_arena* arena = arena_create(Byte(64));

    entity* e = PUSH_STRUCT_ZERO(arena, entity);

    ARENA_MEM_PRINT(arena);

    e->name = STR8_LIT("alma");

    e->x = 0.0;
    e->y = 1.0;

    ARENA_MEM_PRINT(arena);


    entity* e2 = PUSH_STRUCT_ZERO(arena, entity);

    printf("%p\n", e2);

    e2->name = STR8_LIT("alma");

    e2->x = 0.0;
    e2->y = 1.0;

    ARENA_MEM_PRINT(arena);


    e2->name = STR8_LIT("dog");

    ARENA_MEM_PRINT(arena);

    entity* e3 = PUSH_STRUCT_ZERO(arena, entity);

    arena_destroy(arena);
}
