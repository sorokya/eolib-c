#include "code_gen_xml.h"
#include "code_gen_utils.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static ElementList parse_elements(xmlNode *parent);

static int parse_bool_attr(xmlNode *node, const char *name, int default_value)
{
    xmlChar *value = xmlGetProp(node, (const xmlChar *)name);
    if (!value)
    {
        return default_value;
    }
    int result = (xmlStrcasecmp(value, (const xmlChar *)"true") == 0 ||
                  xmlStrcasecmp(value, (const xmlChar *)"1") == 0)
                     ? 1
                     : 0;
    xmlFree(value);
    return result;
}

static int parse_optional_bool_attr(xmlNode *node, const char *name, const char *fallback_name,
                                    int default_value)
{
    xmlChar *value = xmlGetProp(node, (const xmlChar *)name);
    if (!value && fallback_name)
    {
        value = xmlGetProp(node, (const xmlChar *)fallback_name);
    }
    if (!value)
    {
        return default_value;
    }
    int result = (xmlStrcasecmp(value, (const xmlChar *)"true") == 0 ||
                  xmlStrcasecmp(value, (const xmlChar *)"1") == 0)
                     ? 1
                     : 0;
    xmlFree(value);
    return result;
}

static char *get_attr(xmlNode *node, const char *name)
{
    xmlChar *value = xmlGetProp(node, (const xmlChar *)name);
    if (!value)
    {
        return NULL;
    }
    char *out = xstrdup((const char *)value);
    xmlFree(value);
    return out;
}

static char *get_node_text(xmlNode *node)
{
    xmlChar *value = xmlNodeGetContent(node);
    if (!value)
    {
        return NULL;
    }
    char *out = xstrdup((const char *)value);
    xmlFree(value);
    return out;
}

static char *get_direct_text(xmlNode *node)
{
    if (!node)
    {
        return NULL;
    }

    size_t total_len = 0;
    for (xmlNode *child = node->children; child; child = child->next)
    {
        if (child->type == XML_TEXT_NODE && child->content)
        {
            total_len += xmlStrlen(child->content);
        }
    }

    if (total_len == 0)
    {
        return NULL;
    }

    char *buffer = xmalloc(total_len + 1);
    size_t offset = 0;
    for (xmlNode *child = node->children; child; child = child->next)
    {
        if (child->type == XML_TEXT_NODE && child->content)
        {
            size_t len = xmlStrlen(child->content);
            memcpy(buffer + offset, child->content, len);
            offset += len;
        }
    }
    buffer[offset] = '\0';

    size_t start = 0;
    while (buffer[start] && isspace((unsigned char)buffer[start]))
    {
        start++;
    }

    size_t end = offset;
    while (end > start && isspace((unsigned char)buffer[end - 1]))
    {
        end--;
    }

    if (end <= start)
    {
        free(buffer);
        return NULL;
    }

    size_t trimmed_len = end - start;
    char *trimmed = xmalloc(trimmed_len + 1);
    memcpy(trimmed, buffer + start, trimmed_len);
    trimmed[trimmed_len] = '\0';
    free(buffer);
    return trimmed;
}

static EnumDef parse_enum(xmlNode *node)
{
    EnumDef def = {0};
    def.name = get_attr(node, "name");
    def.data_type = get_attr(node, "type");
    def.comment = NULL;

    for (xmlNode *child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
        {
            continue;
        }
        if (xmlStrEqual(child->name, (const xmlChar *)"comment"))
        {
            def.comment = get_node_text(child);
        }
        else if (xmlStrEqual(child->name, (const xmlChar *)"value"))
        {
            EnumValue value = {0};
            value.name = get_attr(child, "name");
            value.comment = NULL;
            value.data_type = def.data_type;

            xmlNode *comment_node = NULL;
            for (xmlNode *sub = child->children; sub; sub = sub->next)
            {
                if (sub->type == XML_ELEMENT_NODE &&
                    xmlStrEqual(sub->name, (const xmlChar *)"comment"))
                {
                    comment_node = sub;
                    break;
                }
            }

            if (comment_node)
            {
                value.comment = get_node_text(comment_node);
            }

            char *value_text = get_direct_text(child);
            value.value = value_text ? atoi(value_text) : 0;
            free(value_text);
            enum_values_push(&def, value);
        }
    }

    return def;
}

static FieldDef parse_field(xmlNode *node)
{
    FieldDef field = {0};
    field.name = get_attr(node, "name");
    field.data_type = get_attr(node, "type");
    field.value = get_direct_text(node);
    field.comment = NULL;
    field.padded = parse_bool_attr(node, "padded", 0);
    field.optional = parse_bool_attr(node, "optional", 0);
    field.length = get_attr(node, "length");

    for (xmlNode *child = node->children; child; child = child->next)
    {
        if (child->type == XML_ELEMENT_NODE &&
            xmlStrEqual(child->name, (const xmlChar *)"comment"))
        {
            field.comment = get_node_text(child);
            break;
        }
    }

    return field;
}

static ArrayDef parse_array(xmlNode *node)
{
    ArrayDef array = {0};
    array.name = get_attr(node, "name");
    array.data_type = get_attr(node, "type");
    array.length = get_attr(node, "length");
    array.optional = parse_bool_attr(node, "optional", 0);
    array.delimited = parse_bool_attr(node, "delimited", 0);
    array.trailing_delimiter = parse_bool_attr(node, "trailing-delimiter", 1);
    array.comment = NULL;

    for (xmlNode *child = node->children; child; child = child->next)
    {
        if (child->type == XML_ELEMENT_NODE &&
            xmlStrEqual(child->name, (const xmlChar *)"comment"))
        {
            array.comment = get_node_text(child);
            break;
        }
    }

    return array;
}

