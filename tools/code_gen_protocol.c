#include "code_gen_protocol.h"
#include "code_gen_utils.h"
#include "code_gen_analysis.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

static void write_serialize_elements(FILE *source, const char *struct_name,
                                     ElementList *elements, EnumDef *enums,
                                     size_t enums_count, StructDef *structs,
                                     size_t structs_count, const char *value_expr,
                                     const char *value_access);
static void write_deserialize_elements(FILE *source, const char *struct_name,
                                       ElementList *elements, EnumDef *enums,
                                       size_t enums_count, StructDef *structs,
                                       size_t structs_count, const char *out_expr,
                                       const char *out_access);
static void write_free_elements(FILE *source, const char *struct_name,
                                ElementList *elements, EnumDef *enums,
                                size_t enums_count, StructDef *structs,
                                size_t structs_count, const char *value_expr,
                                const char *value_access);
static void write_size_elements(FILE *source, const char *struct_name,
                                ElementList *elements, EnumDef *enums,
                                size_t enums_count, StructDef *structs,
                                size_t structs_count, const char *value_expr,
                                const char *value_access);
static void write_struct_def(FILE *header, FILE *source, const char *name, ElementList *elements,
                             EnumDef *enums, size_t enums_count, StructDef *structs,
                             size_t structs_count, const char *comment,
                             const char *packet_family, const char *packet_action);

static void write_enum_def(FILE *header, FILE *source, EnumDef *def)
{
    (void)source;
    if (def->comment)
    {
        write_doc_comment(header, def->comment, "");
    }

    char *enum_prefix = to_upper_snake(def->name);
    fprintf(header, "typedef enum %s {\n", def->name);
    for (size_t i = 0; i < def->values_count; ++i)
    {
        EnumValue *value = &def->values[i];
        if (value->comment)
        {
            write_doc_comment(header, value->comment, "    ");
        }
        char *constant_name = to_upper_snake(value->name);
        fprintf(header, "    %s_%s = %d%s\n",
                enum_prefix,
                constant_name,
                value->value,
                i + 1 == def->values_count ? "" : ",");
        free(constant_name);
    }
    fprintf(header, "} %s;\n\n", def->name);
    free(enum_prefix);
}

