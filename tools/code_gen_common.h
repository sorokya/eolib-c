#ifndef CODE_GEN_COMMON_H
#define CODE_GEN_COMMON_H

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#include <direct.h>
#include <windows.h>
#define EOLIB_PATH_MAX MAX_PATH
#else
#include <dirent.h>
#include <glob.h>
#include <limits.h>
#define EOLIB_PATH_MAX PATH_MAX
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ARRAY_GROW_CAPACITY(current) ((current) < 8 ? 8 : (current) * 2)

extern const char *CODEGEN_WARNING;

typedef struct
{
    char *name;
    char *data_type;
    char *comment;
    int value;
} EnumValue;

typedef struct
{
    char *name;
    char *data_type;
    char *comment;
    EnumValue *values;
    size_t values_count;
    size_t values_capacity;
} EnumDef;

typedef enum
{
    ELEMENT_BREAK,
    ELEMENT_CHUNKED,
    ELEMENT_COMMENT,
    ELEMENT_DUMMY,
    ELEMENT_FIELD,
    ELEMENT_ARRAY,
    ELEMENT_LENGTH,
    ELEMENT_SWITCH
} ElementKind;

typedef struct StructElement StructElement;

typedef struct
{
    StructElement *elements;
    size_t elements_count;
    size_t elements_capacity;
} ElementList;

typedef struct
{
    char *name;
    char *data_type;
    char *value;
    char *comment;
    int padded;
    int optional;
    char *length;
} FieldDef;

typedef struct
{
    char *name;
    char *data_type;
    char *length;
    int optional;
    int delimited;
    int trailing_delimiter;
    char *comment;
} ArrayDef;

typedef struct
{
    char *name;
    char *data_type;
    int optional;
    int offset;
} LengthDef;

typedef struct
{
    char *data_type;
    char *value;
} DummyDef;

typedef struct
{
    int is_default;
    char *value;
    ElementList elements;
    int has_elements;
} CaseDef;

typedef struct
{
    char *field;
    CaseDef *cases;
    size_t cases_count;
    size_t cases_capacity;
} SwitchDef;

struct StructElement
{
    ElementKind kind;
    union
    {
        ElementList chunked;
        char *comment;
        DummyDef dummy;
        FieldDef field;
        ArrayDef array;
        LengthDef length;
        SwitchDef sw;
    } as;
};

typedef struct
{
    char *name;
    ElementList elements;
} StructDef;

typedef struct
{
    char *action;
    char *family;
    ElementList elements;
} PacketDef;

typedef struct
{
    char *path;
    EnumDef *enums;
    size_t enums_count;
    size_t enums_capacity;
    StructDef *structs;
    size_t structs_count;
    size_t structs_capacity;
    PacketDef *packets;
    size_t packets_count;
    size_t packets_capacity;
} ProtocolDef;

typedef struct
{
    char **items;
    size_t count;
    size_t capacity;
} StringList;

typedef struct
{
    char *name;
    char *data_type;
    int is_array;
    int is_static;
    char *static_length;
} LengthTarget;

#endif /* CODE_GEN_COMMON_H */
