#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "base.h"
#include "error.h"
#include "log.h"

typedef void (*job_fn)(void *data, mem_arena *arena);

typedef struct {
  job_fn fn;
  void *data;
} job;

typedef struct {
  job entries[256];
  u32 volatile next_write;
  u32 volatile next_read;
  u32 volatile completion_goal;
  u32 volatile completion_count;
  sem_t semaphore;
} job_queue;

typedef struct {
  pthread_t *threads;
  mem_arena **arenas;
  job_queue *queue;
  u32 size;
} thread_pool;

typedef struct {
  mem_arena *arena;
  job_queue *queue;
} worker_ctx;

void *worker_entry(void *arg) {
  worker_ctx *ctx = arg;
  job_queue *q = ctx->queue;

  while (1) {
    u32 read = q->next_read;
    u32 write = q->next_write;

    if (read != write) {
      u32 idx = __sync_val_compare_and_swap(&q->next_read, read, read + 1);
      if (idx == read) {
        job *j = &q->entries[idx % ARRAY_COUNT(q->entries)];
        j->fn(j->data, ctx->arena);
        __sync_fetch_and_add(&q->completion_count, 1);
      }
    } else {
      dispatch_semaphore_wait(q->semaphore, DISPATCH_TIME_FOREVER);
    }
  }
  return NULL;
}

static inline void job_queue_init(job_queue *q) {
  q->next_write = 0;
  q->next_read = 0;
  q->completion_goal = 0;
  q->completion_count = 0;
  q->semaphore = dispatch_semaphore_create(0);
}

static inline void job_queue_push(job_queue *q, job_fn fn, void *data) {
  u32 idx = q->next_write++ % ARRAY_COUNT(q->entries);
  q->entries[idx].fn = fn;
  q->entries[idx].data = data;
  q->completion_goal++;
  __sync_synchronize();
  dispatch_semaphore_signal(q->semaphore);
}

static inline void job_queue_drain(job_queue *q, mem_arena *arena) {
  while (q->completion_count < q->completion_goal) {
    u32 read = q->next_read;
    u32 write = q->next_write;
    if (read != write) {
      u32 idx = __sync_val_compare_and_swap(&q->next_read, read, read + 1);
      if (idx == read) {
        job *j = &q->entries[idx % ARRAY_COUNT(q->entries)];
        j->fn(j->data, arena);
        __sync_fetch_and_add(&q->completion_count, 1);
      }
    } else {
      sched_yield();
    }
  }
}

thread_pool *thread_pool_create(mem_arena *arena, u32 pool_size,
                                u64 local_arena_size) {
  thread_pool *pool = PUSH_STRUCT_ZERO(arena, thread_pool);
  pool->threads = PUSH_ARRAY_ZERO(arena, pthread_t, pool_size);
  pool->arenas = PUSH_ARRAY_ZERO(arena, mem_arena *, pool_size);
  pool->queue = PUSH_STRUCT_ZERO(arena, job_queue);
  pool->size = pool_size;

  job_queue_init(pool->queue);

  for (u32 i = 0; i < pool_size; ++i) {
    pool->arenas[i] = arena_create(local_arena_size);

    worker_ctx *w = PUSH_STRUCT_ZERO(pool->arenas[i], worker_ctx);
    w->arena = pool->arenas[i];
    w->queue = pool->queue;

    pthread_create(&pool->threads[i], NULL, worker_entry, w);
  }

  return pool;
}

void thread_pool_destroy(thread_pool *pool) {
  for (u32 i = 0; i < pool->size; ++i) {
    arena_destroy(pool->arenas[i]);
  }
}

typedef struct {
  u32 n;
  u64 result;
} fib_job_data;

void fib_job(void *data, mem_arena *arena) {
  fib_job_data *d = data;

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

int main(void) {
  mem_arena *log_arena = arena_create(MByte(1));
  mem_arena *arena = arena_create(MByte(1));

  log_frame_begin(log_arena);

  thread_pool *pool = thread_pool_create(arena, 5, MByte(100));

  fib_job_data *data = PUSH_ARRAY_ZERO(arena, fib_job_data, 100);

  for (u32 i = 0; i < 100; ++i) {
    data[i].n = 20 + i;
    job_queue_push(pool->queue, fib_job, &data[i]);
  }

  job_queue_drain(pool->queue, arena);

  for (u32 i = 0; i < 100; ++i) {
    printf("fib(%u) = %llu\n", data[i].n, (u64)data[i].result);
  }

  sleep(5);

  WARNING_EMIT("main tread", OPEN_FILE_ERROR);

  log_print(log_arena, LOG_LEVEL_WARNING);

  thread_pool_destroy(pool);
  arena_destroy(log_arena);
  arena_destroy(arena);

  return 0;
}