static void write_struct_fields_with_indent(FILE *header, const char *struct_name,
                                            ElementList *elements, EnumDef *enums,
                                            size_t enums_count, StructDef *structs,
                                            size_t structs_count, const char *indent)
{
    (void)struct_name;
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];
        if (element->kind == ELEMENT_FIELD)
        {
            FieldDef *field = &element->as.field;
            if (!field->name)
            {
                continue;
            }

            char *field_name = to_snake_case(field->name);
            char *type_name = normalize_type_name(field->data_type);
            const char *mapped_type = is_primitive_type(type_name) ? map_primitive_type(type_name)
                                                                   : type_name;

            write_doc_comment(header, field->comment, indent);

            if (enum_exists(enums, enums_count, type_name))
            {
                fprintf(header, "%s%s %s%s;\n", indent, type_name, field->optional ? "*" : "", field_name);
            }
            else if (struct_exists(structs, structs_count, type_name))
            {
                fprintf(header, "%s%s %s%s;\n", indent, type_name, field->optional ? "*" : "", field_name);
            }
            else if (strcmp(type_name, "blob") == 0)
            {
                fprintf(header, "%ssize_t %s_length;\n", indent, field_name);
                fprintf(header, "%suint8_t *%s;\n", indent, field_name);
            }
            else
            {
                int is_string_type = strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0;
                if (field->optional && !is_string_type)
                {
                    fprintf(header, "%s%s *%s;\n", indent, mapped_type, field_name);
                }
                else
                {
                    fprintf(header, "%s%s %s;\n", indent, mapped_type, field_name);
                }
            }

            free(type_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_ARRAY)
        {
            ArrayDef *array = &element->as.array;
            char *field_name = to_snake_case(array->name);
            char *type_name = normalize_type_name(array->data_type);
            const char *mapped_type = is_primitive_type(type_name) ? map_primitive_type(type_name)
                                                                   : type_name;
            if (enum_exists(enums, enums_count, type_name))
            {
                mapped_type = type_name;
            }

            write_doc_comment(header, array->comment, indent);

            if (is_static_length(array->length))
            {
                fprintf(header, "%s%s %s[%s];\n", indent, mapped_type, field_name, array->length);
            }
            else
            {
                fprintf(header, "%ssize_t %s_length;\n", indent, field_name);
                fprintf(header, "%s%s *%s;\n", indent, mapped_type, field_name);
            }

            free(type_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_SWITCH)
        {
            SwitchDef *sw = &element->as.sw;
            char *field_name = to_snake_case(sw->field);
            int has_storage = switch_has_storage(sw);
            if (!has_storage)
            {
                free(field_name);
                continue;
            }
            char union_indent[128];
            char struct_indent[128];
            char field_indent[128];
            snprintf(union_indent, sizeof(union_indent), "%s", indent);
            snprintf(struct_indent, sizeof(struct_indent), "%s    ", indent);
            snprintf(field_indent, sizeof(field_indent), "%s        ", indent);
            fprintf(header, "%sunion {\n", union_indent);
            for (size_t c = 0; c < sw->cases_count; ++c)
            {
                CaseDef *case_def = &sw->cases[c];
                if (!case_def->has_elements || !element_list_has_storage(&case_def->elements))
                {
                    continue;
                }
                const char *case_name = case_def->is_default ? "default" : case_def->value;
                char *case_field = to_snake_case(case_name);
                fprintf(header, "%sstruct {\n", struct_indent);
                write_struct_fields_with_indent(header, struct_name, &case_def->elements, enums,
                                                enums_count, structs, structs_count,
                                                field_indent);
                fprintf(header, "%s} %s;\n", struct_indent, case_field);
                free(case_field);
            }
            fprintf(header, "%s} *%s_data;\n", union_indent, field_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_CHUNKED)
        {
            write_struct_fields_with_indent(header, struct_name, &element->as.chunked, enums,
                                            enums_count, structs, structs_count, indent);
        }
    }
}

static void write_struct_fields(FILE *header, const char *struct_name, ElementList *elements,
                                EnumDef *enums, size_t enums_count, StructDef *structs,
                                size_t structs_count)
{
    write_struct_fields_with_indent(header, struct_name, elements, enums, enums_count, structs,
                                    structs_count, "    ");
}

static void write_serialize_elements(FILE *source, const char *struct_name,
                                     ElementList *elements, EnumDef *enums,
                                     size_t enums_count, StructDef *structs,
                                     size_t structs_count, const char *value_expr,
                                     const char *value_access)
{
    (void)struct_name;
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];
        if (element->kind == ELEMENT_CHUNKED)
        {
            fprintf(source, "    eo_writer_set_string_sanitization_mode(writer, true);\n");
            write_serialize_elements(source, struct_name, &element->as.chunked, enums,
                                     enums_count, structs, structs_count, value_expr,
                                     value_access);
            continue;
        }
        if (element->kind == ELEMENT_BREAK)
        {
            fprintf(source,
                    "    if ((result = eo_writer_add_byte(writer, EO_BREAK_BYTE)) != 0) return result;\n");
        }
        else if (element->kind == ELEMENT_DUMMY)
        {
            DummyDef *dummy = &element->as.dummy;
            if (strcmp(dummy->data_type, "string") == 0 ||
                strcmp(dummy->data_type, "encoded_string") == 0)
            {
                fprintf(source,
                        "    if ((result = %s(writer, \"%s\")) != 0) return result;\n",
                        map_writer_fn(dummy->data_type), dummy->value ? dummy->value : "");
            }
            else
            {
                fprintf(source,
                        "    if ((result = %s(writer, %s)) != 0) return result;\n",
                        map_writer_fn(dummy->data_type), dummy->value ? dummy->value : "0");
            }
        }
        else if (element->kind == ELEMENT_FIELD)
        {
            FieldDef *field = &element->as.field;
            if (!field->name)
            {
                char *type_name = normalize_type_name(field->data_type);
                if (strcmp(type_name, "string") == 0 ||
                    strcmp(type_name, "encoded_string") == 0)
                {
                    fprintf(source,
                            "    if ((result = %s(writer, \"%s\")) != 0) return result;\n",
                            map_writer_fn(type_name), field->value ? field->value : "");
                }
                else
                {
                    fprintf(source,
                            "    if ((result = %s(writer, %s)) != 0) return result;\n",
                            map_writer_fn(type_name), field->value ? field->value : "0");
                }
                free(type_name);
                continue;
            }
            char *field_name = to_snake_case(field->name);
            char *type_name = normalize_type_name(field->data_type);
            int is_enum = enum_exists(enums, enums_count, type_name);
            int is_struct = struct_exists(structs, structs_count, type_name);

            if (field->optional)
            {
                fprintf(source, "    if (%s%s%s) {\n", value_expr, value_access, field_name);
            }
            if (is_enum)
            {
                const char *enum_data_type = find_enum_data_type(enums, enums_count, type_name);
                char *type_override = get_type_override(field->data_type);
                const char *effective_type = type_override ? type_override : enum_data_type;
                char *enum_prefix = to_upper_snake(type_name);
                char value_buffer[512];
                if (field->value)
                {
                    if (is_numeric_string(field->value))
                    {
                        snprintf(value_buffer, sizeof(value_buffer), "%s", field->value);
                    }
                    else
                    {
                        char *enum_const = to_upper_snake(field->value);
                        snprintf(value_buffer, sizeof(value_buffer), "%s_%s", enum_prefix,
                                 enum_const);
                        free(enum_const);
                    }
                }
                else
                {
                    snprintf(value_buffer, sizeof(value_buffer), field->optional ? "*%s%s%s" : "%s%s%s",
                             value_expr, value_access, field_name);
                }
                fprintf(source,
                        "    if ((result = %s(writer, %s)) != 0) return result;\n",
                        map_writer_fn(effective_type), value_buffer);
                free(enum_prefix);
                free(type_override);
            }
            else if (is_struct)
            {
                if (field->optional)
                    fprintf(source,
                            "    if ((result = %s_serialize_fn((const EoSerialize *)%s%s%s, writer)) != 0) return result;\n",
                            type_name, value_expr, value_access, field_name);
                else
                    fprintf(source,
                            "    if ((result = %s_serialize_fn((const EoSerialize *)&%s%s%s, writer)) != 0) return result;\n",
                            type_name, value_expr, value_access, field_name);
            }
            else if (strcmp(type_name, "bool") == 0)
            {
                char value_buffer[128];
                if (field->value)
                {
                    if (strcmp(field->value, "true") == 0)
                    {
                        snprintf(value_buffer, sizeof(value_buffer), "1");
                    }
                    else if (strcmp(field->value, "false") == 0)
                    {
                        snprintf(value_buffer, sizeof(value_buffer), "0");
                    }
                    else
                    {
                        snprintf(value_buffer, sizeof(value_buffer), "%s", field->value);
                    }
                }
                else
                {
                    if (field->optional)
                        snprintf(value_buffer, sizeof(value_buffer), "*%s%s%s ? 1 : 0", value_expr,
                                 value_access, field_name);
                    else
                        snprintf(value_buffer, sizeof(value_buffer), "%s%s%s ? 1 : 0", value_expr,
                                 value_access, field_name);
                }
                fprintf(source,
                        "    if ((result = eo_writer_add_char(writer, %s)) != 0) return result;\n",
                        value_buffer);
            }
            else if (strcmp(type_name, "blob") == 0)
            {
                fprintf(source,
                        "    if ((result = eo_writer_add_bytes(writer, %s%s%s, %s%s%s_length)) != 0) return result;\n",
                        value_expr, value_access, field_name, value_expr, value_access,
                        field_name);
            }
            else if (field->length && isdigit((unsigned char)field->length[0]) &&
                     (strcmp(type_name, "string") == 0 ||
                      strcmp(type_name, "encoded_string") == 0))
            {
                char value_buffer[512];
                if (field->value)
                {
                    snprintf(value_buffer, sizeof(value_buffer), "\"%s\"", field->value);
                }
                else
                {
                    snprintf(value_buffer, sizeof(value_buffer), "%s%s%s", value_expr,
                             value_access, field_name);
                }
                fprintf(source,
                        "    if ((result = eo_writer_add_fixed_%s(writer, %s, (size_t)%s, %s)) != 0) return result;\n",
                        strcmp(type_name, "string") == 0 ? "string" : "encoded_string",
                        value_buffer, field->length, field->padded ? "true" : "false");
            }
            else
            {
                char value_buffer[512];
                if (field->value)
                {
                    if (strcmp(type_name, "string") == 0 ||
                        strcmp(type_name, "encoded_string") == 0)
                    {
                        snprintf(value_buffer, sizeof(value_buffer), "\"%s\"",
                                 field->value);
                    }
                    else
                    {
                        snprintf(value_buffer, sizeof(value_buffer), "%s", field->value);
                    }
                }
                else
                {
                    int is_string_type_el = strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0;
                    if (field->optional && !is_string_type_el)
                        snprintf(value_buffer, sizeof(value_buffer), "*%s%s%s", value_expr,
                                 value_access, field_name);
                    else
                        snprintf(value_buffer, sizeof(value_buffer), "%s%s%s", value_expr,
                                 value_access, field_name);
                }
                fprintf(source,
                        "    if ((result = %s(writer, %s)) != 0) return result;\n",
                        map_writer_fn(type_name), value_buffer);
            }

            if (field->optional)
            {
                fprintf(source, "    }\n");
            }

            free(type_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_LENGTH)
        {
            LengthDef *length = &element->as.length;
            LengthTarget target = find_length_target(elements, length->name);
            char *field_name = target.name ? to_snake_case(target.name) : xstrdup(length->name);
            const char *len_expr = "0";
            char buffer[256];

            if (target.name)
            {
                if (target.is_array)
                {
                    if (target.is_static && target.static_length)
                    {
                        len_expr = target.static_length;
                    }
                    else
                    {
                        snprintf(buffer, sizeof(buffer), "%s%s%s_length", value_expr,
                                 value_access, field_name);
                        len_expr = buffer;
                    }
                }
                else if (target.data_type &&
                         (strcmp(target.data_type, "string") == 0 ||
                          strcmp(target.data_type, "encoded_string") == 0))
                {
                    snprintf(buffer, sizeof(buffer), "strlen(%s%s%s)", value_expr,
                             value_access, field_name);
                    len_expr = buffer;
                }
                else if (target.data_type && strcmp(target.data_type, "blob") == 0)
                {
                    snprintf(buffer, sizeof(buffer), "%s%s%s_length", value_expr,
                             value_access, field_name);
                    len_expr = buffer;
                }
                else
                {
                    snprintf(buffer, sizeof(buffer), "%s%s%s", value_expr, value_access,
                             field_name);
                    len_expr = buffer;
                }
            }

            if (length->optional)
            {
                fprintf(source, "    if (%s%s%s) {\n", value_expr, value_access,
                        length->name);
            }

            fprintf(source,
                    "    if ((result = %s(writer, (int32_t)(%s %s %d))) != 0) return result;\n",
                    map_writer_fn(length->data_type), len_expr,
                    length->offset >= 0 ? "+" : "-", abs(length->offset));

            if (length->optional)
            {
                fprintf(source, "    }\n");
            }

            free(field_name);
            free(target.name);
            free(target.data_type);
            free(target.static_length);
        }
        else if (element->kind == ELEMENT_ARRAY)
        {
            ArrayDef *array = &element->as.array;
            char *field_name = to_snake_case(array->name);
            char *type_name = normalize_type_name(array->data_type);
            int is_enum = enum_exists(enums, enums_count, type_name);
            int is_struct = struct_exists(structs, structs_count, type_name);
            int is_static = is_static_length(array->length);
            const char *item_c_type = map_primitive_type(type_name);
            if (is_enum)
            {
                item_c_type = type_name;
            }

            if (array->optional)
            {
                fprintf(source, "    if (%s%s%s) {\n", value_expr, value_access, field_name);
            }

            if (is_static)
            {
                fprintf(source, "    for (size_t i = 0; i < %s; ++i) {\n", array->length);
            }
            else
            {
                fprintf(source, "    for (size_t i = 0; i < %s%s%s_length; ++i) {\n",
                        value_expr, value_access, field_name);
            }

            if (array->delimited && !array->trailing_delimiter)
            {
                fprintf(source, "        if (i > 0) {\n");
                fprintf(source,
                        "            if ((result = eo_writer_add_byte(writer, EO_BREAK_BYTE)) != 0) return result;\n");
                fprintf(source, "        }\n");
            }

            if (is_enum)
            {
                const char *enum_data_type = find_enum_data_type(enums, enums_count, type_name);
                if (is_static)
                {
                    fprintf(source,
                            "        if ((result = %s(writer, %s%s%s[i])) != 0) return result;\n",
                            map_writer_fn(enum_data_type), value_expr, value_access, field_name);
                }
                else
                {
                    fprintf(source,
                            "        if ((result = %s(writer, %s%s%s[i])) != 0) return result;\n",
                            map_writer_fn(enum_data_type), value_expr, value_access, field_name);
                }
            }
            else if (is_struct)
            {
                fprintf(source,
                        "        if ((result = %s_serialize_fn((const EoSerialize *)&%s%s%s[i], writer)) != 0) return result;\n",
                        type_name, value_expr, value_access, field_name);
            }
            else if (strcmp(type_name, "bool") == 0)
            {
                if (is_static)
                {
                    fprintf(source,
                            "        if ((result = eo_writer_add_char(writer, %s%s%s[i] ? 1 : 0)) != 0) return result;\n",
                            value_expr, value_access, field_name);
                }
                else
                {
                    fprintf(source,
                            "        if ((result = eo_writer_add_char(writer, %s%s%s[i] ? 1 : 0)) != 0) return result;\n",
                            value_expr, value_access, field_name);
                }
            }
            else if (strcmp(type_name, "blob") == 0)
            {
                if (is_static)
                {
                    fprintf(source,
                            "        if ((result = eo_writer_add_bytes(writer, %s%s%s[i].data, %s%s%s[i].length)) != 0) return result;\n",
                            value_expr, value_access, field_name, value_expr, value_access,
                            field_name);
                }
                else
                {
                    fprintf(source,
                            "        if ((result = eo_writer_add_bytes(writer, %s%s%s[i].data, %s%s%s[i].length)) != 0) return result;\n",
                            value_expr, value_access, field_name, value_expr, value_access,
                            field_name);
                }
            }
            else
            {
                if (is_static)
                {
                    fprintf(source,
                            "        if ((result = %s(writer, %s%s%s[i])) != 0) return result;\n",
                            map_writer_fn(type_name), value_expr, value_access, field_name);
                }
                else
                {
                    fprintf(source,
                            "        if ((result = %s(writer, %s%s%s[i])) != 0) return result;\n",
                            map_writer_fn(type_name), value_expr, value_access, field_name);
                }
            }

            if (array->delimited && array->trailing_delimiter)
            {
                fprintf(source,
                        "        if ((result = eo_writer_add_byte(writer, EO_BREAK_BYTE)) != 0) return result;\n");
            }

            fprintf(source, "    }\n");
            if (array->optional)
            {
                fprintf(source, "    }\n");
            }

            free(type_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_SWITCH)
        {
            SwitchDef *sw = &element->as.sw;
            char *field_name = to_snake_case(sw->field);
            char *field_type = find_field_data_type(elements, sw->field);
            int has_default = 0;
            int has_storage = switch_has_storage(sw);
            int suppress_enum_switch_warning =
                field_type && enum_exists(enums, enums_count, field_type) &&
                switch_has_numeric_case(sw);
            if (suppress_enum_switch_warning)
            {
                write_switch_pragma_push(source);
            }
            fprintf(source, "    switch (%s%s%s) {\n", value_expr, value_access, field_name);
            for (size_t c = 0; c < sw->cases_count; ++c)
            {
                CaseDef *case_def = &sw->cases[c];
                if (!case_def->has_elements)
                {
                    continue;
                }
                int case_has_storage = element_list_has_storage(&case_def->elements);
                if (case_def->is_default)
                {
                    has_default = 1;
                }
                const char *case_name = case_def->is_default ? "default" : case_def->value;
                char *case_const = to_upper_snake(case_name);
                char *case_field = to_snake_case(case_name);
                if (case_def->is_default)
                {
                    fprintf(source, "        default: {\n");
                }
                else if (case_def->value && is_numeric_string(case_def->value))
                {
                    fprintf(source, "        case %s: {\n", case_def->value);
                }
                else if (field_type && enum_exists(enums, enums_count, field_type))
                {
                    char *enum_prefix = to_upper_snake(field_type);
                    fprintf(source, "        case %s_%s: {\n", enum_prefix, case_const);
                    free(enum_prefix);
                }
                else
                {
                    fprintf(source, "        case %s: {\n", case_def->value ? case_def->value : "0");
                }
                if (case_has_storage && has_storage)
                {
                    fprintf(source,
                            "            if (%s%s%s_data) {\n",
                            value_expr, value_access, field_name);
                    {
                        char case_value[512];
                        snprintf(case_value, sizeof(case_value), "%s%s%s_data->%s", value_expr,
                                 value_access, field_name, case_field);
                        write_serialize_elements(source, struct_name, &case_def->elements, enums,
                                                 enums_count, structs, structs_count, case_value,
                                                 ".");
                    }
                    fprintf(source, "            }\n");
                }
                else if (!case_has_storage)
                {
                    write_serialize_elements(source, struct_name, &case_def->elements, enums,
                                             enums_count, structs, structs_count, value_expr,
                                             value_access);
                }
                fprintf(source, "            break;\n        }\n");
                free(case_const);
                free(case_field);
            }
            if (!has_default)
            {
                fprintf(source, "        default: break;\n");
            }
            fprintf(source, "    }\n");
            if (suppress_enum_switch_warning)
            {
                write_switch_pragma_pop(source);
            }
            free(field_name);
            free(field_type);
        }
    }
}

static void write_deserialize_elements(FILE *source, const char *struct_name,
                                       ElementList *elements, EnumDef *enums,
                                       size_t enums_count, StructDef *structs,
                                       size_t structs_count, const char *out_expr,
                                       const char *out_access)
{
    (void)struct_name;
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];
        if (element->kind == ELEMENT_CHUNKED)
        {
            fprintf(source, "    eo_reader_set_chunked_reading_mode(reader, true);\n");
            write_deserialize_elements(source, struct_name, &element->as.chunked, enums,
                                       enums_count, structs, structs_count, out_expr,
                                       out_access);
            continue;
        }
        if (element->kind == ELEMENT_BREAK)
        {
            fprintf(source,
                    "    if ((result = eo_reader_next_chunk(reader)) != 0) return result;\n");
        }
        else if (element->kind == ELEMENT_DUMMY)
        {
            DummyDef *dummy = &element->as.dummy;
            if (strcmp(dummy->data_type, "string") == 0 ||
                strcmp(dummy->data_type, "encoded_string") == 0)
            {
                const char *read_fn = strcmp(dummy->data_type, "string") == 0
                                          ? "eo_reader_get_string"
                                          : "eo_reader_get_encoded_string";
                const char *expected = dummy->value ? dummy->value : "";
                fprintf(source,
                        "    { char *tmp = NULL; if ((result = %s(reader, &tmp)) != 0) return result; "
                        "if (!tmp || strcmp(tmp, \"%s\") != 0) { free(tmp); return EO_INVALID_DATA; } free(tmp); }\n",
                        read_fn, expected);
            }
            else
            {
                const char *expected = dummy->value ? dummy->value : "0";
                fprintf(source,
                        "    { %s tmp = 0; if ((result = %s(reader, &tmp)) != 0) return result; "
                        "if (tmp != (%s)(%s)) return EO_INVALID_DATA; }\n",
                        map_primitive_type(dummy->data_type), map_reader_fn(dummy->data_type),
                        map_primitive_type(dummy->data_type), expected);
            }
        }
        else if (element->kind == ELEMENT_LENGTH)
        {
            LengthDef *length = &element->as.length;
            if (length->optional)
            {
                fprintf(source, "    if (eo_reader_remaining(reader) > 0) {\n");
            }
            fprintf(source,
                    "    int32_t %s = 0; if ((result = %s(reader, &%s)) != 0) return result;\n",
                    length->name, map_reader_fn(length->data_type), length->name);
            if (length->optional)
            {
                fprintf(source, "    }\n");
            }
        }
        else if (element->kind == ELEMENT_FIELD)
        {
            FieldDef *field = &element->as.field;
            if (!field->name)
            {
                char *type_name = normalize_type_name(field->data_type);
                if (strcmp(type_name, "string") == 0 ||
                    strcmp(type_name, "encoded_string") == 0)
                {
                    const char *read_fn = strcmp(type_name, "string") == 0
                                              ? "eo_reader_get_string"
                                              : "eo_reader_get_encoded_string";
                    const char *expected = field->value ? field->value : "";
                    fprintf(source,
                            "    { char *tmp = NULL; if ((result = %s(reader, &tmp)) != 0) return result; "
                            "if (!tmp || strcmp(tmp, \"%s\") != 0) { free(tmp); return EO_INVALID_DATA; } free(tmp); }\n",
                            read_fn, expected);
                }
                else
                {
                    const char *expected = field->value ? field->value : "0";
                    fprintf(source,
                            "    { %s tmp = 0; if ((result = %s(reader, &tmp)) != 0) return result; "
                            "if (tmp != (%s)(%s)) return EO_INVALID_DATA; }\n",
                            map_primitive_type(type_name), map_reader_fn(type_name),
                            map_primitive_type(type_name), expected);
                }
                free(type_name);
                continue;
            }
            char *field_name = to_snake_case(field->name);
            char *type_name = normalize_type_name(field->data_type);
            int is_enum = enum_exists(enums, enums_count, type_name);
            int is_struct = struct_exists(structs, structs_count, type_name);

            if (field->optional)
            {
                fprintf(source, "    if (eo_reader_remaining(reader) > 0) {\n");
                if (is_struct)
                    fprintf(source, "        %s%s%s = calloc(1, sizeof(%s));\n", out_expr, out_access, field_name, type_name);
                else
                    fprintf(source, "        %s%s%s = malloc(sizeof(*%s%s%s));\n", out_expr, out_access, field_name, out_expr, out_access, field_name);
            }

            char addr_buffer[256];
            if (field->optional)
                snprintf(addr_buffer, sizeof(addr_buffer), "%s%s%s", out_expr, out_access, field_name);
            else
                snprintf(addr_buffer, sizeof(addr_buffer), "&%s%s%s", out_expr, out_access, field_name);

            if (is_enum)
            {
                const char *enum_data_type = find_enum_data_type(enums, enums_count, type_name);
                char *type_override = get_type_override(field->data_type);
                const char *effective_type = type_override ? type_override : enum_data_type;
                const char *enum_c_type = map_primitive_type(effective_type);
                fprintf(source,
                        "    if ((result = %s(reader, (%s *)%s)) != 0) return result;\n",
                        map_reader_fn(effective_type), enum_c_type, addr_buffer);
                free(type_override);
            }
            else if (is_struct)
            {
                fprintf(source,
                        "    if ((result = %s_deserialize_fn((EoSerialize *)%s, reader)) != 0) return result;\n",
                        type_name, addr_buffer);
            }
            else if (strcmp(type_name, "bool") == 0)
            {
                if (field->optional)
                    fprintf(source,
                            "    int32_t raw_%s = 0; if ((result = eo_reader_get_char(reader, &raw_%s)) != 0) return result; *%s%s%s = raw_%s == 1;\n",
                            field_name, field_name, out_expr, out_access, field_name, field_name);
                else
                    fprintf(source,
                            "    int32_t raw_%s = 0; if ((result = eo_reader_get_char(reader, &raw_%s)) != 0) return result; %s%s%s = raw_%s == 1;\n",
                            field_name, field_name, out_expr, out_access, field_name, field_name);
            }
            else if (strcmp(type_name, "blob") == 0)
            {
                fprintf(source,
                        "    size_t remaining = eo_reader_remaining(reader); if ((result = eo_reader_get_bytes(reader, remaining, &%s%s%s)) != 0) return result; %s%s%s_length = remaining;\n",
                        out_expr, out_access, field_name, out_expr, out_access, field_name);
            }
            else if (field->length && (strcmp(type_name, "string") == 0 ||
                                       strcmp(type_name, "encoded_string") == 0))
            {
                fprintf(source,
                        "    if ((result = eo_reader_get_fixed_%s(reader, (size_t)%s, &%s%s%s)) != 0) return result;\n",
                        strcmp(type_name, "string") == 0 ? "string" : "encoded_string",
                        field->length, out_expr, out_access, field_name);
            }
            else
            {
                fprintf(source,
                        "    if ((result = %s(reader, %s)) != 0) return result;\n",
                        map_reader_fn(type_name), addr_buffer);
            }

            if (field->optional)
            {
                fprintf(source, "    }\n");
            }

            free(type_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_ARRAY)
        {
            ArrayDef *array = &element->as.array;
            char *field_name = to_snake_case(array->name);
            char *type_name = normalize_type_name(array->data_type);
            int is_enum = enum_exists(enums, enums_count, type_name);
            int is_struct = struct_exists(structs, structs_count, type_name);
            int is_static = is_static_length(array->length);
            const char *item_c_type = map_primitive_type(type_name);
            if (is_enum)
            {
                item_c_type = type_name;
            }

            if (array->optional)
            {
                fprintf(source, "    if (eo_reader_remaining(reader) > 0) {\n");
            }

            if (is_static)
            {
                fprintf(source, "    for (size_t i = 0; i < %s; ++i) {\n", array->length);
            }
            else if (array->length)
            {
                fprintf(source,
                        "    %s%s%s_length = (size_t)%s;\n",
                        out_expr, out_access, field_name, array->length);
                fprintf(source,
                        "    %s%s%s = %s%s%s_length > 0 ? (%s *)malloc(%s%s%s_length * sizeof(%s)) : NULL;\n",
                        out_expr, out_access, field_name,
                        out_expr, out_access, field_name, item_c_type,
                        out_expr, out_access, field_name, item_c_type);
                fprintf(source,
                        "    if (!%s%s%s && %s%s%s_length > 0) return EO_ALLOC_FAILED;\n",
                        out_expr, out_access, field_name, out_expr, out_access, field_name);
                fprintf(source, "    for (size_t i = 0; i < %s%s%s_length; ++i) {\n",
                        out_expr, out_access, field_name);
            }
            else
            {
                fprintf(source, "    size_t %s_capacity = 0;\n", field_name);
                fprintf(source, "    while (eo_reader_remaining(reader) > 0) {\n");
            }

            if (is_static)
            {
                if (is_enum)
                {
                    const char *enum_data_type = find_enum_data_type(enums, enums_count, type_name);
                    const char *enum_c_type = map_primitive_type(enum_data_type);
                    fprintf(source,
                            "        if ((result = %s(reader, (%s *)&%s%s%s[i])) != 0) return result;\n",
                            map_reader_fn(enum_data_type), enum_c_type, out_expr, out_access, field_name);
                }
                else if (is_struct)
                {
                    fprintf(source,
                            "        if ((result = %s_deserialize_fn((EoSerialize *)&%s%s%s[i], reader)) != 0) return result;\n",
                            type_name, out_expr, out_access, field_name);
                }
                else if (strcmp(type_name, "bool") == 0)
                {
                    fprintf(source,
                            "        int32_t raw = 0; if ((result = eo_reader_get_char(reader, &raw)) != 0) return result; %s%s%s[i] = raw == 1;\n",
                            out_expr, out_access, field_name);
                }
                else if (strcmp(type_name, "blob") == 0)
                {
                    fprintf(source,
                            "        size_t remaining = eo_reader_remaining(reader); if ((result = eo_reader_get_bytes(reader, remaining, &%s%s%s[i].data)) != 0) return result; %s%s%s[i].length = remaining;\n",
                            out_expr, out_access, field_name, out_expr, out_access, field_name);
                }
                else
                {
                    fprintf(source,
                            "        if ((result = %s(reader, &%s%s%s[i])) != 0) return result;\n",
                            map_reader_fn(type_name), out_expr, out_access, field_name);
                }
            }
            else if (array->length)
            {
                if (is_enum)
                {
                    const char *enum_data_type = find_enum_data_type(enums, enums_count, type_name);
                    const char *enum_c_type = map_primitive_type(enum_data_type);
                    fprintf(source,
                            "        if ((result = %s(reader, (%s *)&%s%s%s[i])) != 0) return result;\n",
                            map_reader_fn(enum_data_type), enum_c_type, out_expr, out_access, field_name);
                }
                else if (is_struct)
                {
                    fprintf(source,
                            "        if ((result = %s_deserialize_fn((EoSerialize *)&%s%s%s[i], reader)) != 0) return result;\n",
                            type_name, out_expr, out_access, field_name);
                }
                else if (strcmp(type_name, "bool") == 0)
                {
                    fprintf(source,
                            "        int32_t raw = 0; if ((result = eo_reader_get_char(reader, &raw)) != 0) return result; %s%s%s[i] = raw == 1;\n",
                            out_expr, out_access, field_name);
                }
                else if (strcmp(type_name, "blob") == 0)
                {
                    fprintf(source,
                            "        size_t remaining = eo_reader_remaining(reader); if ((result = eo_reader_get_bytes(reader, remaining, &%s%s%s[i].data)) != 0) return result; %s%s%s[i].length = remaining;\n",
                            out_expr, out_access, field_name, out_expr, out_access, field_name);
                }
                else
                {
                    fprintf(source,
                            "        if ((result = %s(reader, &%s%s%s[i])) != 0) return result;\n",
                            map_reader_fn(type_name), out_expr, out_access, field_name);
                }
            }
            else
            {
                fprintf(source,
                        "        if (%s%s%s_length >= %s_capacity) {\n",
                        out_expr, out_access, field_name, field_name);
                fprintf(source,
                        "            size_t new_capacity = %s_capacity < 8 ? 8 : %s_capacity * 2;\n",
                        field_name, field_name);
                fprintf(source,
                        "            %s *new_items = realloc(%s%s%s, new_capacity * sizeof(%s));\n",
                        item_c_type, out_expr, out_access, field_name, item_c_type);
                fprintf(source,
                        "            if (!new_items) return EO_ALLOC_FAILED;\n            %s%s%s = new_items;\n            %s_capacity = new_capacity;\n",
                        out_expr, out_access, field_name, field_name);
                fprintf(source, "        }\n");
                if (is_enum)
                {
                    const char *enum_data_type = find_enum_data_type(enums, enums_count, type_name);
                    const char *enum_c_type = map_primitive_type(enum_data_type);
                    fprintf(source,
                            "        if ((result = %s(reader, (%s *)&%s%s%s[%s%s%s_length++])) != 0) return result;\n",
                            map_reader_fn(enum_data_type), enum_c_type,
                            out_expr, out_access, field_name, out_expr, out_access, field_name);
                }
                else if (is_struct)
                {
                    fprintf(source,
                            "        if ((result = %s_deserialize_fn((EoSerialize *)&%s%s%s[%s%s%s_length++], reader)) != 0) return result;\n",
                            type_name, out_expr, out_access, field_name, out_expr,
                            out_access, field_name);
                }
                else if (strcmp(type_name, "bool") == 0)
                {
                    fprintf(source,
                            "        int32_t raw = 0; if ((result = eo_reader_get_char(reader, &raw)) != 0) return result; %s%s%s[%s%s%s_length++] = raw == 1;\n",
                            out_expr, out_access, field_name, out_expr, out_access, field_name);
                }
                else if (strcmp(type_name, "blob") == 0)
                {
                    fprintf(source,
                            "        size_t remaining = eo_reader_remaining(reader); if ((result = eo_reader_get_bytes(reader, remaining, &%s%s%s[%s%s%s_length].data)) != 0) return result; %s%s%s[%s%s%s_length++].length = remaining;\n",
                            out_expr, out_access, field_name, out_expr, out_access, field_name,
                            out_expr, out_access, field_name, out_expr, out_access, field_name);
                }
                else
                {
                    fprintf(source,
                            "        if ((result = %s(reader, &%s%s%s[%s%s%s_length++])) != 0) return result;\n",
                            map_reader_fn(type_name), out_expr, out_access, field_name,
                            out_expr, out_access, field_name);
                }
            }

            if (array->delimited)
            {
                if (array->length && !array->trailing_delimiter)
                {
                    fprintf(source, "        if (i + 1 < (size_t)%s) {\n", array->length);
                    fprintf(source,
                            "            if ((result = eo_reader_next_chunk(reader)) != 0) return result;\n        }\n");
                }
                else
                {
                    fprintf(source,
                            "        if ((result = eo_reader_next_chunk(reader)) != 0) return result;\n");
                }
            }

            fprintf(source, "    }\n");
            if (array->optional)
            {
                fprintf(source, "    }\n");
            }

            free(type_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_SWITCH)
        {
            SwitchDef *sw = &element->as.sw;
            char *field_name = to_snake_case(sw->field);
            char *field_type = find_field_data_type(elements, sw->field);
            int has_storage = switch_has_storage(sw);
            int has_default = 0;
            int suppress_enum_switch_warning =
                field_type && enum_exists(enums, enums_count, field_type) &&
                switch_has_numeric_case(sw);
            if (has_storage)
            {
                fprintf(source, "    %s%s%s_data = NULL;\n", out_expr, out_access, field_name);
            }
            if (suppress_enum_switch_warning)
            {
                write_switch_pragma_push(source);
            }
            fprintf(source, "    switch (%s%s%s) {\n", out_expr, out_access, field_name);
            for (size_t c = 0; c < sw->cases_count; ++c)
            {
                CaseDef *case_def = &sw->cases[c];
                if (!case_def->has_elements)
                {
                    continue;
                }
                if (case_def->is_default)
                {
                    has_default = 1;
                }
                const char *case_name = case_def->is_default ? "default" : case_def->value;
                char *case_const = to_upper_snake(case_name);
                char *case_field = to_snake_case(case_name);
                int case_has_storage = element_list_has_storage(&case_def->elements);

                if (case_def->is_default)
                {
                    fprintf(source, "    default: {\n");
                }
                else if (case_def->value && is_numeric_string(case_def->value))
                {
                    fprintf(source, "    case %s: {\n", case_def->value);
                }
                else if (field_type && enum_exists(enums, enums_count, field_type))
                {
                    char *enum_prefix = to_upper_snake(field_type);
                    fprintf(source, "    case %s_%s: {\n", enum_prefix, case_const);
                    free(enum_prefix);
                }
                else
                {
                    fprintf(source, "    case %s: {\n", case_def->value ? case_def->value : "0");
                }
                if (case_has_storage && has_storage)
                {
                    fprintf(source,
                            "        if (!%s%s%s_data) { %s%s%s_data = calloc(1, sizeof(*%s%s%s_data)); if (!%s%s%s_data) return EO_ALLOC_FAILED; }\n",
                            out_expr, out_access, field_name, out_expr, out_access, field_name,
                            out_expr, out_access, field_name, out_expr, out_access, field_name);
                    {
                        char case_out[512];
                        snprintf(case_out, sizeof(case_out), "%s%s%s_data->%s", out_expr,
                                 out_access, field_name, case_field);
                        write_deserialize_elements(source, struct_name, &case_def->elements, enums,
                                                   enums_count, structs, structs_count, case_out,
                                                   ".");
                    }
                }
                else if (!case_has_storage)
                {
                    write_deserialize_elements(source, struct_name, &case_def->elements, enums,
                                               enums_count, structs, structs_count, out_expr,
                                               out_access);
                }
                fprintf(source, "        break;\n    }\n");
                free(case_const);
                free(case_field);
            }
            if (!has_default)
            {
                fprintf(source, "    default: break;\n");
            }
            fprintf(source, "    }\n");
            if (suppress_enum_switch_warning)
            {
                write_switch_pragma_pop(source);
            }
            free(field_type);
            free(field_name);
        }
    }
}

static void write_free_elements(FILE *source, const char *struct_name,
                                ElementList *elements, EnumDef *enums,
                                size_t enums_count, StructDef *structs,
                                size_t structs_count, const char *value_expr,
                                const char *value_access)
{
    (void)struct_name;
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];
        if (element->kind == ELEMENT_FIELD)
        {
            FieldDef *field = &element->as.field;
            if (!field->name)
                continue;
            char *field_name = to_snake_case(field->name);
            char *type_name = normalize_type_name(field->data_type);
            if (strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0 ||
                strcmp(type_name, "blob") == 0)
            {
                fprintf(source, "    free(%s%s%s);\n", value_expr, value_access, field_name);
            }
            else if (!is_primitive_type(type_name) && !enum_exists(enums, enums_count, type_name))
            {
                if (field->optional)
                {
                    if (struct_has_heap(structs, structs_count, type_name, enums, enums_count))
                        fprintf(source, "    if (%s%s%s) { %s_free_fn((EoSerialize *)%s%s%s); free(%s%s%s); }\n",
                                value_expr, value_access, field_name,
                                type_name, value_expr, value_access, field_name,
                                value_expr, value_access, field_name);
                    else
                        fprintf(source, "    free(%s%s%s);\n", value_expr, value_access, field_name);
                }
                else
                {
                    if (struct_has_heap(structs, structs_count, type_name, enums, enums_count))
                        fprintf(source, "    %s_free_fn((EoSerialize *)&%s%s%s);\n", type_name, value_expr, value_access,
                                field_name);
                }
            }
            else if (field->optional)
            {
                fprintf(source, "    free(%s%s%s);\n", value_expr, value_access, field_name);
            }
            free(type_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_ARRAY)
        {
            ArrayDef *array = &element->as.array;
            if (!array->name)
                continue;
            char *field_name = to_snake_case(array->name);
            char *type_name = normalize_type_name(array->data_type);
            int is_static = is_static_length(array->length);
            int is_str = strcmp(type_name, "string") == 0 ||
                         strcmp(type_name, "encoded_string") == 0;
            int elem_has_heap = !is_primitive_type(type_name) &&
                                !enum_exists(enums, enums_count, type_name) &&
                                struct_has_heap(structs, structs_count, type_name, enums, enums_count);

            if (is_str)
            {
                const char *len_expr = is_static ? array->length : NULL;
                if (is_static)
                {
                    fprintf(source,
                            "    { size_t _fi; for (_fi = 0; _fi < %s; _fi++) free(%s%s%s[_fi]); }\n",
                            array->length, value_expr, value_access, field_name);
                }
                else
                {
                    (void)len_expr;
                    fprintf(source,
                            "    { size_t _fi; for (_fi = 0; _fi < %s%s%s_length; _fi++) free(%s%s%s[_fi]); }\n",
                            value_expr, value_access, field_name,
                            value_expr, value_access, field_name);
                    fprintf(source, "    free(%s%s%s);\n", value_expr, value_access, field_name);
                }
            }
            else if (!is_static)
            {
                if (elem_has_heap)
                {
                    fprintf(source,
                            "    { size_t _fi; for (_fi = 0; _fi < %s%s%s_length; _fi++) %s_free_fn((EoSerialize *)&%s%s%s[_fi]); }\n",
                            value_expr, value_access, field_name, type_name,
                            value_expr, value_access, field_name);
                }
                fprintf(source, "    free(%s%s%s);\n", value_expr, value_access, field_name);
            }
            else if (is_static && elem_has_heap)
            {
                fprintf(source,
                        "    { size_t _fi; for (_fi = 0; _fi < %s; _fi++) %s_free_fn((EoSerialize *)&%s%s%s[_fi]); }\n",
                        array->length, type_name, value_expr, value_access, field_name);
            }

            free(type_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_SWITCH)
        {
            SwitchDef *sw = &element->as.sw;
            char *field_name = to_snake_case(sw->field);
            char *field_type = find_field_data_type(elements, sw->field);
            int has_storage = switch_has_storage(sw);
            int suppress_enum_switch_warning =
                field_type && enum_exists(enums, enums_count, field_type) &&
                switch_has_numeric_case(sw);

            int any_case_has_heap = 0;
            for (size_t c = 0; c < sw->cases_count; ++c)
            {
                CaseDef *case_def = &sw->cases[c];
                if (case_def->has_elements &&
                    element_list_has_heap(&case_def->elements, enums, enums_count,
                                          structs, structs_count))
                {
                    any_case_has_heap = 1;
                    break;
                }
            }

            if (has_storage)
            {
                fprintf(source, "    if (%s%s%s_data) {\n", value_expr, value_access, field_name);
                if (any_case_has_heap)
                {
                    int has_default = 0;
                    if (suppress_enum_switch_warning)
                    {
                        write_switch_pragma_push(source);
                    }
                    fprintf(source, "        switch (%s%s%s) {\n", value_expr, value_access, field_name);
                    for (size_t c = 0; c < sw->cases_count; ++c)
                    {
                        CaseDef *case_def = &sw->cases[c];
                        if (!case_def->has_elements)
                            continue;
                        int case_has_storage = element_list_has_storage(&case_def->elements);
                        int case_has_heap = element_list_has_heap(&case_def->elements, enums,
                                                                  enums_count, structs, structs_count);
                        if (!case_has_heap)
                            continue;
                        if (case_def->is_default)
                            has_default = 1;
                        const char *case_name = case_def->is_default ? "default" : case_def->value;
                        char *case_const = to_upper_snake(case_name);
                        char *case_field = to_snake_case(case_name);
                        if (case_def->is_default)
                        {
                            fprintf(source, "        default: {\n");
                        }
                        else if (case_def->value && is_numeric_string(case_def->value))
                        {
                            fprintf(source, "        case %s: {\n", case_def->value);
                        }
                        else if (field_type && enum_exists(enums, enums_count, field_type))
                        {
                            char *enum_prefix = to_upper_snake(field_type);
                            fprintf(source, "        case %s_%s: {\n", enum_prefix, case_const);
                            free(enum_prefix);
                        }
                        else
                        {
                            fprintf(source, "        case %s: {\n",
                                    case_def->value ? case_def->value : "0");
                        }
                        if (case_has_storage && case_has_heap)
                        {
                            char case_value[512];
                            snprintf(case_value, sizeof(case_value), "%s%s%s_data->%s",
                                     value_expr, value_access, field_name, case_field);
                            write_free_elements(source, struct_name, &case_def->elements, enums,
                                                enums_count, structs, structs_count, case_value, ".");
                        }
                        fprintf(source, "            break;\n        }\n");
                        free(case_const);
                        free(case_field);
                    }
                    if (!has_default)
                        fprintf(source, "        default: break;\n");
                    fprintf(source, "        }\n");
                    if (suppress_enum_switch_warning)
                    {
                        write_switch_pragma_pop(source);
                    }
                }
                fprintf(source, "        free(%s%s%s_data);\n", value_expr, value_access, field_name);
                fprintf(source, "    }\n");
            }
            free(field_name);
            free(field_type);
        }
        else if (element->kind == ELEMENT_CHUNKED)
        {
            write_free_elements(source, struct_name, &element->as.chunked, enums, enums_count,
                                structs, structs_count, value_expr, value_access);
        }
    }
}

static int compute_elements_precomputed_size(ElementList *elements, EnumDef *enums,
                                             size_t enums_count, StructDef *structs,
                                             size_t structs_count)
{
    int total = 0;
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];

        if (element->kind == ELEMENT_CHUNKED)
        {
            total += compute_elements_precomputed_size(&element->as.chunked, enums, enums_count,
                                                       structs, structs_count);
            continue;
        }

        if (element->kind == ELEMENT_BREAK)
        {
            total += 1;
            continue;
        }

        if (element->kind == ELEMENT_COMMENT)
            continue;

        if (element->kind == ELEMENT_DUMMY)
        {
            DummyDef *dummy = &element->as.dummy;
            char *type_name = normalize_type_name(dummy->data_type);
            if (strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0)
                total += dummy->value ? (int)strlen(dummy->value) : 0;
            else
                total += primitive_byte_size(type_name);
            free(type_name);
            continue;
        }

        if (element->kind == ELEMENT_LENGTH)
        {
            LengthDef *length = &element->as.length;
            if (!length->optional)
            {
                int sz = primitive_byte_size(length->data_type);
                total += sz > 0 ? sz : 1;
            }
            continue;
        }

        if (element->kind == ELEMENT_FIELD)
        {
            FieldDef *field = &element->as.field;
            if (field->optional)
                continue;

            char *type_name = normalize_type_name(field->data_type);
            int is_enum = enum_exists(enums, enums_count, type_name);

            if (strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0)
            {
                if (field->length && isdigit((unsigned char)field->length[0]))
                    total += atoi(field->length);
                else if (!field->name)
                    total += field->value ? (int)strlen(field->value) : 0;
            }
            else if (strcmp(type_name, "blob") == 0)
            {
                /* dynamic — 0 contribution */
            }
            else if (is_enum)
            {
                char *type_override = get_type_override(field->data_type);
                const char *edt = find_enum_data_type(enums, enums_count, type_name);
                const char *eff = type_override ? type_override : edt;
                int sz = primitive_byte_size(eff);
                if (sz > 0)
                    total += sz;
                free(type_override);
            }
            else if (struct_exists(structs, structs_count, type_name))
            {
                /* struct fields emitted at runtime via TypeName_size() */
            }
            else
            {
                int sz = primitive_byte_size(type_name);
                if (sz > 0)
                    total += sz;
            }
            free(type_name);
            continue;
        }

        if (element->kind == ELEMENT_ARRAY)
        {
            ArrayDef *array = &element->as.array;
            if (array->optional || !is_static_length(array->length))
                continue;

            int count = atoi(array->length);
            char *type_name = normalize_type_name(array->data_type);
            int is_enum = enum_exists(enums, enums_count, type_name);

            if (!struct_exists(structs, structs_count, type_name) &&
                strcmp(type_name, "string") != 0 &&
                strcmp(type_name, "encoded_string") != 0 &&
                strcmp(type_name, "blob") != 0)
            {
                int elem_sz = is_enum
                                  ? primitive_byte_size(find_enum_data_type(enums, enums_count, type_name))
                                  : primitive_byte_size(type_name);
                if (elem_sz > 0)
                {
                    int arr_total = count * elem_sz;
                    if (array->delimited)
                        arr_total += array->trailing_delimiter
                                         ? count
                                         : (count > 1 ? count - 1 : 0);
                    total += arr_total;
                }
            }
            free(type_name);
            continue;
        }

        /* ELEMENT_SWITCH: always dynamic */
    }
    return total;
}

