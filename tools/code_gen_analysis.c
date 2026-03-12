#include "code_gen_analysis.h"
#include "code_gen_utils.h"
#include <stdlib.h>
#include <string.h>

int is_static_length(const char *value)
{
    if (!value || value[0] == '\0')
    {
        return 0;
    }
    for (const char *ch = value; *ch; ++ch)
    {
        if (*ch < '0' || *ch > '9')
        {
            return 0;
        }
    }
    return 1;
}

int is_numeric_string(const char *value)
{
    if (!value || value[0] == '\0')
    {
        return 0;
    }
    for (const char *ch = value; *ch; ++ch)
    {
        if (*ch < '0' || *ch > '9')
        {
            return 0;
        }
    }
    return 1;
}

int is_primitive_type(const char *data_type)
{
    if (!data_type)
    {
        return 0;
    }
    return strcmp(data_type, "byte") == 0 || strcmp(data_type, "char") == 0 ||
           strcmp(data_type, "short") == 0 || strcmp(data_type, "three") == 0 ||
           strcmp(data_type, "int") == 0 || strcmp(data_type, "bool") == 0 ||
           strcmp(data_type, "string") == 0 || strcmp(data_type, "encoded_string") == 0 ||
           strcmp(data_type, "blob") == 0;
}

const char *map_primitive_type(const char *data_type)
{
    if (strcmp(data_type, "byte") == 0)
    {
        return "uint8_t";
    }
    if (strcmp(data_type, "char") == 0 || strcmp(data_type, "short") == 0 ||
        strcmp(data_type, "three") == 0 || strcmp(data_type, "int") == 0)
    {
        return "int32_t";
    }
    if (strcmp(data_type, "bool") == 0)
    {
        return "bool";
    }
    if (strcmp(data_type, "string") == 0 || strcmp(data_type, "encoded_string") == 0)
    {
        return "char *";
    }
    if (strcmp(data_type, "blob") == 0)
    {
        return "uint8_t";
    }
    return data_type;
}

const char *map_writer_fn(const char *data_type)
{
    if (strcmp(data_type, "byte") == 0)
    {
        return "eo_writer_add_byte";
    }
    if (strcmp(data_type, "char") == 0)
    {
        return "eo_writer_add_char";
    }
    if (strcmp(data_type, "short") == 0)
    {
        return "eo_writer_add_short";
    }
    if (strcmp(data_type, "three") == 0)
    {
        return "eo_writer_add_three";
    }
    if (strcmp(data_type, "int") == 0)
    {
        return "eo_writer_add_int";
    }
    if (strcmp(data_type, "string") == 0)
    {
        return "eo_writer_add_string";
    }
    if (strcmp(data_type, "encoded_string") == 0)
    {
        return "eo_writer_add_encoded_string";
    }
    return "eo_writer_add_int";
}

const char *map_reader_fn(const char *data_type)
{
    if (strcmp(data_type, "byte") == 0)
    {
        return "eo_reader_get_byte";
    }
    if (strcmp(data_type, "char") == 0)
    {
        return "eo_reader_get_char";
    }
    if (strcmp(data_type, "short") == 0)
    {
        return "eo_reader_get_short";
    }
    if (strcmp(data_type, "three") == 0)
    {
        return "eo_reader_get_three";
    }
    if (strcmp(data_type, "int") == 0)
    {
        return "eo_reader_get_int";
    }
    if (strcmp(data_type, "string") == 0)
    {
        return "eo_reader_get_string";
    }
    if (strcmp(data_type, "encoded_string") == 0)
    {
        return "eo_reader_get_encoded_string";
    }
    return "eo_reader_get_int";
}

int primitive_byte_size(const char *data_type)
{
    if (!data_type)
        return 0;
    if (strcmp(data_type, "byte") == 0 || strcmp(data_type, "char") == 0 ||
        strcmp(data_type, "bool") == 0)
        return 1;
    if (strcmp(data_type, "short") == 0)
        return 2;
    if (strcmp(data_type, "three") == 0)
        return 3;
    if (strcmp(data_type, "int") == 0)
        return 4;
    return 0;
}

char *normalize_type_name(const char *data_type)
{
    if (!data_type)
    {
        return xstrdup("");
    }
    const char *colon = strchr(data_type, ':');
    if (!colon)
    {
        return xstrdup(data_type);
    }
    size_t len = (size_t)(colon - data_type);
    char *out = xmalloc(len + 1);
    memcpy(out, data_type, len);
    out[len] = '\0';
    return out;
}

char *get_type_override(const char *data_type)
{
    if (!data_type)
        return NULL;
    const char *colon = strchr(data_type, ':');
    if (!colon)
        return NULL;
    return xstrdup(colon + 1);
}

