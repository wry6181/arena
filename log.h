#pragma once

#include "base.h"
#include <stdio.h>
#include <stdint.h>

#ifndef REGISTER_ERROR_VALUE
  typedef int log_error;
  #define REGISTER_ERROR_VALUE(T)  // no-op
#endif

typedef enum {
    LOG_LEVEL_NONE    = 0,
    LOG_LEVEL_INFO    = (1 << 0),
    LOG_LEVEL_WARNING = (1 << 1),
    LOG_LEVEL_ERROR   = (1 << 2),
} log_level;

typedef struct{
    s8 msg;
    log_level level;
    log_error err;
} log_msg;

typedef struct{
    u64 arena_pos;
    u32 size;
    u32 capacity;
    log_msg *msgs;
} log_frame;

typedef struct{
    mem_arena* arena;
    u32 size;
    u32 capacity;
    log_frame* frames;
} log_context;

static inline void log_emit(mem_arena* arena, log_level level, s8 msg, log_error err);
static inline s8 log_frame_peek(mem_arena* arena, u32 level_mask);


static __thread log_context _log_context = { 0 };

#define ERROR_EMIT(msg, err) log_emit(_log_context.arena, LOG_LEVEL_ERROR, \
    str8_append(_log_context.arena, STR8_LIT("LOG MSG [E]: "), STR8_LIT(msg)), (err))

#define WARNING_EMIT(msg, err) log_emit(_log_context.arena, LOG_LEVEL_WARNING, \
    str8_append(_log_context.arena, STR8_LIT("LOG MSG [W]: "), STR8_LIT(msg)), (err))

static inline log_frame log_create_frame(mem_arena* arena) {
    log_frame frame = {
        .size = 0,
        .capacity = 4,
    };

    frame.msgs = PUSH_ARRAY(arena, log_msg, frame.capacity);

    return frame;
}

static inline void log_frame_begin(mem_arena* arena)
{
    _log_context.arena = arena;

    if (_log_context.frames == NULL)
    {
        _log_context.capacity = 4;
        _log_context.size = 0;
        _log_context.arena = arena;
        _log_context.frames = PUSH_ARRAY(arena, log_frame, _log_context.capacity);
    }

    if (_log_context.size >= _log_context.capacity)
    {
        u32 new_capacity = _log_context.capacity * 2;
        log_frame* new_frames = PUSH_ARRAY(arena, log_frame, new_capacity);

        memcpy(new_frames, _log_context.frames, sizeof(log_frame) * _log_context.size);

        _log_context.frames = new_frames;
        _log_context.capacity = new_capacity;
    }

    log_frame frame = { .capacity = 4, .size = 0 };

    frame.arena_pos = arena->pos;

    frame.msgs = PUSH_ARRAY(arena, log_msg, frame.capacity);

    _log_context.frames[_log_context.size++] = frame;
}

static inline s8 log_frame_end(mem_arena* arena, u32 level_mask) {
    if (_log_context.frames == NULL || _log_context.size == 0) {
        return (s8){ 0 };
    }

    s8 out = log_frame_peek(arena, level_mask);

    _log_context.size--;
    return out;
}

static inline s8 log_frame_peek(mem_arena* arena, u32 level_mask) {
    if (_log_context.frames == NULL) { return (s8){ 0 }; }

    log_frame* frame = &_log_context.frames[_log_context.size-1];

    u32 num_logs_in_mask = 0;
    u64 total_out_size = 0;

    for (u32 i = 0; i < frame->size; i++) {
        if ((frame->msgs[i].level & level_mask) == 0) {
            continue;
        }

        num_logs_in_mask++;
        total_out_size += frame->msgs[i].msg.size;
    }

    if (num_logs_in_mask == 0) {
        return (s8){ 0 };
    }

    total_out_size += num_logs_in_mask - 1;

    s8 out = {
        .size = total_out_size,
        .data = PUSH_ARRAY(arena, u8, total_out_size)
    };

    u64 out_pos = 0;

    for (u32 i = 0; i < frame->size; i++) {
        log_msg* msg = &frame->msgs[i];

        if ((msg->level & level_mask) == 0) {
            continue;
        }

        if (out_pos != 0) {
            out.data[out_pos++] = '\n';
        }

        memcpy(out.data + out_pos, msg->msg.data, msg->msg.size);
        out_pos += msg->msg.size;
    }

    return out;
}
static inline void log_emit(mem_arena* arena, log_level level, s8 msg, log_error err) {
    if (_log_context.frames == NULL) { return; }

    log_frame* frame = &_log_context.frames[_log_context.size-1];

    if (frame->size >= frame->capacity) {
        u32 old_capacity = frame->capacity;
        frame->capacity *= 2;
        log_msg* new_msgs = PUSH_ARRAY(arena, log_msg, frame->capacity);
        memcpy(new_msgs, frame->msgs, sizeof(log_msg) * old_capacity);
        frame->msgs = new_msgs;
    }

    frame->msgs[frame->size++] = (log_msg){
        .level = level,
        .msg = msg,
        .err = err,
    };
}

// s8 file_read(mem_arena* scrach_arena, const char *file_name) {
//     FILE *f = fopen(file_name, "rb");
//     if (!f) {
//         ERROR_EMIT("Failed to open file", OPEN_FILE_ERROR);
//         return (s8){0};
//     }

//     fseek(f, 0, SEEK_END);
//     size_t size = ftell(f);
//     rewind(f);

//     char* buffer = PUSH_ARRAY(scrach_arena, char, size+1);

//     if (!buffer) {
//         fclose(f);
//         return (s8){0};
//     }

//     fread(buffer, 1, size, f);
//     buffer[size] = 0;

//     fclose(f);

//     return (s8){(u8*)buffer, size};
// }

static inline void log_print(mem_arena* arena, log_level level) {
    s8 logs = log_frame_end(arena, level);
    if (logs.size) {
        printf("%.*s\n", STR8_FMT(logs));
    }
}

// int main(int argc, char** argv) {
//     if (argc <= 1) { return 0; }

//     mem_arena* log_arena = arena_create(MByte(1));
//     mem_arena* scratch_arena = arena_create(GByte(1));

//     _log_context.arena = log_arena;

//     log_frame_begin(log_arena);

//     string_8 file = file_read(scratch_arena, argv[1]);

//     u32 file_sum = 0;
//     for (u64 i = 0; i < file.size; i++) {
//         file_sum += file.str[i];
//     }

//     printf("File sum: %u\n", file_sum);
//
//     if(!file_sum) {
//         ERROR_EMIT(STR8_LIT("Not able to count file sum"));
//     }


//     string_8 logs = log_frame_end(log_arena, LOG_LEVEL_ERROR);
//     if (logs.size) {
//         printf("%.*s\n", STR8_FMT(logs));
//     }

//     arena_destroy(log_arena);
//     arena_destroy(scratch_arena);

//     return 0;
// }
