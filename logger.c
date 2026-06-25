#include "logger.h"
#include "base.h"
#include <stdio.h>
#define LOG_SCOPE(name, capacity) log_scope_open(STR8_LIT(name), STR8_LIT(__FILE__), (capacity))

log_scope log_scope_open(s8 name, s8 file, u64 capacity) {
    log_scope scope = {0};
    mem_arena* scope_arena = arena_create(capacity);
    log_frame* frame = PUSH_STRUCT(scope_arena, log_frame);
    //log_scope* scope = PUSH_STRUCT(scope_arena, log_scope);
    scope.log_arena = scope_arena;
    scope.frames = frame;
    scope.frames->source_file = file;
    scope.frames->name = STR8_LIT("init frame");
    scope.frames->size = 0;
    return scope;
}

static s8 log_level_conv(LogLevel level) {
    switch (level) {
        case INFO: return STR8_LIT("INFO");
        case DEBUG: return STR8_LIT("DEBUG");
        case WARNING: return STR8_LIT("WARNING");
        case ERROR: return STR8_LIT("ERROR");
        case FATAL: return STR8_LIT("FATAL");
    }
}

void log_scope_close(s8 name) {

}

void log_scope_print(log_scope* scope) {
    log_frame* frames = scope->frames;
    printf("frame name: '%.*s' source: '%.*s'\n",STR8_FMT(frames->name), STR8_FMT(frames->source_file));
    if(frames->size == 0) {
        return;
    }
    for(int i = 0; i < frames->size; ++i) {
        printf("[%.*s] %.*s %.*s\n", STR8_FMT(log_level_conv(frames->logs[i].level)), STR8_FMT(frames->logs[i].source_file), STR8_FMT(frames->logs[i].log_msg));
    }

}

int main (void) {
    log_scope ls_main = LOG_SCOPE("main", MByte(100));

    log_scope_print(&ls_main);
    return 0;
}