int enum_exists(EnumDef *enums, size_t enums_count, const char *name)
{
    for (size_t i = 0; i < enums_count; ++i)
    {
        if (strcmp(enums[i].name, name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int struct_exists(StructDef *structs, size_t structs_count, const char *name)
{
    for (size_t i = 0; i < structs_count; ++i)
    {
        if (strcmp(structs[i].name, name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int element_list_has_storage(ElementList *elements);

int struct_has_storage(StructDef *structs, size_t structs_count, const char *name)
{
    for (size_t i = 0; i < structs_count; ++i)
    {
        if (strcmp(structs[i].name, name) == 0)
        {
            return element_list_has_storage(&structs[i].elements);
        }
    }
    return 0;
}

int switch_has_storage(SwitchDef *sw)
{
    for (size_t i = 0; i < sw->cases_count; ++i)
    {
        CaseDef *case_def = &sw->cases[i];
        if (!case_def->has_elements)
        {
            continue;
        }
        if (element_list_has_storage(&case_def->elements))
        {
            return 1;
        }
    }
    return 0;
}

int switch_has_numeric_case(SwitchDef *sw)
{
    for (size_t i = 0; i < sw->cases_count; ++i)
    {
        CaseDef *case_def = &sw->cases[i];
        if (!case_def->has_elements || case_def->is_default)
        {
            continue;
        }
        if (case_def->value && is_numeric_string(case_def->value))
        {
            return 1;
        }
    }
    return 0;
}

int element_list_has_storage(ElementList *elements)
{
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];
        if (element->kind == ELEMENT_FIELD)
        {
            if (element->as.field.name)
            {
                return 1;
            }
        }
        else if (element->kind == ELEMENT_ARRAY)
        {
            if (element->as.array.name)
            {
                return 1;
            }
        }
        else if (element->kind == ELEMENT_SWITCH)
        {
            if (switch_has_storage(&element->as.sw))
            {
                return 1;
            }
        }
        else if (element->kind == ELEMENT_CHUNKED)
        {
            if (element_list_has_storage(&element->as.chunked))
            {
                return 1;
            }
        }
    }
    return 0;
}

int element_list_has_heap(ElementList *elements, EnumDef *enums, size_t enums_count,
                                 StructDef *structs, size_t structs_count);

int struct_has_heap(StructDef *structs, size_t structs_count, const char *name,
                    EnumDef *enums, size_t enums_count)
{
    for (size_t i = 0; i < structs_count; ++i)
    {
        if (strcmp(structs[i].name, name) == 0)
        {
            return element_list_has_heap(&structs[i].elements, enums, enums_count,
                                         structs, structs_count);
        }
    }
    return 0;
}

int element_list_has_heap(ElementList *elements, EnumDef *enums, size_t enums_count,
                          StructDef *structs, size_t structs_count)
{
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];
        if (element->kind == ELEMENT_FIELD)
        {
            FieldDef *field = &element->as.field;
            if (!field->name)
                continue;
            char *type_name = normalize_type_name(field->data_type);
            int heap = 0;
            if (strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0 ||
                strcmp(type_name, "blob") == 0)
            {
                heap = 1;
            }
            else if (!is_primitive_type(type_name) && !enum_exists(enums, enums_count, type_name))
            {
                heap = struct_has_heap(structs, structs_count, type_name, enums, enums_count);
            }
            free(type_name);
            if (heap)
                return 1;
        }
        else if (element->kind == ELEMENT_ARRAY)
        {
            ArrayDef *array = &element->as.array;
            if (!array->name)
                continue;
            char *type_name = normalize_type_name(array->data_type);
            int heap = 0;
            if (!is_static_length(array->length))
            {
                heap = 1;
            }
            else if (strcmp(type_name, "string") == 0 ||
                     strcmp(type_name, "encoded_string") == 0)
            {
                heap = 1;
            }
            else if (!is_primitive_type(type_name) && !enum_exists(enums, enums_count, type_name))
            {
                heap = struct_has_heap(structs, structs_count, type_name, enums, enums_count);
            }
            free(type_name);
            if (heap)
                return 1;
        }
        else if (element->kind == ELEMENT_SWITCH)
        {
            if (switch_has_storage(&element->as.sw))
                return 1;
        }
        else if (element->kind == ELEMENT_CHUNKED)
        {
            if (element_list_has_heap(&element->as.chunked, enums, enums_count,
                                      structs, structs_count))
                return 1;
        }
    }
    return 0;
}

const char *find_enum_data_type(EnumDef *enums, size_t enums_count, const char *name)
{
    for (size_t i = 0; i < enums_count; ++i)
    {
        if (strcmp(enums[i].name, name) == 0)
        {
            return enums[i].data_type;
        }
    }
    return "int";
}

char *find_field_data_type(ElementList *elements, const char *name)
{
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];
        if (element->kind == ELEMENT_FIELD && element->as.field.name &&
            strcmp(element->as.field.name, name) == 0)
        {
            return normalize_type_name(element->as.field.data_type);
        }
        if (element->kind == ELEMENT_CHUNKED)
        {
            char *nested = find_field_data_type(&element->as.chunked, name);
            if (nested)
            {
                return nested;
            }
        }
    }
    return NULL;
}

LengthTarget find_length_target(ElementList *elements, const char *length_name)
{
    LengthTarget target = {0};
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];
        if (element->kind == ELEMENT_FIELD && element->as.field.length &&
            strcmp(element->as.field.length, length_name) == 0)
        {
            target.name = xstrdup(element->as.field.name);
            target.data_type = normalize_type_name(element->as.field.data_type);
            target.is_array = 0;
            return target;
        }
        if (element->kind == ELEMENT_ARRAY && element->as.array.length &&
            strcmp(element->as.array.length, length_name) == 0)
        {
            target.name = xstrdup(element->as.array.name);
            target.data_type = normalize_type_name(element->as.array.data_type);
            target.is_array = 1;
            target.is_static = is_static_length(element->as.array.length);
            if (target.is_static)
            {
                target.static_length = xstrdup(element->as.array.length);
            }
            return target;
        }
        if (element->kind == ELEMENT_CHUNKED)
        {
            target = find_length_target(&element->as.chunked, length_name);
            if (target.name)
            {
                return target;
            }
        }
    }
    return target;
}