static void write_size_elements(FILE *source, const char *struct_name,
                                ElementList *elements, EnumDef *enums,
                                size_t enums_count, StructDef *structs,
                                size_t structs_count, const char *value_expr,
                                const char *value_access)
{
    (void)struct_name;
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];

        if (element->kind == ELEMENT_CHUNKED)
        {
            write_size_elements(source, struct_name, &element->as.chunked, enums, enums_count,
                                structs, structs_count, value_expr, value_access);
            continue;
        }

        if (element->kind == ELEMENT_BREAK ||
            element->kind == ELEMENT_COMMENT ||
            element->kind == ELEMENT_DUMMY)
            continue;

        if (element->kind == ELEMENT_LENGTH)
        {
            LengthDef *length = &element->as.length;
            if (!length->optional)
                continue;
            char *field_name = to_snake_case(length->name);
            int sz = primitive_byte_size(length->data_type);
            if (sz <= 0)
                sz = 1;
            fprintf(source, "    if (%s%s%s) {\n", value_expr, value_access, field_name);
            fprintf(source, "        total += %d;\n", sz);
            fprintf(source, "    }\n");
            free(field_name);
        }
        else if (element->kind == ELEMENT_FIELD)
        {
            FieldDef *field = &element->as.field;
            char *type_name = normalize_type_name(field->data_type);
            int is_enum = enum_exists(enums, enums_count, type_name);
            int is_struct = struct_exists(structs, structs_count, type_name);
            char *field_name = field->name ? to_snake_case(field->name) : NULL;
            int is_fixed_string =
                (strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0) &&
                field->length && isdigit((unsigned char)field->length[0]);
            int is_unnamed_literal =
                !field->name &&
                (strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0);

            if (!field->optional &&
                ((!is_struct && !is_enum &&
                  strcmp(type_name, "blob") != 0 &&
                  strcmp(type_name, "string") != 0 &&
                  strcmp(type_name, "encoded_string") != 0) ||
                 is_enum ||
                 is_fixed_string ||
                 is_unnamed_literal))
            {
                free(field_name);
                free(type_name);
                continue;
            }

            if (field->optional && field_name)
                fprintf(source, "    if (%s%s%s) {\n", value_expr, value_access, field_name);

            if (is_struct)
            {
                if (field_name)
                {
                    if (field->optional)
                        fprintf(source, "    total += %s_get_size_fn((const EoSerialize *)%s%s%s);\n",
                                type_name, value_expr, value_access, field_name);
                    else
                        fprintf(source, "    total += %s_get_size_fn((const EoSerialize *)&%s%s%s);\n",
                                type_name, value_expr, value_access, field_name);
                }
            }
            else if (strcmp(type_name, "blob") == 0 && field_name)
            {
                fprintf(source, "    total += %s%s%s_length;\n",
                        value_expr, value_access, field_name);
            }
            else if (strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0)
            {
                if (field_name)
                    fprintf(source, "    total += %s%s%s ? strlen(%s%s%s) : 0;\n",
                            value_expr, value_access, field_name,
                            value_expr, value_access, field_name);
            }
            else if (is_enum)
            {
                char *type_override = get_type_override(field->data_type);
                const char *edt = find_enum_data_type(enums, enums_count, type_name);
                const char *eff = type_override ? type_override : edt;
                int sz = primitive_byte_size(eff);
                if (sz > 0)
                    fprintf(source, "    total += %d;\n", sz);
                free(type_override);
            }
            else
            {
                int sz = primitive_byte_size(type_name);
                if (sz > 0)
                    fprintf(source, "    total += %d;\n", sz);
            }

            if (field->optional && field_name)
                fprintf(source, "    }\n");

            free(field_name);
            free(type_name);
        }
        else if (element->kind == ELEMENT_ARRAY)
        {
            ArrayDef *array = &element->as.array;
            char *field_name = to_snake_case(array->name);
            char *type_name = normalize_type_name(array->data_type);
            int is_enum = enum_exists(enums, enums_count, type_name);
            int is_struct = struct_exists(structs, structs_count, type_name);
            int is_static_count = is_static_length(array->length);
            int is_dynamic_elem = is_struct ||
                                  strcmp(type_name, "string") == 0 ||
                                  strcmp(type_name, "encoded_string") == 0 ||
                                  strcmp(type_name, "blob") == 0;

            if (!array->optional && is_static_count && !is_dynamic_elem)
            {
                free(type_name);
                free(field_name);
                continue;
            }

            int elem_size = 0;
            if (!is_dynamic_elem)
            {
                if (is_enum)
                {
                    const char *edt = find_enum_data_type(enums, enums_count, type_name);
                    elem_size = primitive_byte_size(edt);
                }
                else
                {
                    elem_size = primitive_byte_size(type_name);
                }
            }

            if (array->optional)
                fprintf(source, "    if (%s%s%s) {\n", value_expr, value_access, field_name);

            if (!is_dynamic_elem && elem_size > 0)
            {
                fprintf(source, "    total += %s%s%s_length * %d;\n",
                        value_expr, value_access, field_name, elem_size);
                if (array->delimited)
                {
                    if (array->trailing_delimiter)
                        fprintf(source, "    total += %s%s%s_length;\n",
                                value_expr, value_access, field_name);
                    else
                        fprintf(source,
                                "    if (%s%s%s_length > 0) total += %s%s%s_length - 1;\n",
                                value_expr, value_access, field_name,
                                value_expr, value_access, field_name);
                }
            }
            else
            {
                if (is_static_count)
                    fprintf(source,
                            "    { size_t _i; for (_i = 0; _i < (size_t)%s; ++_i) {\n",
                            array->length);
                else
                    fprintf(source,
                            "    { size_t _i; for (_i = 0; _i < %s%s%s_length; ++_i) {\n",
                            value_expr, value_access, field_name);

                if (array->delimited && !array->trailing_delimiter)
                    fprintf(source, "        if (_i > 0) total += 1;\n");

                if (is_struct)
                {
                    fprintf(source, "        total += %s_get_size_fn((const EoSerialize *)&%s%s%s[_i]);\n",
                            type_name, value_expr, value_access, field_name);
                }
                else if (strcmp(type_name, "string") == 0 ||
                         strcmp(type_name, "encoded_string") == 0)
                {
                    fprintf(source, "        total += %s%s%s[_i] ? strlen(%s%s%s[_i]) : 0;\n",
                            value_expr, value_access, field_name,
                            value_expr, value_access, field_name);
                }
                else if (strcmp(type_name, "blob") == 0)
                {
                    fprintf(source, "        total += %s%s%s[_i].length;\n",
                            value_expr, value_access, field_name);
                }

                if (array->delimited && array->trailing_delimiter)
                    fprintf(source, "        total += 1;\n");

                fprintf(source, "    } }\n");
            }

            if (array->optional)
                fprintf(source, "    }\n");

            free(type_name);
            free(field_name);
        }
        else if (element->kind == ELEMENT_SWITCH)
        {
            SwitchDef *sw = &element->as.sw;
            char *field_name = to_snake_case(sw->field);
            char *field_type = find_field_data_type(elements, sw->field);
            int has_storage = switch_has_storage(sw);
            int has_default = 0;
            int suppress_enum_switch_warning =
                field_type && enum_exists(enums, enums_count, field_type) &&
                switch_has_numeric_case(sw);

            if (suppress_enum_switch_warning)
            {
                write_switch_pragma_push(source);
            }
            fprintf(source, "    switch (%s%s%s) {\n", value_expr, value_access, field_name);

            for (size_t c = 0; c < sw->cases_count; ++c)
            {
                CaseDef *case_def = &sw->cases[c];
                if (!case_def->has_elements)
                    continue;
                int case_has_storage = element_list_has_storage(&case_def->elements);
                if (case_def->is_default)
                    has_default = 1;
                const char *case_name = case_def->is_default ? "default" : case_def->value;
                char *case_const = to_upper_snake(case_name);
                char *case_field = to_snake_case(case_name);

                if (case_def->is_default)
                    fprintf(source, "    default: {\n");
                else if (case_def->value && is_numeric_string(case_def->value))
                    fprintf(source, "    case %s: {\n", case_def->value);
                else if (field_type && enum_exists(enums, enums_count, field_type))
                {
                    char *enum_prefix = to_upper_snake(field_type);
                    fprintf(source, "    case %s_%s: {\n", enum_prefix, case_const);
                    free(enum_prefix);
                }
                else
                    fprintf(source, "    case %s: {\n", case_def->value ? case_def->value : "0");

                int case_precomputed = compute_elements_precomputed_size(
                    &case_def->elements, enums, enums_count, structs, structs_count);
                if (case_precomputed > 0)
                    fprintf(source, "        total += %d;\n", case_precomputed);

                if (case_has_storage && has_storage)
                {
                    char case_value[512];
                    snprintf(case_value, sizeof(case_value), "%s%s%s_data->%s",
                             value_expr, value_access, field_name, case_field);
                    fprintf(source, "        if (%s%s%s_data) {\n",
                            value_expr, value_access, field_name);
                    write_size_elements(source, struct_name, &case_def->elements, enums,
                                        enums_count, structs, structs_count, case_value, ".");
                    fprintf(source, "        }\n");
                }
                else
                {
                    write_size_elements(source, struct_name, &case_def->elements, enums,
                                        enums_count, structs, structs_count,
                                        value_expr, value_access);
                }
                fprintf(source, "        break;\n    }\n");
                free(case_const);
                free(case_field);
            }
            if (!has_default)
                fprintf(source, "    default: break;\n");
            fprintf(source, "    }\n");

            if (suppress_enum_switch_warning)
            {
                write_switch_pragma_pop(source);
            }
            free(field_name);
            free(field_type);
        }
    }
}

