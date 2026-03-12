#include "code_gen_wasm.h"
#include "code_gen_utils.h"
#include "code_gen_analysis.h"
#include <json-c/json.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─── Packet storage map (same logic as code_gen_tests.c) ──────────────────── */

typedef struct
{
    char **names;
    int *has_storage;
    int *has_heap;
    size_t count;
    size_t capacity;
} PacketStorageMap;

static void psm_push(PacketStorageMap *m, const char *name, int hs, int hh)
{
    if (m->count >= m->capacity)
    {
        m->capacity = ARRAY_GROW_CAPACITY(m->capacity);
        m->names = realloc(m->names, m->capacity * sizeof(char *));
        m->has_storage = realloc(m->has_storage, m->capacity * sizeof(int));
        m->has_heap = realloc(m->has_heap, m->capacity * sizeof(int));
    }
    m->names[m->count] = xstrdup(name);
    m->has_storage[m->count] = hs;
    m->has_heap[m->count] = hh;
    m->count++;
}

static int psm_storage(const PacketStorageMap *m, const char *name)
{
    for (size_t i = 0; i < m->count; ++i)
        if (strcmp(m->names[i], name) == 0)
            return m->has_storage[i];
    return 0;
}

static int psm_heap(const PacketStorageMap *m, const char *name)
{
    for (size_t i = 0; i < m->count; ++i)
        if (strcmp(m->names[i], name) == 0)
            return m->has_heap[i];
    return 0;
}

static PacketStorageMap build_psm(ProtocolDef *protocols, size_t protocol_count)
{
    EnumDef *all_enums = NULL;
    size_t all_enums_count = 0;
    StructDef *all_structs = NULL;
    size_t all_structs_count = 0;
    collect_all_enums_structs(protocols, protocol_count, &all_enums, &all_enums_count,
                              &all_structs, &all_structs_count);

    PacketStorageMap map = {0};
    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *proto = &protocols[p];
        const char *suffix = (strstr(proto->path, "/server/") ||
                              strstr(proto->path, "/server/protocol.xml"))
                                 ? "Server"
                                 : "Client";
        for (size_t i = 0; i < proto->packets_count; ++i)
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s%s%sPacket",
                     proto->packets[i].family, proto->packets[i].action, suffix);
            int hs = element_list_has_storage(&proto->packets[i].elements);
            int hh = hs && element_list_has_heap(&proto->packets[i].elements,
                                                  all_enums, all_enums_count,
                                                  all_structs, all_structs_count);
            psm_push(&map, buf, hs, hh);
        }
    }
    free(all_enums);
    free(all_structs);
    return map;
}

static void psm_free(PacketStorageMap *m)
{
    for (size_t i = 0; i < m->count; ++i)
        free(m->names[i]);
    free(m->names);
    free(m->has_storage);
    free(m->has_heap);
}

/* ─── Enum value lookup ─────────────────────────────────────────────────────── */

static int find_enum_value(ProtocolDef *protocols, size_t protocol_count,
                           const char *enum_name, const char *value_name)
{
    for (size_t p = 0; p < protocol_count; ++p)
    {
        for (size_t e = 0; e < protocols[p].enums_count; ++e)
        {
            EnumDef *en = &protocols[p].enums[e];
            if (strcmp(en->name, enum_name) != 0)
                continue;
            for (size_t v = 0; v < en->values_count; ++v)
                if (strcmp(en->values[v].name, value_name) == 0)
                    return en->values[v].value;
        }
    }
    return -1;
}

/* Returns the comment string for (family, action, is_server) or NULL */
static const char *find_packet_comment(ProtocolDef *protocols, size_t protocol_count,
                                       const char *family, const char *action, int is_server)
{
    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *proto = &protocols[p];
        int proto_server = (strstr(proto->path, "/server/") ||
                            strstr(proto->path, "/server/protocol.xml"))
                               ? 1
                               : 0;
        if (proto_server != is_server)
            continue;
        for (size_t i = 0; i < proto->packets_count; ++i)
        {
            if (strcmp(proto->packets[i].family, family) == 0 &&
                strcmp(proto->packets[i].action, action) == 0)
                return proto->packets[i].comment;
        }
    }
    return NULL;
}

/* ─── Write wasm/wasm_protocol_dispatch.c ───────────────────────────────────── */

typedef struct
{
    char *family;
    char *action;
    int direction; /* 0 = client, 1 = server */
    char *struct_name;
    int has_storage;
    int has_heap;
} DispatchEntry;

static void write_dispatch_case(FILE *f, const DispatchEntry *e)
{
    fprintf(f, "                    if (direction == %d) {\n", e->direction);
    if (e->has_storage)
    {
        fprintf(f, "                        %s pkt;\n", e->struct_name);
        fprintf(f, "                        result = %s_deserialize(&pkt, &reader);\n",
                e->struct_name);
        fprintf(f, "                        if (result != -1) {\n");
        fprintf(f, "                            result = %s_serialize(&pkt, &writer);\n",
                e->struct_name);
        if (e->has_heap)
            fprintf(f, "                            %s_free(&pkt);\n", e->struct_name);
        fprintf(f, "                        }\n");
    }
    else
    {
        fprintf(f, "                        result = %s_deserialize(&reader);\n",
                e->struct_name);
        fprintf(f, "                        if (result != -1)\n");
        fprintf(f, "                            result = %s_serialize(&writer);\n",
                e->struct_name);
    }
    fprintf(f, "                    }\n");
}

