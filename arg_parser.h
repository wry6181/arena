#include "base.h"
#include "types.h"

#define REQUIRED_ARG(type, name, ...) type name;
#define OPTIONAL_ARG(type, name, ...) type name;
#define BOOL_ARG(name, ...) b32 name;

typedef struct cl_args cl_args;
struct cl_args {
#ifdef REQUIRED_ARGS
  REQUIRED_ARGS
#endif
#ifdef OPTIONAL_ARGS
  OPTIONAL_ARGS
#endif
#ifdef BOOL_ARGS
  BOOL_ARGS
#endif
};

#undef REQUIRED_ARG
#undef OPTIONAL_ARG
#undef BOOL_ARG

static inline cl_args make_default_args(void) {
  cl_args args = {
#define REQUIRED_ARG(type, name, ...) .name = (type)0,
#define OPTIONAL_ARG(type, name, default, ...) .name = default,
#define BOOL_ARG(name, ...) .name = 0,

#ifdef REQUIRED_ARGS
      REQUIRED_ARGS
#endif

#ifdef OPTIONAL_ARGS
          OPTIONAL_ARGS
#endif

#ifdef BOOL_ARGS
              BOOL_ARGS
#endif

#undef REQUIRED_ARG
#undef OPTIONAL_ARG
#undef BOOL_ARG
  };

  return args;
}

// Parse arguments. Returns 0 if failed.
static inline int parse_args(int argc, const char *argv[], cl_args *args) {
  if (!argc || !argv) {
    fprintf(stderr, "Internal error: null args or argv.\n");
    return 0;
  }

  int ok;
  int i = 1;

// Get required arguments
#ifdef REQUIRED_ARGS
#define REQUIRED_ARG(type, name, label, description, parser)                   \
  ok = 0;                                                                      \
  args->name = (type)parser(argv[i++], &ok);                                   \
  if (!ok)                                                                     \
    return 0;

  REQUIRED_ARGS
#undef REQUIRED_ARG
#endif

// Get optional and boolean arguments
#define OPTIONAL_ARG(type, name, default, flag, label, description, formatter, \
                     parser)                                                   \
  if (!strcmp(argv[i], flag)) {                                                \
    if (i + 1 >= argc) {                                                       \
      fprintf(stderr, "Error: option '%s' requires a value.\n", flag);         \
      return 0;                                                                \
    }                                                                          \
    ok = 0;                                                                    \
    args->name = (type)parser(argv[++i], &ok);                                 \
    if (!ok)                                                                   \
      return 0;                                                                \
    continue;                                                                  \
  }

#define BOOLEAN_ARG(name, flag, description)                                   \
  if (!strcmp(argv[i], flag)) {                                                \
    args->name = 1;                                                            \
    continue;                                                                  \
  }

#undef OPTIONAL_ARG
#undef BOOLEAN_ARG

  return 1;
}
