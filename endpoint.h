#pragma once

#include "base.h"
#include "json_parse.h"
#include "networking.h"
#include "thread_pool.h"
#include <stdio.h>

typedef enum {
  GET = 0,
  POST,
} endpoint_type;

typedef struct endpoint endpoint;
struct endpoint {
  s8 path;
  endpoint_type type;
  void (*handler)(http_request *req, http_response *res, mem_arena *arena);
  struct endpoint *next;
};

static endpoint *endpoint_list = NULL;

static endpoint *register_endpoint(
    mem_arena *arena, endpoint_type type, s8 path,
    void (*handler)(http_request *req, http_response *res, mem_arena *arena)) {
  endpoint *e = PUSH_STRUCT_ZERO(arena, endpoint);
  e->path = path;
  e->type = type;
  e->handler = handler;
  e->next = endpoint_list;
  endpoint_list = e;
  return e;
}

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

static void handle_get_echo(http_request *req, http_response *res,
                            mem_arena *arena) {
  (void)arena;
  res->status = 200;
  res->content_type = STR8_LIT("application/json");
  res->body = req->body;
}

static void handle_post_echo(http_request *req, http_response *res,
                             mem_arena *arena) {
  c_json *parsed = json_parse_from_s8(arena, req->body);
  if (parsed) {
    if (json_is_object(parsed)) {
      c_json *msg = json_get(parsed, "message");
      if (msg && json_is_string(msg)) {
        fprintf(stderr, "parsed message: %s\n", json_get_string(msg));
      }
      c_json *count = json_get(parsed, "count");
      if (count && json_is_number(count)) {
        fprintf(stderr, "parsed count: %.0f\n", json_get_number(count));
      }
      c_json *active = json_get(parsed, "active");
      if (active && json_is_bool(active)) {
        fprintf(stderr, "parsed active: %s\n",
                json_get_bool(active) ? "true" : "false");
      }
      c_json *ratio = json_get(parsed, "ratio");
      if (ratio && json_is_number(ratio)) {
        fprintf(stderr, "parsed ratio: %f\n", json_get_number(ratio));
      }
      c_json *val = json_get(parsed, "value");
      if (val && json_is_null(val)) {
        fprintf(stderr, "parsed value: null\n");
      }
      c_json *user = json_get(parsed, "user");
      if (user && json_is_object(user)) {
        fprintf(stderr, "parsed user: (object)\n");
      }
    } else if (json_is_array(parsed)) {
      c_json *first = json_get_index(parsed, 0);
      if (first && json_is_string(first)) {
        fprintf(stderr, "parsed array[0]: %s\n", json_get_string(first));
      }
      c_json *second = json_get_index(parsed, 1);
      if (second && json_is_string(second)) {
        fprintf(stderr, "parsed array[1]: %s\n", json_get_string(second));
      }
    }
  }
  res->status = 200;
  res->content_type = STR8_LIT("application/json");
  res->body = req->body;
}

static c_json *config_cache = NULL;

static void handle_get_config(http_request *req, http_response *res,
                              mem_arena *arena) {
  (void)req;
  if (!config_cache) {
    config_cache = json_parse_file(arena, "config.json");
  }
  if (config_cache) {
    res->status = 200;
    res->content_type = STR8_LIT("application/json");
    res->body = STR8_LIT("config loaded");
  } else {
    res->status = 500;
    res->body = STR8_LIT("config not found");
  }
}

static void dispatch(http_request *req, http_response *res, mem_arena *arena) {
  if (!req->valid) {
    res->status = 400;
    res->body = STR8_LIT("bad request");
    return;
  }

  endpoint_type req_type = GET;
  if (str8_eq(req->method, STR8_LIT("POST")))
    req_type = POST;

  for (endpoint *e = endpoint_list; e; e = e->next) {
    if (!str8_eq(req->path, e->path))
      continue;
    if (e->type != req_type) {
      res->status = 405;
      res->body = STR8_LIT("method not allowed");
      return;
    }
    e->handler(req, res, arena);
    return;
  }

  res->status = 404;
  res->body = STR8_LIT("not found");
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