static void write_wasm_dispatch(ProtocolDef *protocols, size_t protocol_count,
                                const PacketStorageMap *map)
{
    DispatchEntry *entries = NULL;
    size_t count = 0, capacity = 0;

    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *proto = &protocols[p];
        int is_server = (strstr(proto->path, "/server/") ||
                         strstr(proto->path, "/server/protocol.xml"))
                            ? 1
                            : 0;
        const char *suffix = is_server ? "Server" : "Client";

        for (size_t i = 0; i < proto->packets_count; ++i)
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "%s%s%sPacket",
                     proto->packets[i].family, proto->packets[i].action, suffix);
            if (count >= capacity)
            {
                capacity = ARRAY_GROW_CAPACITY(capacity);
                entries = realloc(entries, capacity * sizeof(DispatchEntry));
            }
            entries[count].family = xstrdup(proto->packets[i].family);
            entries[count].action = xstrdup(proto->packets[i].action);
            entries[count].direction = is_server;
            entries[count].struct_name = xstrdup(buf);
            entries[count].has_storage = psm_storage(map, buf);
            entries[count].has_heap = psm_heap(map, buf);
            count++;
        }
    }

    FILE *f = fopen("wasm/wasm_protocol_dispatch.c", "w");
    if (!f)
    {
        fprintf(stderr, "Failed to open wasm/wasm_protocol_dispatch.c\n");
        goto cleanup;
    }

    fprintf(f, "%s", CODEGEN_WARNING);
    fprintf(f, "#include \"../include/protocol.h\"\n");
    fprintf(f, "#include \"../include/data.h\"\n");
    fprintf(f, "#include <stdint.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <string.h>\n\n");

    fprintf(f, "static uint8_t g_roundtrip_buf[65536];\n");
    fprintf(f, "static int g_roundtrip_len = 0;\n\n");
    fprintf(f, "int wasm_get_roundtrip_len(void) { return g_roundtrip_len; }\n\n");

    fprintf(f,
            "const uint8_t *wasm_packet_roundtrip(\n"
            "    int family, int action, int direction,\n"
            "    const uint8_t *input, int input_len)\n"
            "{\n");
    fprintf(f, "    g_roundtrip_len = 0;\n");
    fprintf(f,
            "    EoReader reader = eo_reader_init(\n"
            "        input_len > 0 ? input : NULL,\n"
            "        (size_t)(input_len > 0 ? input_len : 0));\n");
    fprintf(f,
            "    EoWriter writer = eo_writer_init_with_capacity(\n"
            "        (size_t)(input_len > 0 ? input_len : 1));\n");
    fprintf(f, "    int result = 0;\n\n");

    /* Collect unique families */
    StringList families = {0};
    for (size_t i = 0; i < count; ++i)
        string_list_push_unique(&families, entries[i].family);

    fprintf(f, "    switch (family) {\n");
    for (size_t fi = 0; fi < families.count; ++fi)
    {
        const char *fam = families.items[fi];
        char *fam_upper = to_upper_snake(fam);
        fprintf(f, "        case PACKET_FAMILY_%s:\n", fam_upper);
        fprintf(f, "            switch (action) {\n");
        free(fam_upper);

        StringList actions = {0};
        for (size_t i = 0; i < count; ++i)
            if (strcmp(entries[i].family, fam) == 0)
                string_list_push_unique(&actions, entries[i].action);

        for (size_t ai = 0; ai < actions.count; ++ai)
        {
            const char *act = actions.items[ai];
            char *act_upper = to_upper_snake(act);
            fprintf(f, "                case PACKET_ACTION_%s:\n", act_upper);
            free(act_upper);

            for (size_t i = 0; i < count; ++i)
            {
                if (strcmp(entries[i].family, fam) != 0 ||
                    strcmp(entries[i].action, act) != 0)
                    continue;
                write_dispatch_case(f, &entries[i]);
            }
            fprintf(f, "                    break;\n");
        }
        string_list_free(&actions);

        fprintf(f, "            }\n");
        fprintf(f, "            break;\n");
    }
    string_list_free(&families);
    fprintf(f, "    }\n\n");

    fprintf(f,
            "    if (result != -1 && writer.length > 0 &&\n"
            "        writer.length <= sizeof(g_roundtrip_buf)) {\n"
            "        memcpy(g_roundtrip_buf, writer.data, writer.length);\n"
            "        g_roundtrip_len = (int)writer.length;\n"
            "    }\n");
    fprintf(f, "    free(writer.data);\n");
    fprintf(f, "    return g_roundtrip_len > 0 ? g_roundtrip_buf : NULL;\n");
    fprintf(f, "}\n");

    fclose(f);
    printf("Generated wasm/wasm_protocol_dispatch.c\n");