static void write_struct_def(FILE *header, FILE *source, const char *name, ElementList *elements,
                             EnumDef *enums, size_t enums_count, StructDef *structs,
                             size_t structs_count, const char *comment,
                             const char *packet_family, const char *packet_action)
{
    int has_storage = element_list_has_storage(elements);
    int has_heap = has_storage && element_list_has_heap(elements, enums, enums_count,
                                                        structs, structs_count);
    int is_packet = (packet_family != NULL);

    /* ---- Header ---- */
    if (comment)
        write_doc_comment(header, comment, "");
    fprintf(header, "typedef struct %s {\n", name);
    if (is_packet)
        fprintf(header, "    EoPacket _packet;\n");
    else
        fprintf(header, "    EoSerialize _serialize;\n");
    if (has_storage)
        write_struct_fields(header, name, elements, enums, enums_count, structs, structs_count);
    fprintf(header, "} %s;\n\n", name);
    fprintf(header,
            "/** @brief Creates a new ::%s with the serialization vtable initialized.\n"
            " * @return A zero-initialized ::%s with its vtable pointer set.\n"
            " */\n",
            name, name);
    /* Declare vtables as extern so the static inline init can reference them */
    fprintf(header, "extern const EoSerializeVTable %s_vtable;\n", name);
    if (is_packet)
        fprintf(header, "extern const EoPacketVTable %s_packet_vtable;\n", name);
    /* static inline _init avoids 400 exported symbols while keeping zero overhead */
    fprintf(header, "static inline %s %s_init(void) {\n", name, name);
    fprintf(header, "    %s result = {0};\n", name);
    if (is_packet)
    {
        fprintf(header, "    result._packet.base.vtable = &%s_vtable;\n", name);
        fprintf(header, "    result._packet.vtable = &%s_packet_vtable;\n", name);
    }
    else
    {
        fprintf(header, "    result._serialize.vtable = &%s_vtable;\n", name);
    }
    fprintf(header, "    return result;\n}\n\n");

    /* ---- Source: forward declarations ---- */
    fprintf(source,
            "static EoResult %s_serialize_fn(const EoSerialize *self, EoWriter *writer);\n",
            name);
    fprintf(source,
            "static EoResult %s_deserialize_fn(EoSerialize *self, EoReader *reader);\n",
            name);
    fprintf(source, "static size_t %s_get_size_fn(const EoSerialize *self);\n", name);
    if (has_heap)
        fprintf(source, "static void %s_free_fn(EoSerialize *self);\n", name);
    if (is_packet)
    {
        fprintf(source, "static uint8_t %s_get_family_fn(const EoPacket *self);\n", name);
        fprintf(source, "static uint8_t %s_get_action_fn(const EoPacket *self);\n", name);
    }
    fprintf(source, "\n");

    /* ---- Source: vtable definitions ---- */
    fprintf(source, "const EoSerializeVTable %s_vtable = {\n", name);
    fprintf(source, "    %s_deserialize_fn,\n", name);
    fprintf(source, "    %s_serialize_fn,\n", name);
    fprintf(source, "    %s_get_size_fn,\n", name);
    if (has_heap)
        fprintf(source, "    %s_free_fn,\n", name);
    else
        fprintf(source, "    NULL,\n");
    fprintf(source, "};\n");

    if (is_packet)
    {
        fprintf(source, "const EoPacketVTable %s_packet_vtable = {\n", name);
        fprintf(source, "    %s_get_family_fn,\n", name);
        fprintf(source, "    %s_get_action_fn,\n", name);
        fprintf(source, "};\n");
    }
    fprintf(source, "\n");

    /* ---- Source: serialize ---- */
    fprintf(source,
            "static EoResult %s_serialize_fn(const EoSerialize *self, EoWriter *writer) {\n",
            name);
    if (has_storage)
        fprintf(source, "    const %s *value = (const %s *)self;\n", name, name);
    else
        fprintf(source, "    (void)self;\n");
    fprintf(source, "    EoResult result = EO_SUCCESS;\n");
    fprintf(source,
            "    bool previous_sanitization = eo_writer_get_string_sanitization_mode(writer);\n");
    write_serialize_elements(source, name, elements, enums, enums_count, structs, structs_count,
                             has_storage ? "value" : "", has_storage ? "->" : "");
    fprintf(source,
            "    eo_writer_set_string_sanitization_mode(writer, previous_sanitization);\n");
    fprintf(source, "    return result;\n}\n\n");

    /* ---- Source: deserialize ---- */
    fprintf(source,
            "static EoResult %s_deserialize_fn(EoSerialize *self, EoReader *reader) {\n",
            name);
    fprintf(source, "    %s *out = (%s *)self;\n", name, name);
    fprintf(source, "    EoResult result = EO_SUCCESS;\n");
    fprintf(source,
            "    bool previous_chunked = eo_reader_get_chunked_reading_mode(reader);\n");
    fprintf(source, "    memset(out, 0, sizeof(*out));\n");
    if (is_packet)
    {
        fprintf(source, "    out->_packet.base.vtable = &%s_vtable;\n", name);
        fprintf(source, "    out->_packet.vtable = &%s_packet_vtable;\n", name);
    }
    else
    {
        fprintf(source, "    out->_serialize.vtable = &%s_vtable;\n", name);
    }
    write_deserialize_elements(source, name, elements, enums, enums_count, structs, structs_count,
                               has_storage ? "out" : "", has_storage ? "->" : "");
    fprintf(source, "    eo_reader_set_chunked_reading_mode(reader, previous_chunked);\n");
    fprintf(source, "    return result;\n}\n\n");

    /* ---- Source: get_size ---- */
    fprintf(source, "static size_t %s_get_size_fn(const EoSerialize *self) {\n", name);
    if (has_storage)
    {
        fprintf(source, "    const %s *value = (const %s *)self;\n", name, name);
        fprintf(source, "    if (!value) return 0;\n");
    }
    else
    {
        fprintf(source, "    (void)self;\n");
    }
    fprintf(source, "    size_t total = %d;\n",
            compute_elements_precomputed_size(elements, enums, enums_count, structs, structs_count));
    write_size_elements(source, name, elements, enums, enums_count, structs, structs_count,
                        has_storage ? "value" : "", has_storage ? "->" : "");
    fprintf(source, "    return total;\n}\n\n");

    /* ---- Source: free (only when has_heap) ---- */
    if (has_heap)
    {
        fprintf(source, "static void %s_free_fn(EoSerialize *self) {\n", name);
        fprintf(source, "    %s *value = (%s *)self;\n", name, name);
        fprintf(source, "    if (!value) return;\n");
        write_free_elements(source, name, elements, enums, enums_count, structs, structs_count,
                            "value", "->");
        fprintf(source, "}\n\n");
    }

    /* ---- Source: get_family / get_action (packets only) ---- */
    if (is_packet)
    {
        char *family_upper = to_upper_snake(packet_family);
        char *action_upper = to_upper_snake(packet_action);
        fprintf(source, "static uint8_t %s_get_family_fn(const EoPacket *self) {\n", name);
        fprintf(source, "    (void)self;\n");
        fprintf(source, "    return PACKET_FAMILY_%s;\n", family_upper);
        fprintf(source, "}\n\n");
        fprintf(source, "static uint8_t %s_get_action_fn(const EoPacket *self) {\n", name);
        fprintf(source, "    (void)self;\n");
        fprintf(source, "    return PACKET_ACTION_%s;\n", action_upper);
        fprintf(source, "}\n\n");
        free(family_upper);
        free(action_upper);
    }
}

