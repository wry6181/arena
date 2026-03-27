#include "base.h"
#include "endpoint.h"
#include "error.h"
#include "log.h"
#include "networking.h"
#include "thread_pool.h"

#include <stdio.h>
#include <stdlib.h>

static inline const char *parse_str(const char *s, int *ok) {
  *ok = 1;
  return s;
}
static inline u32 parse_u32(const char *s, int *ok) {
  *ok = 1;
  return (u32)strtoul(s, NULL, 10);
}

#define REQUIRED_ARGS                                                          \
  REQUIRED_ARG(const char *, input_file, "input", "Input file path", parse_str)

#define OPTIONAL_ARGS                                                          \
  OPTIONAL_ARG(u32, threads, 1, "-t", "threads", "Number of threads to use",   \
               "%u", parse_u32)

#define BOOL_ARGS BOOL_ARG(help, "-h", "Show help")

#include "arg_parser.h"

int main(int argc, const char *argv[]) {

  // for (u64 i = 0; i < argc; ++i) {
  //     printf("%lu %s\n", i, args[i]);
  // }
  //
  cl_args args = make_default_args();

  if (!parse_args(argc, argv, &args)) {
    // print_help(argv[0]);
    return 1;
  }

  printf("%s\n%u\n%d\n", args.input_file, args.threads, args.help);

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