cleanup:
    for (size_t i = 0; i < count; ++i)
    {
        free(entries[i].family);
        free(entries[i].action);
        free(entries[i].struct_name);
    }
    free(entries);
}

/* ─── Write wasm/packets.js ─────────────────────────────────────────────────── */

static int b64_decoded_length(const char *b64)
{
    size_t len = strlen(b64);
    if (len == 0)
        return 0;
    int out = (int)((len / 4) * 3);
    if (len >= 1 && b64[len - 1] == '=')
        out--;
    if (len >= 2 && b64[len - 2] == '=')
        out--;
    return out;
}

static void write_wasm_packets_js(ProtocolDef *protocols, size_t protocol_count)
{
    FILE *f = fopen("wasm/packets.js", "w");
    if (!f)
    {
        fprintf(stderr, "Failed to open wasm/packets.js\n");
        return;
    }

    fprintf(f, "// DO NOT EDIT - Generated by code_gen\n\n");
    fprintf(f, "const PACKETS = [\n");

    const char *dirs[] = {"client", "server"};
    int first = 1;

    for (int d = 0; d < 2; ++d)
    {
        char dir_path[256];
        snprintf(dir_path, sizeof(dir_path), "eo-captured-packets/%s", dirs[d]);

        StringList paths = {0};
        list_json_files_in_dir(dir_path, &paths);

        for (size_t i = 0; i < paths.count; ++i)
        {
            struct json_object *root = json_object_from_file(paths.items[i]);
            if (!root)
                continue;

            struct json_object *fam_obj = NULL, *act_obj = NULL, *exp_obj = NULL,
                               *props_obj = NULL;
            if (!json_object_object_get_ex(root, "family", &fam_obj) ||
                !json_object_object_get_ex(root, "action", &act_obj) ||
                !json_object_object_get_ex(root, "expected", &exp_obj))
            {
                json_object_put(root);
                continue;
            }
            json_object_object_get_ex(root, "properties", &props_obj);

            const char *family = json_object_get_string(fam_obj);
            const char *action = json_object_get_string(act_obj);
            const char *expected = json_object_get_string(exp_obj);

            if (b64_decoded_length(expected) > 1024)
            {
                json_object_put(root);
                continue;
            }

            int family_id = find_enum_value(protocols, protocol_count, "PacketFamily", family);
            int action_id = find_enum_value(protocols, protocol_count, "PacketAction", action);
            const char *comment = find_packet_comment(protocols, protocol_count,
                                                       family, action, d);

            /* Derive stem from filename */
            const char *basename = strrchr(paths.items[i], '/');
            basename = basename ? basename + 1 : paths.items[i];
            size_t stem_len = strlen(basename);
            if (stem_len > 5 && strcmp(basename + stem_len - 5, ".json") == 0)
                stem_len -= 5;
            char *stem = malloc(stem_len + 1);
            memcpy(stem, basename, stem_len);
            stem[stem_len] = '\0';

            if (!first)
                fprintf(f, ",\n");
            first = 0;

            /* Use json-c to safely escape comment and stem strings */
            struct json_object *comment_js = json_object_new_string(comment ? comment : "");
            struct json_object *capture_js = json_object_new_string(stem);

            fprintf(f, "  {\n");
            fprintf(f, "    \"id\": \"%s_%s\",\n", dirs[d], stem);
            fprintf(f, "    \"capture\": %s,\n", json_object_to_json_string(capture_js));
            fprintf(f, "    \"comment\": %s,\n", json_object_to_json_string(comment_js));
            fprintf(f, "    \"family\": \"%s\",\n", family);
            fprintf(f, "    \"action\": \"%s\",\n", action);
            fprintf(f, "    \"direction\": %d,\n", d);
            fprintf(f, "    \"familyId\": %d,\n", family_id);
            fprintf(f, "    \"actionId\": %d,\n", action_id);
            fprintf(f, "    \"expected\": \"%s\",\n", expected);
            fprintf(f, "    \"properties\": ");
            if (props_obj)
                fputs(json_object_to_json_string(props_obj), f);
            else
                fputs("[]", f);
            fprintf(f, "\n  }");

            json_object_put(comment_js);
            json_object_put(capture_js);
            free(stem);
            json_object_put(root);
        }
        string_list_free(&paths);
    }

    fprintf(f, "\n];\n");
    fclose(f);
    printf("Generated wasm/packets.js\n");
}

/* ─── Public entry point ────────────────────────────────────────────────────── */

void write_wasm_files(ProtocolDef *protocols, size_t protocol_count)
{
    PacketStorageMap map = build_psm(protocols, protocol_count);
    write_wasm_dispatch(protocols, protocol_count, &map);
    write_wasm_packets_js(protocols, protocol_count);
    psm_free(&map);
}
