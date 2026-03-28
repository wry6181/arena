#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef float f32;
typedef i32 b32;
typedef i8 b8;
typedef atomic_uint au32;

#define Byte(n) (u64)(n)
#define KByte(n) ((u64)(n) << 10)
#define MByte(n) ((u64)(n) << 20)
#define GByte(n) ((u64)(n) << 30)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define UNUSED(x) (void(x))
#ifdef DNDEBUG
    #define ASSERT(cond) (void(cond))
#else
    #define ASSERT(cond) \
    do {    \
        if(!cond) {\
            fprintf(stderr, "ASSERT FAILED: %s\n  %s:%d\n", #cond, __FILE__, __LINE__);\
            __builtin_trap();\
        }\
    } while(0 )
#endif

#define UNREACHABLE() ASSERT(!"Unreachable code!");
#define NOTINPLAMENTED() ASSERT(!"Not inplamented yet!")
