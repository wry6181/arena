#include "base.h"
#include "error.h"
#include "log.h"
#include "networking.h"
#include "thread_pool.h"
#include "endpoint.h"


int main(void) {
  mem_arena *log_arena = arena_create(MByte(1));
  mem_arena *arena = arena_create(MByte(100));

  log_frame_begin(log_arena);

  thread_pool *pool = thread_pool_create(arena, 8, MByte(100));

  net_socket server = net_listen(8888);
  if (!net_socket_valid(server)) {
    fprintf(stderr, "failed to bind\n");
    return 1;
  }

  printf("listening on :8888\n");
  fflush(stdout);

  while (1) {
    net_socket client = net_accept(server);
    if (!net_socket_valid(client))
      continue;

    job *j = PUSH_STRUCT_ZERO(arena, job);
    j->fn = conn_job;
    j->sock = client;
    job_queue_push(pool->queue, j->fn, j);
  }

  WARNING_EMIT("main tread", OPEN_FILE_ERROR);

  log_print(log_arena, LOG_LEVEL_WARNING);

  thread_pool_destroy(pool);
  arena_destroy(log_arena);
  arena_destroy(arena);

  return 0;
}