void write_protocol_files(ProtocolDef *protocols, size_t protocol_count)
{
    if (ensure_dir("include") != 0 || ensure_dir("src") != 0)
    {
        fprintf(stderr, "Failed to create output directories\n");
        return;
    }

    FILE *header = fopen("include/eolib/protocol.h", "w");
    FILE *source = fopen("src/protocol.c", "w");
    if (!header || !source)
    {
        fprintf(stderr, "Failed to open output files\n");
        if (header)
        {
            fclose(header);
        }
        if (source)
        {
            fclose(source);
        }
        return;
    }

    fprintf(header, "%s", CODEGEN_WARNING);
    fprintf(source, "%s", CODEGEN_WARNING);

    fprintf(header, "#ifndef EOLIB_PROTOCOL_H\n#define EOLIB_PROTOCOL_H\n\n");
    fprintf(header, "#include <stdbool.h>\n#include <stddef.h>\n#include <stdint.h>\n\n");
    fprintf(header, "#include \"data.h\"\n\n");

    fprintf(source, "#include \"eolib/protocol.h\"\n\n");
    fprintf(source, "#include <string.h>\n#include <stdlib.h>\n\n");

    size_t all_enums_count = 0;
    EnumDef *all_enums = NULL;
    size_t all_structs_count = 0;
    StructDef *all_structs = NULL;
    collect_all_enums_structs(protocols, protocol_count, &all_enums, &all_enums_count, &all_structs, &all_structs_count);

    StringList written_enums = {0};
    StringList written_structs = {0};

    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *protocol = &protocols[p];
        for (size_t i = 0; i < protocol->enums_count; ++i)
        {
            if (string_list_contains(&written_enums, protocol->enums[i].name))
            {
                continue;
            }
            write_enum_def(header, source, &protocol->enums[i]);
            string_list_push(&written_enums, protocol->enums[i].name);
        }
    }

    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *protocol = &protocols[p];
        for (size_t i = 0; i < protocol->structs_count; ++i)
        {
            if (string_list_contains(&written_structs, protocol->structs[i].name))
            {
                continue;
            }
            write_struct_def(header, source, protocol->structs[i].name,
                             &protocol->structs[i].elements, all_enums,
                             all_enums_count, all_structs, all_structs_count,
                             protocol->structs[i].comment, NULL, NULL);
            string_list_push(&written_structs, protocol->structs[i].name);
        }
    }

    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *protocol = &protocols[p];
        for (size_t i = 0; i < protocol->packets_count; ++i)
        {
            const char *source_suffix = "Client";
            if (strstr(protocol->path, "/server/") ||
                strstr(protocol->path, "/server/protocol.xml"))
            {
                source_suffix = "Server";
            }
            else if (strstr(protocol->path, "/client/"))
            {
                source_suffix = "Client";
            }

            char name_buffer[256];
            snprintf(name_buffer, sizeof(name_buffer), "%s%s%sPacket",
                     protocol->packets[i].family, protocol->packets[i].action, source_suffix);
            write_struct_def(header, source, name_buffer, &protocol->packets[i].elements,
                             all_enums, all_enums_count, all_structs, all_structs_count,
                             protocol->packets[i].comment,
                             protocol->packets[i].family, protocol->packets[i].action);
        }
    }

    fprintf(header, "#endif // EOLIB_PROTOCOL_H\n");
    fclose(header);
    fclose(source);

    string_list_free(&written_enums);
    string_list_free(&written_structs);
    free(all_enums);
    free(all_structs);
}
