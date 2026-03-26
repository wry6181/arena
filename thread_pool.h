#pragma once
#include "base.h"
#include "error.h"
#include "log.h"
#include "networking.h"
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

typedef struct job_queue job_queue;

typedef void (*job_fn)(void *data, mem_arena *arena, mem_arena *main_arena);

typedef struct {
  job_fn fn;
  void *data;
  net_socket sock;
} job;

struct job_queue {
  job entries[10000];
  u32 volatile next_write;
  u32 volatile next_read;
  u32 volatile completion_goal;
  u32 volatile completion_count;
  pthread_mutex_t lock;
  pthread_cond_t cond;
};

typedef struct {
  pthread_t *threads;
  mem_arena **arenas;
  job_queue *queue;
  u32 size;
} thread_pool;

typedef struct {
  mem_arena *arena;
  mem_arena *main_arena;
  job_queue *queue;
} worker_ctx;

void *worker_entry(void *arg) {
  worker_ctx *ctx = (worker_ctx *)arg;
  job_queue *q = ctx->queue;
  while (1) {
    pthread_mutex_lock(&q->lock);
    while (q->next_read >= q->next_write) {
      pthread_cond_wait(&q->cond, &q->lock);
    }
    u32 read = q->next_read++;
    pthread_mutex_unlock(&q->lock);
    job *j = &q->entries[read % ARRAY_COUNT(q->entries)];
    j->fn(j->data, ctx->arena, ctx->main_arena);
    __sync_fetch_and_add(&q->completion_count, 1);
  }
  return NULL;
}

static inline void job_queue_init(job_queue *q) {
  q->next_write = 0;
  q->next_read = 0;
  q->completion_goal = 0;
  q->completion_count = 0;
  pthread_mutex_init(&q->lock, NULL);
  pthread_cond_init(&q->cond, NULL);
}

static inline void job_queue_push(job_queue *q, job_fn fn, void *data) {
  pthread_mutex_lock(&q->lock);
  u32 idx = q->next_write % ARRAY_COUNT(q->entries);
  q->entries[idx].fn = fn;
  q->entries[idx].data = data;
  q->next_write++;
  __sync_fetch_and_add(&q->completion_goal, 1);
  pthread_cond_broadcast(&q->cond);
  pthread_mutex_unlock(&q->lock);
}

static inline void job_queue_drain(job_queue *q, mem_arena *arena,
                                   mem_arena *main_arena) {
  while (q->completion_count < q->completion_goal) {
    u32 read = q->next_read;
    u32 write = q->next_write;
    if (read != write) {
      u32 idx = __sync_val_compare_and_swap(&q->next_read, read, read + 1);
      if (idx == read) {
        job *j = &q->entries[idx % ARRAY_COUNT(q->entries)];
        j->fn(j->data, arena, main_arena);
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
    worker_ctx *w = PUSH_STRUCT_ZERO(arena, worker_ctx);
    w->arena = pool->arenas[i];
    w->main_arena = arena;
    w->queue = pool->queue;
    pthread_create(&pool->threads[i], NULL, worker_entry, w);
  }
  return pool;
}

void thread_pool_destroy(thread_pool *pool) {
  pthread_mutex_destroy(&pool->queue->lock);
  pthread_cond_destroy(&pool->queue->cond);
  for (u32 i = 0; i < pool->size; ++i) {
    arena_destroy(pool->arenas[i]);
  }
}
