#include "base.h"
#include "error.h"
#include "log.h"
#include "thread_pool.h"
#include "networking.h"

typedef struct {
  net_socket sock;
} conn_job_data;

void conn_job(void *data, mem_arena *arena) {
  conn_job_data *d = data;

  s8 raw = net_read(d->sock, arena);

  http_request req = http_parse(raw, arena);
  http_response res = {0};

  if (!req.valid) {
    res.status = 500;
    res.body   = STR8_LIT("bad request");
  } else if (req.path.size == 1 && req.path.data[0] == '/') {
    res.status = 200;
    res.body   = STR8_LIT("hello from arena server");
  } else {
    res.status = 404;
    res.body   = STR8_LIT("not found");
  }

  s8 out = http_build_response(res, arena);
  net_write(d->sock, out);
  net_close(d->sock);

  arena_clear(arena);
}

int main(void) {
  mem_arena *log_arena = arena_create(MByte(1));
  mem_arena *arena = arena_create(MByte(100));

  log_frame_begin(log_arena);

  thread_pool *pool = thread_pool_create(arena, 5, MByte(100));

  net_socket server = net_listen(8080);
    if (!net_socket_valid(server)) {
      fprintf(stderr, "failed to bind\n");
      return 1;
    }

    printf("listening on :8080\n");

    while (1) {
      net_socket client = net_accept(server);
      if (!net_socket_valid(client)) continue;

      conn_job_data *d = PUSH_STRUCT_ZERO(arena, conn_job_data);
      d->sock = client;
      job_queue_push(pool->queue, conn_job, d);
    }


  WARNING_EMIT("main tread", OPEN_FILE_ERROR);

  log_print(log_arena, LOG_LEVEL_WARNING);

  thread_pool_destroy(pool);
  arena_destroy(log_arena);
  arena_destroy(arena);

  return 0;
}
