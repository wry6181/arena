#pragma once

#include "base.h"
#include "networking.h"
#include "thread_pool.h"
#include <stdio.h>

static void handle_get_index(http_request *req, http_response *res,
                             mem_arena *arena) {
  (void)req;
  (void)arena;
  res->status = 200;
  res->content_type = STR8_LIT("text/plain");
  res->body = STR8_LIT("hello from arena server");
}

static void handle_get_ping(http_request *req, http_response *res,
                            mem_arena *arena) {
  (void)req;
  (void)arena;
  res->status = 200;
  res->content_type = STR8_LIT("application/json");
  res->body = STR8_LIT("{\"ok\":true}");
}

static void handle_post_echo(http_request *req, http_response *res,
                             mem_arena *arena) {
  (void)arena;
  printf("body: %.*s\n", STR8_FMT(req->body));
  res->status = 200;
  res->content_type = STR8_LIT("text/plain");
  res->body = req->body;
}

static const http_route routes[] = {
    {STR8_LIT("GET"), STR8_LIT("/"), handle_get_index},
    {STR8_LIT("GET"), STR8_LIT("/ping"), handle_get_ping},
    {STR8_LIT("POST"), STR8_LIT("/echo"), handle_post_echo},
};

static const u32 route_count = sizeof(routes) / sizeof(routes[0]);

static void dispatch(http_request *req, http_response *res, mem_arena *arena) {
  if (!req->valid) {
    res->status = 400;
    res->body = STR8_LIT("bad request");
    return;
  }
  b32 path_matched = false;
  for (u32 i = 0; i < route_count; i++) {
    if (!str8_eq(req->path, routes[i].path))
      continue;
    path_matched = true;
    if (!str8_eq(req->method, routes[i].method))
      continue;
    routes[i].handler(req, res, arena);
    return;
  }
  res->status = path_matched ? 405 : 404;
  res->body =
      path_matched ? STR8_LIT("method not allowed") : STR8_LIT("not found");

  printf("%.*s %.*s %u\n", STR8_FMT(req->path), STR8_FMT(req->method),
         res->status);
}

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
    res.content_type = STR8_LIT("text/plain");

    b32 keep_alive = false;
    if (req.valid) {
      s8 conn = http_header_get(req, STR8_LIT("Connection"));
      if (conn.size > 0) {
        if (str8_eq(conn, STR8_LIT("keep-alive")))
          keep_alive = true;
        else if (str8_eq(conn, STR8_LIT("close")))
          keep_alive = false;
      } else if (req.method.size >= 4 && req.method.data[0] == 'H') {
        keep_alive = true;
      }
    }

    dispatch(&req, &res, arena);
    printf("%.*s", STR8_FMT(req.body));

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
