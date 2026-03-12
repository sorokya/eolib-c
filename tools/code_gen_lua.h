#ifndef CODE_GEN_LUA_H
#define CODE_GEN_LUA_H
#include "code_gen_common.h"
#include <stddef.h>

void write_lua_files(ProtocolDef *protocols, size_t protocol_count);
void write_lua_annotations(ProtocolDef *protocols, size_t protocol_count);

#endif /* CODE_GEN_LUA_H */
