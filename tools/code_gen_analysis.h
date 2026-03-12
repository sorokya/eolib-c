#ifndef CODE_GEN_ANALYSIS_H
#define CODE_GEN_ANALYSIS_H
#include "code_gen_common.h"
#include <stddef.h>

int is_static_length(const char *value);
int is_numeric_string(const char *value);
int is_primitive_type(const char *data_type);
const char *map_primitive_type(const char *data_type);
const char *map_writer_fn(const char *data_type);
const char *map_reader_fn(const char *data_type);
int primitive_byte_size(const char *data_type);
char *normalize_type_name(const char *data_type);
char *get_type_override(const char *data_type);
int enum_exists(EnumDef *enums, size_t enums_count, const char *name);
int struct_exists(StructDef *structs, size_t structs_count, const char *name);
const char *find_enum_data_type(EnumDef *enums, size_t enums_count, const char *name);
int struct_has_storage(StructDef *structs, size_t structs_count, const char *name);
int element_list_has_storage(ElementList *elements);
int switch_has_storage(SwitchDef *sw);
int switch_has_numeric_case(SwitchDef *sw);
int struct_has_heap(StructDef *structs, size_t structs_count, const char *name,
                    EnumDef *enums, size_t enums_count);
int element_list_has_heap(ElementList *elements, EnumDef *enums, size_t enums_count,
                          StructDef *structs, size_t structs_count);
char *find_field_data_type(ElementList *elements, const char *name);
LengthTarget find_length_target(ElementList *elements, const char *length_name);

#endif /* CODE_GEN_ANALYSIS_H */
