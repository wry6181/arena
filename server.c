#include "base.h"
#include "error.h"
#include "log.h"
#include "networking.h"
#include "thread_pool.h"

void conn_job(void *data, mem_arena *arena, mem_arena *main_arena) {
  job *j = data;
  net_socket sock = j->sock;
  u64 arena_pos = main_arena->pos;

  while (1) {
    s8 raw = net_read(sock, arena);

    if (!raw.data || raw.size == 0) {
      net_close(sock);
      arena_pop_to(main_arena, arena_pos);
      break;
    }

    http_request req = http_parse(raw, arena);
    http_response res = {0};

    b32 keep_alive = false;
    if (req.valid) {
      s8 conn = http_header_get(req, STR8_LIT("Connection"));
      if (conn.size > 0) {
        if (str8_eq(conn, STR8_LIT("keep-alive"))) {
          keep_alive = true;
        } else if (str8_eq(conn, STR8_LIT("close"))) {
          keep_alive = false;
        }
      } else if (req.method.size >= 4 && req.method.data[0] == 'H') {
        keep_alive = true;
      }
    }

    if (!req.valid) {
      res.status = 500;
      res.body = STR8_LIT("bad request");
    } else if (req.path.size == 1 && req.path.data[0] == '/') {
      res.status = 200;
      res.body = STR8_LIT("hello from arena server");
    } else {
      res.status = 404;
      res.body = STR8_LIT("not found");
    }

    s8 out = http_build_response(res, keep_alive, arena);
    net_write(sock, out);

    arena_clear(arena);

    if (!keep_alive) {
      net_close(sock);
      arena_pop_to(main_arena, arena_pos);
      break;
    }
  }
}

int main(void) {
  mem_arena *log_arena = arena_create(MByte(1));
  mem_arena *arena = arena_create(MByte(100));

  log_frame_begin(log_arena);

  thread_pool *pool = thread_pool_create(arena, 8, MByte(100));

  net_socket server = net_listen(8080);
  if (!net_socket_valid(server)) {
    fprintf(stderr, "failed to bind\n");
    return 1;
  }

  printf("listening on :8080\n");
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
