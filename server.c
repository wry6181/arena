#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "base.h"
#include "error.h"
#include "log.h"

typedef struct {
    pthread_t* threads;
    u32 size;
} thread_pool;

typedef struct {
    s8* msg;
    volatile int* running;
} thread_ctx;

void* foo(void* args) {
    thread_ctx* ctx = args;
    pthread_t currentThread = pthread_self();
    while(*ctx->running) {
        printf("running %lu %.*s\n", (unsigned long)currentThread, STR8_FMT(*ctx->msg));
        sleep(1);
    }
    return NULL;
}

int main(void) {
    mem_arena* log_arena = arena_create(MByte(1));
    mem_arena* arena = arena_create(MByte(1));

    log_frame_begin(log_arena);

    pthread_t* threads = PUSH_ARRAY(arena, pthread_t, 5);

    thread_pool pool = {
        .threads = threads,
        .size = 5,
    };

    int running = 1;
    s8 msg = STR8_LIT("alma");

    thread_ctx ctx = {
        .msg = &msg,
        .running = &running,
    };

    int status;
    for(u32 i = 0; i < pool.size; ++i) {
        status = pthread_create(&pool.threads[i], NULL, foo, &ctx);
        if (status != 0) {
            ERROR_EMIT("error", OPEN_FILE_ERROR);
            exit(-1);
        }
    }

    sleep(5);

    WARNING_EMIT("main tread", OPEN_FILE_ERROR);
    running = 0;
    for(u32 i = 0; i < pool.size; ++i) {
        pthread_join(pool.threads[i], NULL);
    }

    log_print(log_arena, LOG_LEVEL_WARNING);

    arena_destroy(log_arena);
    arena_destroy(arena);

    return 0;
}
