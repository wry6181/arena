#pragma once

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "base.h"

#define NET_READ_BUF_SIZE KByte(64)

typedef struct {
  int fd;
} net_socket;

typedef struct {
  net_socket sock;
  s8 request;
  mem_arena *arena;
} net_conn;

// ---- socket ----

static inline net_socket net_listen(u16 port) {
  net_socket s = {0};
  s.fd = socket(AF_INET, SOCK_STREAM, 0);
  if (s.fd < 0)
    return s;

  int opt = 1;
  setsockopt(s.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(s.fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(s.fd);
    s.fd = -1;
    return s;
  }

  listen(s.fd, 8192);
  return s;
}

static inline net_socket net_accept(net_socket server) {
  net_socket client = {0};
  struct sockaddr_in addr = {0};
  socklen_t len = sizeof(addr);
  client.fd = accept(server.fd, (struct sockaddr *)&addr, &len);

  if (client.fd >= 0) {
    int nodelay = 1;
    setsockopt(client.fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
  }

  return client;
}

static inline void net_close(net_socket s) { close(s.fd); }

static inline b32 net_socket_valid(net_socket s) { return s.fd >= 0; }

static inline s8 net_read(net_socket s, mem_arena *arena) {
  u64 cap = NET_READ_BUF_SIZE;
  u8 *buf = PUSH_ARRAY(arena, u8, cap);
  u64 len = 0;

  while (len < cap) {
    i64 n = recv(s.fd, buf + len, cap - len, 0);
    if (n <= 0)
      break;
    len += (u64)n;

    for (u64 i = 0; i + 3 < len; ++i) {
      if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' &&
          buf[i + 3] == '\n') {
        return (s8){.data = buf, .size = len};
      }
    }
  }

  if (len == 0)
    return (s8){0};
  return (s8){.data = buf, .size = len};
}

static inline void net_write(net_socket s, s8 buf) {
  u64 sent = 0;
  while (sent < buf.size) {
    i64 n = send(s.fd, buf.data + sent, buf.size - sent, 0);
    if (n <= 0)
      break;
    sent += (u64)n;
  }
}

typedef struct {
  s8 method;
  s8 path;
  s8 body;
  s8 headers;
  b32 valid;
} http_request;

typedef struct {
  u16 status;
  s8 content_type;
  s8 body;
} http_response;

static inline http_request http_parse(s8 raw, mem_arena *arena) {
  (void)arena;
  http_request req = {0};
  if (!raw.data || raw.size == 0)
    return req;

  s8 rest = raw;
  req.method = str8_slice_at(&rest, ' ');
  req.path = str8_slice_at(&rest, ' ');

  for (u64 i = 0; i + 3 < rest.size; ++i) {
    if (rest.data[i] == '\r' && rest.data[i + 1] == '\n' &&
        rest.data[i + 2] == '\r' && rest.data[i + 3] == '\n') {
      req.headers.data = rest.data;
      req.headers.size = i;
      req.body.data = rest.data + i + 4;
      req.body.size = rest.size - i - 4;
      break;
    }
  }

  req.valid = true;
  return req;
}

static inline s8 http_header_get(http_request req, s8 name) {
  s8 headers = req.headers;
  while (headers.size > 0) {
    s8 line = str8_slice_at(&headers, '\n');
    if (line.size == 0 ||
        (line.size == headers.size && line.data[line.size - 1] != '\n')) {
      break;
    }
    if (line.size > 0 && line.data[line.size - 1] == '\r') {
      str8_cut_right(&line, 1);
    }
    for (u64 i = 0; i < line.size; ++i) {
      if (line.data[i] == ':') {
        s8 key = {.data = line.data, .size = i};
        s8 value = {.data = line.data + i + 1, .size = line.size - i - 1};
        while (value.size > 0 && value.data[0] == ' ') {
          str8_cut_left(&value, 1);
        }
        if (str8_eq(key, name)) {
          return value;
        }
        break;
      }
    }
  }
  return (s8){0};
}

static inline s8 http_build_response(http_response res, b32 keep_alive,
                                     mem_arena *arena) {
  char status_line[64];
  const char *reason = "OK";
  if (res.status == 404)
    reason = "Not Found";
  else if (res.status == 500)
    reason = "Internal Server Error";

  int hlen = snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n",
                      res.status, reason);

  const char *ct = "text/plain";
  const char *conn =
      keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n";

  char meta[256];
  int mlen = snprintf(meta, sizeof(meta),
                      "Content-Type: %s\r\nContent-Length: %llu\r\n%s\r\n", ct,
                      (unsigned long long)res.body.size, conn);

  u64 total = (u64)hlen + (u64)mlen + res.body.size;
  u8 *buf = PUSH_ARRAY(arena, u8, total);

  u64 off = 0;
  memcpy(buf + off, status_line, (u64)hlen);
  off += (u64)hlen;
  memcpy(buf + off, meta, (u64)mlen);
  off += (u64)mlen;
  memcpy(buf + off, res.body.data, res.body.size);

  return (s8){.data = buf, .size = total};
}
