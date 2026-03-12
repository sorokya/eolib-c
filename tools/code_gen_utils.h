#ifndef CODE_GEN_UTILS_H
#define CODE_GEN_UTILS_H
#include "code_gen_common.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int eolib_mkdir(const char *path);
void *xmalloc(size_t size);
char *xstrdup(const char *value);
void string_list_push(StringList *list, const char *value);
int string_list_contains(const StringList *list, const char *value);
void string_list_free(StringList *list);
void string_list_push_unique(StringList *paths, const char *value);
void element_list_push(ElementList *list, StructElement element);
void enum_values_push(EnumDef *def, EnumValue value);
void protocol_enums_push(ProtocolDef *protocol, EnumDef def);
void protocol_structs_push(ProtocolDef *protocol, StructDef def);
void protocol_packets_push(ProtocolDef *protocol, PacketDef def);
void switch_cases_push(SwitchDef *sw, CaseDef def);
void enum_list_push(EnumDef **list, size_t *count, size_t *capacity, const EnumDef *def);
void struct_list_push(StructDef **list, size_t *count, size_t *capacity, const StructDef *def);
char *sanitize_identifier(const char *value);
char *to_snake_case(const char *value);
char *to_upper_snake(const char *value);
char *to_pascal_case(const char *snake);
char *normalize_path(const char *path);
char *relative_protocol_dir(const char *path);
int path_is_dir(const char *path);
int path_exists(const char *path);
void list_subdirs(const char *base_path, StringList *out);
void append_glob_results(StringList *paths, const char *pattern);
int ensure_dir(const char *path);
void list_json_files_in_dir(const char *dir_path, StringList *out);
void write_doc_comment(FILE *out, const char *comment, const char *indent);
void write_lua_doc_comment(FILE *f, const char *comment);
void write_escaped_c_string(FILE *f, const char *s);
void write_byte_array_literal(FILE *f, const uint8_t *data, size_t len);
void write_switch_pragma_push(FILE *f);
void write_switch_pragma_pop(FILE *f);
void collect_all_enums_structs(ProtocolDef *protocols, size_t protocol_count,
    EnumDef **all_enums, size_t *all_enums_count,
    StructDef **all_structs, size_t *all_structs_count);

#endif /* CODE_GEN_UTILS_H */
