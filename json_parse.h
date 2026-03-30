#pragma once

#include "base.h"
#include "types.h"
#include "error.h"
#include "log.h"

typedef enum {
    TYPE_OBJECT = 0,
    TYPE_ARRAY,
    TYPE_STRING,
    TYPE_NUMBER,
    TYPE_BOOL,
    TYPE_NULL,
} c_json_type;

typedef struct c_json c_json;
struct c_json {
    struct c_json *next;
    struct c_json *child;
    c_json_type    type;
    char          *valuestring;
    char          *string;
    double         valuenumber;
    b32            valuebool;
};

static void skip_whitespace(char **cursor) {
    while (**cursor == ' '  || **cursor == '\n' ||
           **cursor == '\r' || **cursor == '\t') {
        (*cursor)++;
    }
}

static char *parse_string(mem_arena *arena, char **cursor) {
    (*cursor)++;
    char *start = *cursor;
    while (**cursor != '\0') {
        if (**cursor == '\\') { (*cursor) += 2; continue; }
        if (**cursor == '"')  { break; }
        (*cursor)++;
    }
    u64   len    = *cursor - start;
    char *result = PUSH_ARRAY_ZERO(arena, char, len + 1);
    memcpy(result, start, len);
    (*cursor)++;
    return result;
}

static double parse_number(char **cursor) {
    char *start = *cursor;
    if (**cursor == '-') (*cursor)++;
    while (**cursor >= '0' && **cursor <= '9') (*cursor)++;
    if (**cursor == '.') {
        (*cursor)++;
        while (**cursor >= '0' && **cursor <= '9') (*cursor)++;
    }
    if (**cursor == 'e' || **cursor == 'E') {
        (*cursor)++;
        if (**cursor == '+' || **cursor == '-') (*cursor)++;
        while (**cursor >= '0' && **cursor <= '9') (*cursor)++;
    }
    return strtod(start, NULL);
}

static c_json *parse_value(mem_arena *arena, char **cursor);

static c_json *parse_object(mem_arena *arena, char **cursor) {
    (*cursor)++;
    c_json *first = NULL;
    c_json *last  = NULL;

    while (**cursor != '}' && **cursor != '\0') {
        skip_whitespace(cursor);
        if (**cursor != '"') break;

        char *key = parse_string(arena, cursor);
        skip_whitespace(cursor);
        if (**cursor == ':') (*cursor)++;
        skip_whitespace(cursor);

        c_json *child  = parse_value(arena, cursor);
        child->string  = key;

        if (!first) { first = last = child; }
        else        { last->next = child; last = child; }

        skip_whitespace(cursor);
        if (**cursor == ',') (*cursor)++;
        skip_whitespace(cursor);
    }
    if (**cursor == '}') (*cursor)++;

    c_json *node = PUSH_STRUCT(arena, c_json);
    node->type   = TYPE_OBJECT;
    node->child  = first;
    return node;
}

static c_json *parse_array(mem_arena *arena, char **cursor) {
    (*cursor)++;
    c_json *first = NULL;
    c_json *last  = NULL;

    while (**cursor != ']' && **cursor != '\0') {
        skip_whitespace(cursor);
        c_json *child = parse_value(arena, cursor);

        if (!first) { first = last = child; }
        else        { last->next = child; last = child; }

        skip_whitespace(cursor);
        if (**cursor == ',') (*cursor)++;
        skip_whitespace(cursor);
    }
    if (**cursor == ']') (*cursor)++;

    c_json *node = PUSH_STRUCT(arena, c_json);
    node->type   = TYPE_ARRAY;
    node->child  = first;
    return node;
}

static c_json *parse_value(mem_arena *arena, char **cursor) {
    skip_whitespace(cursor);
    char c = **cursor;

    if      (c == '{') { return parse_object(arena, cursor); }
    else if (c == '[') { return parse_array(arena, cursor);  }

    c_json *node = PUSH_STRUCT(arena, c_json);

    if (c == '"') {
        node->type        = TYPE_STRING;
        node->valuestring = parse_string(arena, cursor);
    }
    else if (c == '-' || (c >= '0' && c <= '9')) {
        node->type        = TYPE_NUMBER;
        node->valuenumber = parse_number(cursor);
    }
    else if (strncmp(*cursor, "true", 4) == 0) {
        node->type      = TYPE_BOOL;
        node->valuebool = 1;
        *cursor += 4;
    }
    else if (strncmp(*cursor, "false", 5) == 0) {
        node->type      = TYPE_BOOL;
        node->valuebool = 0;
        *cursor += 5;
    }
    else if (strncmp(*cursor, "null", 4) == 0) {
        node->type = TYPE_NULL;
        *cursor += 4;
    }
    else {
        ERROR_EMIT("Json parse unexpected character", PARSE_ERROR);
    }

    return node;
}

static c_json *json_parse_from_s8(mem_arena *arena, s8 json_str) {
    char *cursor = (char *)json_str.data;
    return parse_value(arena, &cursor);
}

static c_json *json_parse_file(mem_arena *arena, const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        ERROR_EMIT("Json parse open file error", OPEN_FILE_ERROR);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    u8 *buffer = PUSH_ARRAY_ZERO(arena, u8, size + 1);
    fread(buffer, 1, size, file);
    fclose(file);

    return json_parse_from_s8(arena, (s8){ .data = buffer, .size = (u64)size });
}

static c_json *json_get(c_json *obj, const char *key) {
    if (!obj || obj->type != TYPE_OBJECT) return NULL;
    for (c_json *child = obj->child; child; child = child->next) {
        if (child->string && strcmp(child->string, key) == 0) return child;
    }
    return NULL;
}

static c_json *json_get_index(c_json *arr, u32 index) {
    if (!arr || arr->type != TYPE_ARRAY) return NULL;
    c_json *child = arr->child;
    for (u32 i = 0; i < index && child; i++) child = child->next;
    return child;
}

static inline b32 json_is_null  (c_json *n) { return n && n->type == TYPE_NULL;   }
static inline b32 json_is_bool  (c_json *n) { return n && n->type == TYPE_BOOL;   }
static inline b32 json_is_number(c_json *n) { return n && n->type == TYPE_NUMBER; }
static inline b32 json_is_string(c_json *n) { return n && n->type == TYPE_STRING; }
static inline b32 json_is_object(c_json *n) { return n && n->type == TYPE_OBJECT; }
static inline b32 json_is_array (c_json *n) { return n && n->type == TYPE_ARRAY;  }


static inline double json_get_number(c_json *n) { ASSERT(json_is_number(n)); return n->valuenumber; }
static inline char  *json_get_string(c_json *n) { ASSERT(json_is_string(n)); return n->valuestring; }
static inline b32    json_get_bool  (c_json *n) { ASSERT(json_is_bool(n));   return n->valuebool;   }

static inline double json_get_number_default(c_json *n, double def)    { return json_is_number(n) ? n->valuenumber : def; }
static inline char  *json_get_string_default(c_json *n, char *def)     { return json_is_string(n) ? n->valuestring : def; }
static inline b32    json_get_bool_default  (c_json *n, b32 def)       { return json_is_bool(n)   ? n->valuebool   : def; }
