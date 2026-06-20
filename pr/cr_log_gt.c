#include "../cr_types.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/*
 * Configs
 */

#define CR_LOG_ITEM_SIZE   (1 << 9)
#define CR_LOG_ITEM_FIELDS 16

#if (CR_LOG_ITEM_SIZE & (CR_LOG_ITEM_SIZE - 1)) != 0
#error "CR_LOG_ITEM_SIZE is not a power of 2"
#endif

/*
 * Types
 */

enum cr_log_type {
    /* NONE used as sentinal value */
    CR_LOG_TYPE_NONE = 0,
    CR_LOG_TYPE_U64,
    CR_LOG_TYPE_I64,
    CR_LOG_TYPE_BOOL,
    CR_LOG_TYPE_STRING,
};

/*
 * if name = nullptr -> format param
 * else kv pair
 */
typedef struct cr_log_field {
    // null to use this as a format param
    char            *name;
    enum cr_log_type type;
    union {
        u64   u;
        i64   i;
        f64   d;
        b8    b;
        char *s;
    } value;
} cr_log_field;

static constexpr usize item_buffer_size
    = CR_LOG_ITEM_SIZE - (sizeof(cr_log_field) * CR_LOG_ITEM_FIELDS) - (sizeof(char *) * 2);

typedef struct cr_log_item {
    alignas(CR_LOG_ITEM_SIZE) char *message;
    cr_log_field fields[CR_LOG_ITEM_FIELDS];

    // Arena for strings
    char *arena_ptr;
    char  arena[item_buffer_size];
} cr_log_item;

/*
 * Asserts
 */

static_assert(item_buffer_size > 0, "CR_LOG_ITEM_SIZE is too small, reconfigure it or adjust CR_LOG_ITEM_FIELDS");

static_assert(
    sizeof(cr_log_item) == CR_LOG_ITEM_SIZE,
    "sizeof cr_log_item is not equal to the size configured by CR_LOG_ITEM_SIZE");

/*
 * Macros
 */

#define CR_VALUE(v)                                                                                                    \
    _Generic(                                                                                                          \
        (0, v),                                                                                                        \
        int8_t: cr_i64,                                                                                                \
        int16_t: cr_i64,                                                                                               \
        int32_t: cr_i64,                                                                                               \
        int64_t: cr_i64,                                                                                               \
        uint8_t: cr_u64,                                                                                               \
        uint16_t: cr_u64,                                                                                              \
        uint32_t: cr_u64,                                                                                              \
        uint64_t: cr_u64,                                                                                              \
        bool: cr_bool,                                                                                                 \
        char *: cr_str)(v)

#define CR_KV(k, v)                                                                                                    \
    ({                                                                                                                 \
        cr_log_field value = CR_VALUE(v);                                                                              \
        (cr_log_field) { .name = (k), .type = value.type, .value = value.value };                                      \
    })

#define CR_VAR(var) CR_KV(#var, var)

/*
 * Declarations
 */

cr_log_field cr_i64(i64 value);
cr_log_field cr_u64(u64 value);
cr_log_field cr_bool(bool value);
cr_log_field cr_str(char *value);

char *cr_log_item_strdup(cr_log_item *item, const char *str);

/*
 * Definations
 */

cr_log_field
cr_i64(i64 value)
{
    return (cr_log_field) { .name = nullptr, .type = CR_LOG_TYPE_I64, .value.i = value };
}

cr_log_field
cr_u64(u64 value)
{
    return (cr_log_field) { .name = nullptr, .type = CR_LOG_TYPE_U64, .value.u = value };
}

cr_log_field
cr_bool(bool value)
{
    return (cr_log_field) { .name = nullptr, .type = CR_LOG_TYPE_BOOL, .value.b = value };
}

cr_log_field
cr_str(char *value)
{
    return (cr_log_field) { .name = nullptr, .type = CR_LOG_TYPE_STRING, .value.s = value };
}

char *
cr_log_item_strdup(cr_log_item *item, const char *str)
{
    usize len = strlen(str) + 1; // adjust for '\0'
    if (likely(item->arena_ptr + len <= item->arena + item_buffer_size)) {
        char *copy = memcpy(item->arena_ptr, str, len);
        item->arena_ptr += len;
        return copy;
    }
    return NULL;
}

/*
 * Testing
 */

void
print_kv(cr_log_field kv)
{
    switch (kv.type) {
    case CR_LOG_TYPE_I64:
        printf("%s = %" PRIi64 "\n", kv.name, kv.value.i);
        break;
    case CR_LOG_TYPE_U64:
        printf("%s = %" PRIu64 "\n", kv.name, kv.value.u);
        break;
    case CR_LOG_TYPE_BOOL:
        printf("%s = %s\n", kv.name, (!!kv.value.b) ? "true" : "false");
        break;
    case CR_LOG_TYPE_STRING:
        printf("%s = %s\n", kv.name, kv.value.s);
        break;
    default:
        break;
    }
}

int
main(void)
{
    char *name = "felix";
    print_kv(CR_VAR(name));
    print_kv(CR_KV("age", 19));
    auto field = CR_VALUE(19);
    if (field.type == CR_LOG_TYPE_I64) {
        printf("field.value = %" PRIi64 "\n", field.value.i);
    }
}
