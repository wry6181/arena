#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>


#include <stdatomic.h>
#include <stdatomic.h>
#include "base.h"
#include "log.h"

typedef struct {
    pthread_t* threads;
    mem_arena** arenas;
    u32 size;
} thread_pool;

typedef struct {
    s8* msg;
    au32* running;
} thread_ctx;

typedef void (*job_fn)(void* data, mem_arena* arena);

typedef struct {
    job_fn fn;
    void* data;
} job;


typedef struct {
    u32 n;
    u64 result;
} fib_job_data;

typedef struct {
    mem_arena* arena;
    job* job;
} worker_ctx;

typedef struct {
    job* jobs;
    u32 capacity;
    au32 read_index;
    au32 write_index;
} job_queue;

thread_pool* thread_pool_create(mem_arena* arena, u32 pool_size, u64 local_arena_size) {
    pthread_t* threads = PUSH_ARRAY_ZERO(arena, pthread_t, pool_size);
    thread_pool* pool = PUSH_STRUCT_ZERO(arena, thread_pool);
    mem_arena** thread_local_arenas = PUSH_ARRAY_ZERO(arena, mem_arena*, pool_size);

    for(u32 i = 0; i < pool_size; ++i) {
        thread_local_arenas[i] = arena_create(local_arena_size);
    }

    pool->threads = threads;
    pool->arenas = thread_local_arenas;
    pool->size = pool_size;
    return pool;
}

void* worker_entry(void* arg) {
    worker_ctx* ctx = arg;

    job* j = ctx->job;
    j->fn(j->data, ctx->arena);

    return NULL;
}

void thread_pool_run(mem_arena* arena, thread_pool* pool, job* jobs) {
    worker_ctx* workers = PUSH_ARRAY(arena, worker_ctx, pool->size);

    for (u32 i = 0; i < pool->size; ++i) {
        workers[i].arena = pool->arenas[i];
        workers[i].job = &jobs[i];

        int status = pthread_create(&pool->threads[i], NULL, worker_entry, &workers[i]);
        if (status != 0) {
            ERROR_EMIT(STR8_LIT("error"));
            exit(-1);
        }
    }

    for (u32 i = 0; i < pool->size; ++i) {
        pthread_join(pool->threads[i], NULL);
    }

    for (u32 i = 0; i < pool->size; ++i) {
            fib_job_data* d = (fib_job_data*)workers[i].job->data;

            printf("fib(%u) = %llu\n",
                   d->n,
                   (unsigned long long)d->result);
        }
}


void thread_pool_destroy(thread_pool* pool) {
    for (u32 i = 0; i < pool->size; ++i) {
        arena_destroy(pool->arenas[i]);
    }
}


void fib_job(void* data, mem_arena* arena) {
    fib_job_data* d = data;

    u32 n = d->n;

    if (n == 0) {
        d->result = 0;
        return;
    }
    if (n == 1) {
        d->result = 1;
        return;
    }

    u64 a = 0;
    u64 b = 1;

    for (u32 i = 2; i <= n; ++i) {
        u64 next = a + b;
        a = b;
        b = next;
    }

    d->result = b;
}

void foo(void* data, mem_arena* arena) {
    char* msg = data;
    printf("job says: %s\n", msg);
}

int main(void) {
    mem_arena* log_arena = arena_create(MByte(1));
    mem_arena* arena = arena_create(MByte(1));

    log_frame_begin(log_arena);

    thread_pool* pool = thread_pool_create(arena, 5, MByte(100));

    fib_job_data* data = PUSH_ARRAY(arena, fib_job_data, pool->size);
    job* jobs = PUSH_ARRAY(arena, job, pool->size);

    for (u32 i = 0; i < pool->size; ++i) {
        data[i].n = 20 + i;
        data[i].result = 0;

        jobs[i].data = &data[i];
        jobs[i].fn = fib_job;
    }

    thread_pool_run(arena, pool, jobs);

    sleep(5);

    WARNING_EMIT("main tread", OPEN_FILE_ERROR);

    log_print(log_arena, LOG_LEVEL_WARNING);

    thread_pool_destroy(pool);
    arena_destroy(log_arena);
    arena_destroy(arena);

    return 0;
}
