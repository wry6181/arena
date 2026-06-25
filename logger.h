#include "base.h"
#include "types.h"

typedef enum LogLevel {
    INFO = 0,
    DEBUG,
    WARNING,
    ERROR,
    FATAL
} LogLevel;

typedef struct log log;
struct log {
    LogLevel level;
    s8 log_msg;
    s8 source_file;
};

typedef struct log_frame log_frame;
struct log_frame {
    log* logs;
    s8 source_file;
    s8 name;
    u64 size;
};

typedef struct log_scope log_scope;
struct log_scope {
    mem_arena* log_arena;
    log_frame* frames;
};


log_scope log_scope_open(s8 name, s8 file, u64 capacity);

void log_scope_close(s8 name);

void log_scope_print(log_scope* scope);
