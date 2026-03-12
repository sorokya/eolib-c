#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "code_gen_utils.h"
#include "code_gen_xml.h"
#include <stdlib.h>
#include <stdio.h>
#include "code_gen_protocol.h"
#include "code_gen_tests.h"
#include "code_gen_lua.h"

int main(void)
{
    StringList protocol_paths = {0};
    append_glob_results(&protocol_paths, "eo-protocol/xml/protocol.xml");
    append_glob_results(&protocol_paths, "eo-protocol/xml/*/protocol.xml");
    append_glob_results(&protocol_paths, "eo-protocol/xml/*/*/protocol.xml");

    if (protocol_paths.count == 0)
    {
        fprintf(stderr, "No protocol files found.\n");
        return 1;
    }

    ProtocolDef *protocols = NULL;
    size_t protocol_count = 0;
    size_t protocol_capacity = 0;

    for (size_t i = 0; i < protocol_paths.count; ++i)
    {
        if (protocol_count >= protocol_capacity)
        {
            protocol_capacity = ARRAY_GROW_CAPACITY(protocol_capacity);
            protocols = realloc(protocols, protocol_capacity * sizeof(ProtocolDef));
            if (!protocols)
            {
                fprintf(stderr, "Out of memory\n");
                return 1;
            }
        }
        protocols[protocol_count++] = parse_protocol_file(protocol_paths.items[i]);
    }

    write_protocol_files(protocols, protocol_count);
    write_packet_tests(protocols, protocol_count);
    write_lua_files(protocols, protocol_count);
    write_lua_annotations(protocols, protocol_count);

    return 0;
}