static LengthDef parse_length(xmlNode *node)
{
    LengthDef length = {0};
    length.name = get_attr(node, "name");
    length.data_type = get_attr(node, "type");
    length.optional = parse_optional_bool_attr(node, "optional", "length", 0);
    char *offset = get_attr(node, "offset");
    length.offset = offset ? atoi(offset) : 0;
    free(offset);
    return length;
}

static DummyDef parse_dummy(xmlNode *node)
{
    DummyDef dummy = {0};
    dummy.data_type = get_attr(node, "type");
    dummy.value = get_direct_text(node);
    return dummy;
}

static SwitchDef parse_switch(xmlNode *node)
{
    SwitchDef sw = {0};
    sw.field = get_attr(node, "field");
    for (xmlNode *child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE ||
            !xmlStrEqual(child->name, (const xmlChar *)"case"))
        {
            continue;
        }

        CaseDef case_def = {0};
        case_def.is_default = parse_bool_attr(child, "default", 0);
        case_def.value = get_attr(child, "value");
        case_def.has_elements = 0;

        ElementList elements = parse_elements(child);
        if (elements.elements_count > 0)
        {
            case_def.has_elements = 1;
            case_def.elements = elements;
        }

        switch_cases_push(&sw, case_def);
    }
    return sw;
}

static StructElement parse_element(xmlNode *node)
{
    StructElement element = {0};
    if (xmlStrEqual(node->name, (const xmlChar *)"break"))
    {
        element.kind = ELEMENT_BREAK;
    }
    else if (xmlStrEqual(node->name, (const xmlChar *)"comment"))
    {
        element.kind = ELEMENT_COMMENT;
        element.as.comment = get_node_text(node);
    }
    else if (xmlStrEqual(node->name, (const xmlChar *)"dummy"))
    {
        element.kind = ELEMENT_DUMMY;
        element.as.dummy = parse_dummy(node);
    }
    else if (xmlStrEqual(node->name, (const xmlChar *)"field"))
    {
        element.kind = ELEMENT_FIELD;
        element.as.field = parse_field(node);
    }
    else if (xmlStrEqual(node->name, (const xmlChar *)"array"))
    {
        element.kind = ELEMENT_ARRAY;
        element.as.array = parse_array(node);
    }
    else if (xmlStrEqual(node->name, (const xmlChar *)"length"))
    {
        element.kind = ELEMENT_LENGTH;
        element.as.length = parse_length(node);
    }
    else if (xmlStrEqual(node->name, (const xmlChar *)"switch"))
    {
        element.kind = ELEMENT_SWITCH;
        element.as.sw = parse_switch(node);
    }
    else if (xmlStrEqual(node->name, (const xmlChar *)"chunked"))
    {
        element.kind = ELEMENT_CHUNKED;
        element.as.chunked = parse_elements(node);
    }
    else
    {
        element.kind = ELEMENT_COMMENT;
        element.as.comment = NULL;
    }
    return element;
}

static ElementList parse_elements(xmlNode *parent)
{
    ElementList list = {0};
    for (xmlNode *child = parent->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
        {
            continue;
        }
        StructElement element = parse_element(child);
        element_list_push(&list, element);
    }
    return list;
}

static StructDef parse_struct(xmlNode *node)
{
    StructDef def = {0};
    def.name = get_attr(node, "name");
    def.elements = parse_elements(node);
    return def;
}

static PacketDef parse_packet(xmlNode *node)
{
    PacketDef def = {0};
    def.action = get_attr(node, "action");
    def.family = get_attr(node, "family");
    def.comment = NULL;
    for (xmlNode *child = node->children; child; child = child->next)
    {
        if (child->type == XML_ELEMENT_NODE &&
            xmlStrEqual(child->name, (const xmlChar *)"comment"))
        {
            def.comment = get_node_text(child);
            break;
        }
    }
    def.elements = parse_elements(node);
    return def;
}

ProtocolDef parse_protocol_file(const char *path)
{
    ProtocolDef protocol = {0};
    protocol.path = xstrdup(path);

    xmlDoc *doc = xmlReadFile(path, NULL, XML_PARSE_NOBLANKS);
    if (!doc)
    {
        fprintf(stderr, "Failed to parse XML: %s\n", path);
        exit(1);
    }

    xmlNode *root = xmlDocGetRootElement(doc);
    if (!root)
    {
        fprintf(stderr, "Empty XML: %s\n", path);
        xmlFreeDoc(doc);
        exit(1);
    }

    for (xmlNode *node = root->children; node; node = node->next)
    {
        if (node->type != XML_ELEMENT_NODE)
        {
            continue;
        }
        if (xmlStrEqual(node->name, (const xmlChar *)"enum"))
        {
            protocol_enums_push(&protocol, parse_enum(node));
        }
        else if (xmlStrEqual(node->name, (const xmlChar *)"struct"))
        {
            protocol_structs_push(&protocol, parse_struct(node));
        }
        else if (xmlStrEqual(node->name, (const xmlChar *)"packet"))
        {
            protocol_packets_push(&protocol, parse_packet(node));
        }
    }

    xmlFreeDoc(doc);
    return protocol;
}
