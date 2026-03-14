#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eolib/eolib.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <richedit.h>

#ifndef RICHEDIT_CLASSA
#define RICHEDIT_CLASSA "RichEdit20A"
#endif

#ifdef CLEARTYPE_QUALITY
#define XP_FONT_QUALITY CLEARTYPE_QUALITY
#else
#define XP_FONT_QUALITY DEFAULT_QUALITY
#endif

#ifndef GWLP_USERDATA
#define GWLP_USERDATA GWL_USERDATA
#define SetWindowLongPtrA SetWindowLongA
#define GetWindowLongPtrA GetWindowLongA
#define LONG_PTR LONG
#endif

#define APP_CLASS_NAME "EOLibLauncherWindow"
#define MODULE_CLASS_NAME "EOLibModuleWindow"
#define APP_TITLE "EOLib Demo Station"

#define MAX_FIELDS 4

#define ID_MAIN_BUTTON_BASE 1000

#define ID_MODULE_RUN 2001
#define ID_MODULE_RESET 2002
#define ID_MODULE_CLOSE 2003
#define ID_MODULE_RANDOM_SWAP 2004
#define ID_MODULE_MODE_ENCRYPT 2005
#define ID_MODULE_MODE_DECRYPT 2006
#define ID_MODULE_LOG 2010
#define ID_MODULE_PACKET_LIST 2011
#define ID_MODULE_FIRST_LABEL 2100
#define ID_MODULE_FIRST_EDIT 2200
#define ID_MODULE_DESC 2300

#define COLOR_MAIN_BG RGB(246, 241, 231)
#define COLOR_MAIN_PANEL RGB(255, 250, 242)
#define COLOR_MAIN_HEADER RGB(39, 58, 76)
#define COLOR_TEXT_DARK RGB(48, 47, 44)
#define COLOR_TEXT_SOFT RGB(99, 94, 86)
#define COLOR_HEX_FF RGB(193, 60, 36)
#define COLOR_HEX_FE RGB(176, 121, 29)
#define COLOR_HEX_STRING RGB(43, 96, 170)
#define COLOR_HEX_DEFAULT RGB(28, 28, 28)

typedef enum DemoModule
{
    MODULE_RNG = 0,
    MODULE_SEQUENCER = 1,
    MODULE_DATA = 2,
    MODULE_ENCRYPT = 3,
    MODULE_PACKET_BROWSER = 4,
    MODULE_TEXT = 5,
    MODULE_COUNT = 6
} DemoModule;

typedef struct PacketValueName
{
    int value;
    const char *name;
} PacketValueName;

typedef struct PacketCaptureExample
{
    const char *label;
    int family;
    int action;
    int side; /* 0 = client, 1 = server */
    const char *description;
    const char *source;
    const uint8_t *payload;
    size_t payloadLength;
} PacketCaptureExample;

typedef struct ModuleDefinition
{
    DemoModule module;
    const char *title;
    const char *subtitle;
    const char *description;
    const char *cardTitle;
    const char *cardText;
    int fieldCount;
    const char *fieldLabels[MAX_FIELDS];
    const char *fieldDefaults[MAX_FIELDS];
} ModuleDefinition;

typedef struct MainState
{
    HWND hwnd;
    HWND moduleWindows[MODULE_COUNT];
    HFONT titleFont;
    HFONT subtitleFont;
    HFONT cardTitleFont;
    HFONT cardBodyFont;
    HBRUSH bgBrush;
} MainState;

typedef struct ModuleWindowState
{
    MainState *owner;
    DemoModule module;
    HWND hwndDesc;
    HWND hwndLabels[MAX_FIELDS];
    HWND hwndEdits[MAX_FIELDS];
    HWND hwndRun;
    HWND hwndReset;
    HWND hwndClose;
    HWND hwndPacketRandomSwap;
    HWND hwndModeEncrypt;
    HWND hwndModeDecrypt;
    HWND hwndPacketList;
    HWND hwndLog;
    HFONT titleFont;
    HFONT subtitleFont;
    HFONT bodyFont;
    HFONT labelFont;
    HFONT monoFont;
    HBRUSH bgBrush;
    HBRUSH panelBrush;
    int rngSeedInitialized;
    int rngSeedIsManual;
    uint32_t rngCurrentSeed;
} ModuleWindowState;

static HMODULE g_richedit_lib = NULL;

static const ModuleDefinition g_modules[MODULE_COUNT] = {
    {MODULE_RNG,
     "RNG Lab",
     "Seed the generator and inspect verification math.",
     "Use a deterministic seed or leave it blank to fall back to the current tick count."
     " Range minimum and maximum are used for eo_rand_range().",
     "RNG Lab",
     "Random numbers, ranged values, challenge generation, and verification hash.",
     3,
     {"Seed", "Range minimum", "Range maximum", NULL},
     {"1337", "1", "1000", NULL}},
    {MODULE_SEQUENCER,
     "Sequencer",
     "Explore sequence progression and byte derivation.",
     "Provide a starting value and sample count to inspect sequence progression."
     " Derived start values are computed by roundtripping generated init/ping bytes.",
     "Sequencer",
     "Advance a sequencer, derive init bytes, and compare start calculations.",
     2,
     {"Start value", "Sample count", NULL, NULL},
     {"69", "11", NULL, NULL}},
    {MODULE_DATA,
     "Data Lab",
     "Work with EO numbers, strings, and the writer/reader pipeline.",
     "This module combines number encoding, string transforms, fixed string IO, and"
     " serialized buffer inspection.",
     "Data Lab",
     "Numbers, EO string transform, fixed strings, and writer/reader roundtrip.",
     4,
     {"Number to encode", "Raw fixed string", "Encoded fixed string", "Fixed length"},
     {"123456", "RAW-DEMO", "ENCODED", "12"}},
    {MODULE_ENCRYPT,
     "Encryption",
     "Encrypt or decrypt EO packet bytes with a selected mode.",
     "Choose mode (encrypt/decrypt), provide a swap multiple, and enter packet bytes as hex.",
     "Encryption",
     "Run encryption/decryption and verify a roundtrip with the selected mode.",
     3,
     {"Mode", "Swap multiple", "Packet bytes (hex)", NULL},
     {NULL, "8", "8A 12 18 1E 24 2A 30 36", NULL}},
    {MODULE_PACKET_BROWSER,
     "Packet Browser",
     "Click through captured packet examples and inspect roundtrip bytes.",
     "Select a packet to inspect family/action and a payload encrypt/decrypt"
     " roundtrip from captured expected payload bytes.",
     "Packet Browser",
     "Browse a packet capture list and inspect per-packet roundtrip bytes.",
     1,
     {"Swap multiple (6-12)", NULL, NULL, NULL},
     {"7", NULL, NULL, NULL}},
    {MODULE_TEXT,
     "Text Conversion",
     "Inspect UTF-8 to Windows-1252 conversion and EO text transforms.",
     "Use any text you want. The report shows a Windows-1252 conversion and the raw EO"
     " string transform bytes for the same input.",
     "Text",
     "Windows-1252 conversion plus EO encode/decode string helpers.",
     1,
     {"UTF-8 text", NULL, NULL, NULL},
     {"Cafe - naive text", NULL, NULL, NULL}}};

static COLORREF module_color(DemoModule module)
{
    switch (module)
    {
    case MODULE_RNG:
        return RGB(77, 122, 94);
    case MODULE_SEQUENCER:
        return RGB(70, 95, 154);
    case MODULE_DATA:
        return RGB(177, 114, 49);
    case MODULE_ENCRYPT:
        return RGB(160, 76, 63);
    case MODULE_PACKET_BROWSER:
        return RGB(121, 85, 136);
    case MODULE_TEXT:
        return RGB(115, 107, 58);
    }

    return RGB(90, 90, 90);
}

static const ModuleDefinition *get_module_definition(DemoModule module)
{
    if ((int)module < 0 || module >= MODULE_COUNT)
    {
        return NULL;
    }

    return &g_modules[module];
}

static const PacketValueName g_packet_families[] = {
    {1, "CONNECTION"}, {2, "ACCOUNT"}, {3, "CHARACTER"}, {4, "LOGIN"}, {5, "WELCOME"}, {6, "WALK"}, {7, "FACE"}, {8, "CHAIR"}, {9, "EMOTE"}, {11, "ATTACK"}, {12, "SPELL"}, {13, "SHOP"}, {14, "ITEM"}, {16, "STAT_SKILL"}, {17, "GLOBAL"}, {18, "TALK"}, {19, "WARP"}, {21, "JUKEBOX"}, {22, "PLAYERS"}, {23, "AVATAR"}, {24, "PARTY"}, {25, "REFRESH"}, {26, "NPC"}, {27, "PLAYER_RANGE"}, {28, "NPC_RANGE"}, {29, "RANGE"}, {30, "PAPERDOLL"}, {31, "EFFECT"}, {32, "TRADE"}, {33, "CHEST"}, {34, "DOOR"}, {35, "MESSAGE"}, {36, "BANK"}, {37, "LOCKER"}, {38, "BARBER"}, {39, "GUILD"}, {40, "MUSIC"}, {41, "SIT"}, {42, "RECOVER"}, {43, "BOARD"}, {44, "CAST"}, {45, "ARENA"}, {46, "PRIEST"}, {47, "MARRIAGE"}, {48, "ADMIN_INTERACT"}, {49, "CITIZEN"}, {50, "QUEST"}, {51, "BOOK"}, {250, "ERROR"}, {255, "INIT"}};

static const PacketValueName g_packet_actions[] = {
    {1, "REQUEST"}, {2, "ACCEPT"}, {3, "REPLY"}, {4, "REMOVE"}, {5, "AGREE"}, {6, "CREATE"}, {7, "ADD"}, {8, "PLAYER"}, {9, "TAKE"}, {10, "USE"}, {11, "BUY"}, {12, "SELL"}, {13, "OPEN"}, {14, "CLOSE"}, {15, "MSG"}, {16, "SPEC"}, {17, "ADMIN"}, {18, "LIST"}, {20, "TELL"}, {21, "REPORT"}, {22, "ANNOUNCE"}, {23, "SERVER"}, {24, "DROP"}, {25, "JUNK"}, {26, "OBTAIN"}, {27, "GET"}, {28, "KICK"}, {29, "RANK"}, {30, "TARGET_SELF"}, {31, "TARGET_OTHER"}, {33, "TARGET_GROUP"}, {34, "DIALOG"}, {240, "PING"}, {241, "PONG"}, {242, "NET242"}, {243, "NET243"}, {244, "NET244"}, {250, "ERROR"}, {255, "INIT"}};

#include "packets.inc"

static const char *find_packet_name(const PacketValueName *items, size_t count, int value)
{
    size_t i;

    for (i = 0; i < count; ++i)
    {
        if (items[i].value == value)
        {
            return items[i].name;
        }
    }

    return NULL;
}

static size_t append_packet_slice(char *out, size_t outSize, size_t offset, const char *title, const PacketValueName *items, size_t count, int startValue)
{
    size_t i;
    int shown = 0;

    if (!out || outSize == 0)
    {
        return 0;
    }

    if (offset >= outSize)
    {
        offset = outSize - 1;
        out[offset] = '\0';
    }

    if (offset < outSize)
    {
        int written = snprintf(out + offset, outSize - offset, "%s\r\n", title);
        if (written < 0)
        {
            out[offset] = '\0';
            return offset;
        }
        offset += (size_t)written;
        if (offset >= outSize)
        {
            offset = outSize - 1;
            out[offset] = '\0';
        }
    }

    for (i = 0; i < count; ++i)
    {
        if (items[i].value < startValue)
        {
            continue;
        }

        if (offset < outSize)
        {
            int written = snprintf(out + offset, outSize - offset, "  %3d -> %s\r\n", items[i].value, items[i].name);
            if (written < 0)
            {
                out[offset] = '\0';
                return offset;
            }
            offset += (size_t)written;
            if (offset >= outSize)
            {
                offset = outSize - 1;
                out[offset] = '\0';
                return offset;
            }
        }

        ++shown;
        if (shown >= 8)
        {
            break;
        }
    }

    if (shown == 0 && offset < outSize)
    {
        int written = snprintf(out + offset, outSize - offset, "  (no entries at or above %d)\r\n", startValue);
        if (written < 0)
        {
            out[offset] = '\0';
            return offset;
        }
        offset += (size_t)written;
        if (offset >= outSize)
        {
            offset = outSize - 1;
            out[offset] = '\0';
        }
    }

    return offset;
}

static void get_text(HWND hwnd, char *out, size_t outSize)
{
    if (!out || outSize == 0)
    {
        return;
    }

    GetWindowTextA(hwnd, out, (int)outSize);
    out[outSize - 1] = '\0';
}

static void set_text(HWND hwnd, const char *text)
{
    SetWindowTextA(hwnd, text ? text : "");
}

static int parse_int32_str(const char *text, int32_t *out)
{
    char *end;
    long value;

    if (!text || !*text || !out)
    {
        return 0;
    }

    value = strtol(text, &end, 10);
    while (end && *end && isspace((unsigned char)*end))
    {
        ++end;
    }

    if (end && *end)
    {
        return 0;
    }

    *out = (int32_t)value;
    return 1;
}

static int parse_uint32_str(const char *text, uint32_t *out)
{
    char *end;
    unsigned long value;

    if (!text || !*text || !out)
    {
        return 0;
    }

    value = strtoul(text, &end, 10);
    while (end && *end && isspace((unsigned char)*end))
    {
        ++end;
    }

    if (end && *end)
    {
        return 0;
    }

    *out = (uint32_t)value;
    return 1;
}

static void bytes_to_hex(const uint8_t *bytes, size_t byteCount, char *out, size_t outSize)
{
    size_t i;
    size_t pos = 0;

    if (!out || outSize == 0)
    {
        return;
    }

    out[0] = '\0';

    for (i = 0; i < byteCount; ++i)
    {
        int written;
        if (pos + 4 >= outSize)
        {
            break;
        }

        written = snprintf(out + pos, outSize - pos, "%02X%s", bytes[i], (i + 1 < byteCount) ? " " : "");
        if (written < 0)
        {
            break;
        }

        pos += (size_t)written;
    }
}

static void bytes_to_hex_preview(const uint8_t *bytes, size_t byteCount, size_t previewBytes, char *out, size_t outSize)
{
    size_t shown = byteCount;

    if (!out || outSize == 0)
    {
        return;
    }

    if (!bytes)
    {
        snprintf(out, outSize, "(none)");
        return;
    }

    if (shown > previewBytes)
    {
        shown = previewBytes;
    }

    bytes_to_hex(bytes, shown, out, outSize);

    if (shown < byteCount)
    {
        size_t used = strlen(out);
        if (used < outSize)
        {
            snprintf(out + used, outSize - used, " ... (+%lu bytes)", (unsigned long)(byteCount - shown));
        }
    }
}

static int is_ascii_windows_1252_printable(uint8_t b)
{
    return b >= 0x20 && b <= 0x7E;
}

static size_t detect_printable_run(const uint8_t *bytes, size_t byteCount, size_t start)
{
    size_t i = start;

    while (i < byteCount && is_ascii_windows_1252_printable(bytes[i]))
    {
        ++i;
    }

    return i - start;
}

static int is_rich_edit_control(HWND hwnd)
{
    char className[32];

    if (!hwnd)
    {
        return 0;
    }

    className[0] = '\0';
    GetClassNameA(hwnd, className, (int)sizeof(className));
    return (strstr(className, "RichEdit") != NULL);
}

static void rich_append_colored(HWND hwnd, const char *text, COLORREF color)
{
    long start;
    CHARFORMAT2A format;

    if (!hwnd || !text || !*text)
    {
        return;
    }

    start = GetWindowTextLengthA(hwnd);
    SendMessageA(hwnd, EM_SETSEL, (WPARAM)start, (LPARAM)start);

    ZeroMemory(&format, sizeof(format));
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR | CFM_EFFECTS;
    format.dwEffects = 0;
    format.crTextColor = color;
    SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
    SendMessageA(hwnd, EM_REPLACESEL, FALSE, (LPARAM)text);
}

static void rich_append_hex_dump(HWND hwnd, const uint8_t *bytes, size_t byteCount, size_t previewBytes)
{
    size_t shown = byteCount;
    size_t i = 0;

    if (!hwnd)
    {
        return;
    }

    if (!bytes || byteCount == 0)
    {
        rich_append_colored(hwnd, "(none)", COLOR_TEXT_SOFT);
        return;
    }

    if (shown > previewBytes)
    {
        shown = previewBytes;
    }

    while (i < shown)
    {
        size_t run = detect_printable_run(bytes, shown, i);

        if (run >= 4)
        {
            char token[320];
            size_t pos = 0;
            size_t j;

            token[pos++] = '"';
            for (j = 0; j < run && pos + 3 < sizeof(token); ++j)
            {
                char c = (char)bytes[i + j];
                if (c == '"' || c == '\\')
                {
                    token[pos++] = '\\';
                }
                token[pos++] = c;
            }
            token[pos++] = '"';
            token[pos] = '\0';

            rich_append_colored(hwnd, token, COLOR_HEX_STRING);
            i += run;
        }
        else
        {
            char byteText[4];
            COLORREF color = COLOR_HEX_DEFAULT;

            snprintf(byteText, sizeof(byteText), "%02X", bytes[i]);
            if (bytes[i] == 0xFF)
            {
                color = COLOR_HEX_FF;
            }
            else if (bytes[i] == 0xFE)
            {
                color = COLOR_HEX_FE;
            }

            rich_append_colored(hwnd, byteText, color);
            ++i;
        }

        if (i < shown)
        {
            rich_append_colored(hwnd, " ", COLOR_HEX_DEFAULT);
        }
    }

    if (shown < byteCount)
    {
        char suffix[64];
        snprintf(suffix, sizeof(suffix), " ... (+%lu bytes)", (unsigned long)(byteCount - shown));
        rich_append_colored(hwnd, suffix, COLOR_TEXT_SOFT);
    }
}

static void rich_append_hex_dump_plain(HWND hwnd, const uint8_t *bytes, size_t byteCount, size_t previewBytes)
{
    size_t shown = byteCount;
    size_t i;

    if (!hwnd)
    {
        return;
    }

    if (!bytes || byteCount == 0)
    {
        rich_append_colored(hwnd, "(none)", COLOR_TEXT_SOFT);
        return;
    }

    if (shown > previewBytes)
    {
        shown = previewBytes;
    }

    for (i = 0; i < shown; ++i)
    {
        char byteText[4];
        snprintf(byteText, sizeof(byteText), "%02X", bytes[i]);
        rich_append_colored(hwnd, byteText, COLOR_HEX_DEFAULT);
        if (i + 1 < shown)
        {
            rich_append_colored(hwnd, " ", COLOR_HEX_DEFAULT);
        }
    }

    if (shown < byteCount)
    {
        char suffix[64];
        snprintf(suffix, sizeof(suffix), " ... (+%lu bytes)", (unsigned long)(byteCount - shown));
        rich_append_colored(hwnd, suffix, COLOR_TEXT_SOFT);
    }
}

static void render_packet_capture_report_rich(ModuleWindowState *state, size_t index, uint8_t swapMultiple)
{
    const PacketCaptureExample *example;
    const char *familyName;
    const char *actionName;
    const char *sideName;
    const char *description;
    const uint8_t *data;
    uint8_t *encrypted;
    size_t length;
    int roundtripMatch;
    char line[512];

    if (!state || !state->hwndLog)
    {
        return;
    }

    if (index >= sizeof(g_packet_capture_examples) / sizeof(g_packet_capture_examples[0]))
    {
        set_text(state->hwndLog, "Select a packet from the list to inspect details.\r\n");
        return;
    }

    example = &g_packet_capture_examples[index];
    sideName = (example->side == 0) ? "client" : "server";
    familyName = find_packet_name(g_packet_families, sizeof(g_packet_families) / sizeof(g_packet_families[0]), example->family);
    actionName = find_packet_name(g_packet_actions, sizeof(g_packet_actions) / sizeof(g_packet_actions[0]), example->action);
    description = (example->description && example->description[0]) ? example->description : "(none)";
    data = example->payload;
    length = example->payloadLength;

    if (!data || length == 0)
    {
        set_text(state->hwndLog, "Packet Browser\r\n==============\r\nNo payload bytes for selected packet.\r\n");
        return;
    }

    encrypted = (uint8_t *)malloc(length);
    if (!encrypted)
    {
        set_text(state->hwndLog, "Packet Browser\r\n==============\r\nOut of memory while rendering packet report.\r\n");
        return;
    }

    memcpy(encrypted, data, length);
    eo_encrypt_packet(encrypted, length, swapMultiple);

    SetWindowTextA(state->hwndLog, "");
    rich_append_colored(state->hwndLog, "Packet Browser\r\n==============\r\n", COLOR_HEX_DEFAULT);

    snprintf(line, sizeof(line), "Packet                  : %s\r\n", example->label);
    rich_append_colored(state->hwndLog, line, COLOR_HEX_DEFAULT);
    snprintf(line, sizeof(line), "Side                    : %s\r\n", sideName);
    rich_append_colored(state->hwndLog, line, COLOR_HEX_DEFAULT);
    snprintf(line, sizeof(line), "Description             : %s\r\n", description);
    rich_append_colored(state->hwndLog, line, COLOR_HEX_DEFAULT);
    snprintf(line, sizeof(line), "Capture source          : %s\r\n", example->source);
    rich_append_colored(state->hwndLog, line, COLOR_HEX_DEFAULT);
    snprintf(line, sizeof(line), "Family                  : %d -> %s\r\n", example->family, familyName ? familyName : "(unknown)");
    rich_append_colored(state->hwndLog, line, COLOR_HEX_DEFAULT);
    snprintf(line, sizeof(line), "Action                  : %d -> %s\r\n", example->action, actionName ? actionName : "(unknown)");
    rich_append_colored(state->hwndLog, line, COLOR_HEX_DEFAULT);
    snprintf(line, sizeof(line), "Payload bytes           : %lu\r\n", (unsigned long)length);
    rich_append_colored(state->hwndLog, line, COLOR_HEX_DEFAULT);
    snprintf(line, sizeof(line), "Swap multiple           : %u\r\n\r\n", (unsigned int)swapMultiple);
    rich_append_colored(state->hwndLog, line, COLOR_HEX_DEFAULT);

    rich_append_colored(state->hwndLog, "Original payload        : ", COLOR_HEX_DEFAULT);
    rich_append_hex_dump(state->hwndLog, data, length, 512);
    rich_append_colored(state->hwndLog, "\r\n\r\n", COLOR_HEX_DEFAULT);

    rich_append_colored(state->hwndLog, "Encrypted payload       : ", COLOR_HEX_DEFAULT);
    rich_append_hex_dump_plain(state->hwndLog, encrypted, length, 512);
    rich_append_colored(state->hwndLog, "\r\n\r\n", COLOR_HEX_DEFAULT);

    eo_decrypt_packet(encrypted, length, swapMultiple);
    roundtripMatch = (memcmp(data, encrypted, length) == 0);

    rich_append_colored(state->hwndLog, "Decrypted payload       : ", COLOR_HEX_DEFAULT);
    rich_append_hex_dump(state->hwndLog, encrypted, length, 512);
    rich_append_colored(state->hwndLog, "\r\n\r\n", COLOR_HEX_DEFAULT);

    snprintf(line, sizeof(line), "Roundtrip match         : %s\r\n", roundtripMatch ? "yes" : "no");
    rich_append_colored(state->hwndLog, line, COLOR_HEX_DEFAULT);
    rich_append_colored(state->hwndLog, "Note: capture payload omits protocol header/length/sequence fields.\r\n", COLOR_TEXT_SOFT);
    rich_append_colored(state->hwndLog, "Legend: ", COLOR_TEXT_SOFT);
    rich_append_colored(state->hwndLog, "FF", COLOR_HEX_FF);
    rich_append_colored(state->hwndLog, " = break bytes, ", COLOR_TEXT_SOFT);
    rich_append_colored(state->hwndLog, "FE", COLOR_HEX_FE);
    rich_append_colored(state->hwndLog, " = padding, ", COLOR_TEXT_SOFT);
    rich_append_colored(state->hwndLog, "\"text\"", COLOR_HEX_STRING);
    rich_append_colored(state->hwndLog, " = printable windows-1252 runs.\r\n", COLOR_TEXT_SOFT);

    free(encrypted);
}

static int parse_hex_bytes(const char *text, uint8_t *out, size_t outMax, size_t *outLen)
{
    size_t count = 0;
    const char *cursor = text;

    if (!out || !outLen)
    {
        return 0;
    }

    while (cursor && *cursor)
    {
        char *end;
        unsigned long value;

        while (*cursor && (isspace((unsigned char)*cursor) || *cursor == ',' || *cursor == '-'))
        {
            ++cursor;
        }

        if (!*cursor)
        {
            break;
        }

        value = strtoul(cursor, &end, 16);
        if (end == cursor || value > 0xFFUL)
        {
            return 0;
        }

        if (count >= outMax)
        {
            return 0;
        }

        out[count++] = (uint8_t)value;
        cursor = end;
    }

    *outLen = count;
    return 1;
}

static void build_rng_report(ModuleWindowState *state, const char *seedText, const char *minText, const char *maxText, char *out, size_t outSize)
{
    uint32_t parsedSeed;
    uint32_t seedUsed;
    int32_t minValue;
    int32_t maxValue;
    uint32_t randomValue;
    uint32_t rangedValue;
    int32_t challenge;
    int32_t verifyHash;
    int hasManualSeed;

    hasManualSeed = parse_uint32_str(seedText, &parsedSeed);

    if (!state)
    {
        seedUsed = hasManualSeed ? parsedSeed : (uint32_t)GetTickCount();
        eo_srand(seedUsed);
    }
    else if (hasManualSeed)
    {
        if (!state->rngSeedInitialized || !state->rngSeedIsManual || state->rngCurrentSeed != parsedSeed)
        {
            state->rngCurrentSeed = parsedSeed;
            state->rngSeedInitialized = 1;
            state->rngSeedIsManual = 1;
            eo_srand(state->rngCurrentSeed);
        }
        seedUsed = state->rngCurrentSeed;
    }
    else
    {
        if (!state->rngSeedInitialized || state->rngSeedIsManual)
        {
            state->rngCurrentSeed = (uint32_t)GetTickCount();
            state->rngSeedInitialized = 1;
            state->rngSeedIsManual = 0;
            eo_srand(state->rngCurrentSeed);
        }
        seedUsed = state->rngCurrentSeed;
    }

    if (!parse_int32_str(minText, &minValue))
    {
        minValue = 1;
    }

    if (!parse_int32_str(maxText, &maxValue))
    {
        maxValue = 1000;
    }

    if (minValue > maxValue)
    {
        int32_t temp = minValue;
        minValue = maxValue;
        maxValue = temp;
    }

    randomValue = eo_rand();
    rangedValue = eo_rand_range((uint32_t)minValue, (uint32_t)maxValue);
    challenge = eo_generate_server_verification_challenge();
    verifyHash = eo_server_verification_hash(challenge);

    snprintf(
        out,
        outSize,
        "RNG Lab\r\n"
        "=======\r\n"
        "Seed used                : %lu\r\n"
        "Range                    : %ld to %ld\r\n"
        "eo_rand()                : %lu\r\n"
        "eo_rand_range()          : %lu\r\n"
        "Verification challenge   : %ld\r\n"
        "Verification hash        : %ld\r\n",
        (unsigned long)seedUsed,
        (long)minValue,
        (long)maxValue,
        (unsigned long)randomValue,
        (unsigned long)rangedValue,
        (long)challenge,
        (long)verifyHash);
}

static void build_sequencer_report(const char *startText, const char *countText, char *out, size_t outSize)
{
    int32_t start;
    int32_t sampleCount;
    int32_t initDerived = 0;
    int32_t pingDerived = 0;
    int i;
    char sequenceRows[640];
    uint8_t initBytes[2] = {0, 0};
    uint8_t pingBytes[2] = {0, 0};
    EoResult initResult;
    EoResult pingResult;
    EoSequencer seq;

    if (!parse_int32_str(startText, &start))
    {
        start = eo_generate_sequence_start();
    }

    if (!parse_int32_str(countText, &sampleCount) || sampleCount < 1)
    {
        sampleCount = 11;
    }

    seq = eo_sequencer_init(start);
    sequenceRows[0] = '\0';

    for (i = 0; i < sampleCount; ++i)
    {
        int32_t value = 0;
        EoResult nextResult = eo_sequencer_next(&seq, &value);
        char line[96];
        snprintf(line, sizeof(line), "next[%d] = %ld\r\n", i, (long)value);
        strncat(sequenceRows, line, sizeof(sequenceRows) - strlen(sequenceRows) - 1);
    }

    initResult = eo_sequence_init_bytes(start, initBytes);
    pingResult = eo_sequence_ping_bytes(start, pingBytes);
    if (initResult == EO_SUCCESS)
    {
        initDerived = eo_sequence_start_from_init((int32_t)initBytes[0], (int32_t)initBytes[1]);
    }
    if (pingResult == EO_SUCCESS)
    {
        pingDerived = eo_sequence_start_from_ping((int32_t)pingBytes[0], (int32_t)pingBytes[1]);
    }

    snprintf(
        out,
        outSize,
        "Sequencer\r\n"
        "=========\r\n"
        "Start value              : %ld\r\n"
        "init bytes               : %02X %02X\r\n"
        "ping bytes               : %02X %02X\r\n"
        "Derived from init bytes  : %ld\r\n"
        "Derived from ping bytes  : %ld\r\n"
        "Init roundtrip match     : %s\r\n"
        "Ping roundtrip match     : %s\r\n\r\n"
        "%s",
        (long)start,
        initBytes[0],
        initBytes[1],
        pingBytes[0],
        pingBytes[1],
        (long)initDerived,
        (long)pingDerived,
        (initResult == EO_SUCCESS && initDerived == start) ? "yes" : "no",
        (pingResult == EO_SUCCESS && pingDerived == start) ? "yes" : "no",
        sequenceRows);
}

static void build_data_report(const char *numberText, const char *rawText, const char *encodedText, const char *lengthText, char *out, size_t outSize)
{
    int32_t value;
    int32_t fixedLength;
    uint8_t encodedNumber[4] = {0, 0, 0, 0};
    size_t encodedLen = 4;
    EoResult encodeResult;
    int32_t decodedValue = 0;
    char numberHex[64];
    char stringWork[256];
    char stringHex[1024];
    EoWriter writer;
    EoReader reader;
    EoResult pipelineResult = EO_SUCCESS;
    uint8_t byteValue = 0;
    int32_t charValue = 0;
    int32_t shortValue = 0;
    int32_t threeValue = 0;
    int32_t intValue = 0;
    char *fixedRaw = NULL;
    char *fixedEncoded = NULL;
    char writerHex[1024];
    const char *rawSource = (rawText && rawText[0]) ? rawText : "RAW-DEMO";
    const char *encodedSource = (encodedText && encodedText[0]) ? encodedText : "ENCODED";

    if (!parse_int32_str(numberText, &value))
    {
        value = 123456;
    }

    if (!parse_int32_str(lengthText, &fixedLength) || fixedLength < 1)
    {
        fixedLength = 12;
    }
    if (fixedLength > 32)
    {
        fixedLength = 32;
    }

    encodeResult = eo_encode_number(value, encodedNumber);
    while (encodedLen > 1 && encodedNumber[encodedLen - 1] == EO_NUMBER_PADDING)
    {
        --encodedLen;
    }
    bytes_to_hex(encodedNumber, encodedLen, numberHex, sizeof(numberHex));
    if (encodeResult == EO_SUCCESS)
    {
        decodedValue = eo_decode_number(encodedNumber, encodedLen);
    }

    strncpy(stringWork, encodedSource, sizeof(stringWork) - 1);
    stringWork[sizeof(stringWork) - 1] = '\0';
    eo_encode_string((uint8_t *)stringWork, strlen(stringWork));
    bytes_to_hex((const uint8_t *)stringWork, strlen(stringWork), stringHex, sizeof(stringHex));
    eo_decode_string((uint8_t *)stringWork, strlen(stringWork));

    writer = eo_writer_init();
    eo_writer_set_string_sanitization_mode(&writer, 1);
    pipelineResult = eo_writer_add_byte(&writer, 0x7F);
    if (pipelineResult == EO_SUCCESS)
        pipelineResult = eo_writer_add_char(&writer, 120);
    if (pipelineResult == EO_SUCCESS)
        pipelineResult = eo_writer_add_short(&writer, 30000);
    if (pipelineResult == EO_SUCCESS)
        pipelineResult = eo_writer_add_three(&writer, 1000000);
    if (pipelineResult == EO_SUCCESS)
        pipelineResult = eo_writer_add_int(&writer, value);
    if (pipelineResult == EO_SUCCESS)
        pipelineResult = eo_writer_add_fixed_string(&writer, rawSource, (size_t)fixedLength, 1);
    if (pipelineResult == EO_SUCCESS)
        pipelineResult = eo_writer_add_fixed_encoded_string(&writer, encodedSource, (size_t)fixedLength, 1);

    bytes_to_hex(writer.data, writer.length, writerHex, sizeof(writerHex));

    if (pipelineResult == EO_SUCCESS)
    {
        reader = eo_reader_init(writer.data, writer.length);
        pipelineResult = eo_reader_get_byte(&reader, &byteValue);
        if (pipelineResult == EO_SUCCESS)
            pipelineResult = eo_reader_get_char(&reader, &charValue);
        if (pipelineResult == EO_SUCCESS)
            pipelineResult = eo_reader_get_short(&reader, &shortValue);
        if (pipelineResult == EO_SUCCESS)
            pipelineResult = eo_reader_get_three(&reader, &threeValue);
        if (pipelineResult == EO_SUCCESS)
            pipelineResult = eo_reader_get_int(&reader, &intValue);
        if (pipelineResult == EO_SUCCESS)
            pipelineResult = eo_reader_get_fixed_string(&reader, (size_t)fixedLength, &fixedRaw);
        if (pipelineResult == EO_SUCCESS)
            pipelineResult = eo_reader_get_fixed_encoded_string(&reader, (size_t)fixedLength, &fixedEncoded);
    }

    snprintf(
        out,
        outSize,
        "Data Lab\r\n"
        "========\r\n"
        "Input number             : %ld\r\n"
        "Encoded number bytes     : %s (%s)\r\n"
        "Decoded number           : %ld\r\n"
        "EO string bytes          : %s\r\n"
        "EO string roundtrip      : %s\r\n\r\n"
        "Writer sanitization      : %s\r\n"
        "Pipeline result          : %s\r\n"
        "Serialized buffer        : %s\r\n"
        "Read byte                : 0x%02X\r\n"
        "Read char                : %ld\r\n"
        "Read short               : %ld\r\n"
        "Read three               : %ld\r\n"
        "Read int                 : %ld\r\n"
        "Read fixed string        : %s\r\n"
        "Read fixed encoded       : %s\r\n",
        (long)value,
        numberHex,
        eo_result_string(encodeResult),
        (long)decodedValue,
        stringHex,
        stringWork,
        eo_writer_get_string_sanitization_mode(&writer) ? "enabled" : "disabled",
        eo_result_string(pipelineResult),
        writer.length ? writerHex : "(none)",
        (unsigned int)byteValue,
        (long)charValue,
        (long)shortValue,
        (long)threeValue,
        (long)intValue,
        fixedRaw ? fixedRaw : "(n/a)",
        fixedEncoded ? fixedEncoded : "(n/a)");

    if (fixedRaw)
    {
        free(fixedRaw);
    }
    if (fixedEncoded)
    {
        free(fixedEncoded);
    }

    eo_writer_free(&writer);
}

static void build_encrypt_report(const char *modeText, const char *multipleText, const char *bytesText, char *out, size_t outSize)
{
    uint8_t data[256];
    uint8_t original[256];
    size_t length = 0;
    int32_t multipleValue;
    uint8_t multiple;
    int decryptMode = 0;
    const char *mode = (modeText && modeText[0]) ? modeText : "encrypt";
    char beforeHex[1024];
    char firstPassHex[1024];
    char secondPassHex[1024];

    if (tolower((unsigned char)mode[0]) == 'd')
    {
        decryptMode = 1;
    }

    if (!parse_hex_bytes(bytesText, data, sizeof(data), &length) || length == 0)
    {
        uint8_t fallback[] = {0x8A, 0x12, 0x18, 0x1E, 0x24, 0x2A, 0x30, 0x36};
        memcpy(data, fallback, sizeof(fallback));
        length = sizeof(fallback);
    }

    memcpy(original, data, length);

    if (parse_int32_str(multipleText, &multipleValue) && multipleValue >= 1 && multipleValue <= 255)
    {
        multiple = (uint8_t)multipleValue;
    }
    else
    {
        multiple = eo_generate_swap_multiple();
    }

    bytes_to_hex(original, length, beforeHex, sizeof(beforeHex));
    if (decryptMode)
    {
        eo_decrypt_packet(data, length, multiple);
        bytes_to_hex(data, length, firstPassHex, sizeof(firstPassHex));
        eo_encrypt_packet(data, length, multiple);
        bytes_to_hex(data, length, secondPassHex, sizeof(secondPassHex));

        snprintf(
            out,
            outSize,
            "Encryption\r\n"
            "==========\r\n"
            "Mode                     : decrypt\r\n"
            "Swap multiple            : %u\r\n"
            "Input bytes              : %s\r\n"
            "Decrypted bytes          : %s\r\n"
            "Re-encrypted bytes       : %s\r\n"
            "Roundtrip match          : %s\r\n",
            (unsigned int)multiple,
            beforeHex,
            firstPassHex,
            secondPassHex,
            (memcmp(original, data, length) == 0) ? "yes" : "no");
    }
    else
    {
        eo_encrypt_packet(data, length, multiple);
        bytes_to_hex(data, length, firstPassHex, sizeof(firstPassHex));
        eo_decrypt_packet(data, length, multiple);
        bytes_to_hex(data, length, secondPassHex, sizeof(secondPassHex));

        snprintf(
            out,
            outSize,
            "Encryption\r\n"
            "==========\r\n"
            "Mode                     : encrypt\r\n"
            "Swap multiple            : %u\r\n"
            "Input bytes              : %s\r\n"
            "Encrypted bytes          : %s\r\n"
            "Decrypted bytes          : %s\r\n"
            "Roundtrip match          : %s\r\n",
            (unsigned int)multiple,
            beforeHex,
            firstPassHex,
            secondPassHex,
            (memcmp(original, data, length) == 0) ? "yes" : "no");
    }
}

static void build_packet_capture_report(size_t index, uint8_t swapMultiple, char *out, size_t outSize)
{
    const PacketCaptureExample *example;
    const char *familyName;
    const char *actionName;
    const char *sideName;
    const char *description;
    const uint8_t *data;
    uint8_t *encrypted;
    size_t length;
    char dataHex[6144];
    char encryptedHex[6144];
    char decryptedHex[6144];
    int roundtripMatch;

    if (index >= sizeof(g_packet_capture_examples) / sizeof(g_packet_capture_examples[0]))
    {
        snprintf(out, outSize, "Select a packet from the list to inspect details.\r\n");
        return;
    }

    example = &g_packet_capture_examples[index];
    sideName = (example->side == 0) ? "client" : "server";
    familyName = find_packet_name(g_packet_families, sizeof(g_packet_families) / sizeof(g_packet_families[0]), example->family);
    actionName = find_packet_name(g_packet_actions, sizeof(g_packet_actions) / sizeof(g_packet_actions[0]), example->action);
    description = (example->description && example->description[0]) ? example->description : "(none)";
    data = example->payload;
    length = example->payloadLength;

    if (!out || outSize == 0)
    {
        return;
    }

    if (!data || length == 0)
    {
        snprintf(out, outSize, "Packet Browser\r\n==============\r\nNo payload bytes for %s\r\n", example->label);
        return;
    }

    encrypted = (uint8_t *)malloc(length);
    if (!encrypted)
    {
        snprintf(out, outSize, "Packet Browser\r\n==============\r\nOut of memory for %s\r\n", example->label);
        return;
    }

    memcpy(encrypted, data, length);
    bytes_to_hex_preview(data, length, 512, dataHex, sizeof(dataHex));
    eo_encrypt_packet(encrypted, length, swapMultiple);
    bytes_to_hex_preview(encrypted, length, 512, encryptedHex, sizeof(encryptedHex));
    eo_decrypt_packet(encrypted, length, swapMultiple);
    bytes_to_hex_preview(encrypted, length, 512, decryptedHex, sizeof(decryptedHex));
    roundtripMatch = (memcmp(data, encrypted, length) == 0);

    snprintf(
        out,
        outSize,
        "Packet Browser\r\n"
        "==============\r\n"
        "Packet                  : %s\r\n"
        "Side                    : %s\r\n"
        "Description             : %s\r\n"
        "Capture source          : %s\r\n"
        "Family                  : %d -> %s\r\n"
        "Action                  : %d -> %s\r\n"
        "Payload bytes           : %lu\r\n"
        "Swap multiple           : %u\r\n\r\n"
        "Roundtrip match         : %s\r\n"
        "Note: capture payload omits protocol header/length/sequence fields.\r\n"
        "Hex output: showing up to first 512 bytes per row.\r\n\r\n"
        "Original payload        : %s\r\n"
        "Encrypted payload       : %s\r\n"
        "Decrypted payload       : %s\r\n",
        example->label,
        sideName,
        description,
        example->source,
        example->family,
        familyName ? familyName : "(unknown)",
        example->action,
        actionName ? actionName : "(unknown)",
        (unsigned long)length,
        (unsigned int)swapMultiple,
        roundtripMatch ? "yes" : "no",
        dataHex,
        encryptedHex,
        decryptedHex);

    free(encrypted);
}

static int get_packet_browser_swap_multiple(ModuleWindowState *state, uint8_t *outSwapMultiple)
{
    char text[32];
    int32_t value;

    if (!state || !state->hwndEdits[0] || !outSwapMultiple)
    {
        return 0;
    }

    get_text(state->hwndEdits[0], text, sizeof(text));
    if (!parse_int32_str(text, &value) || value < 6 || value > 12)
    {
        set_text(state->hwndLog, "Swap multiple must be an integer between 6 and 12.\r\n");
        return 0;
    }

    *outSwapMultiple = (uint8_t)value;
    return 1;
}

static const char *get_encrypt_mode(ModuleWindowState *state)
{
    if (!state)
    {
        return "encrypt";
    }

    if (state->hwndModeDecrypt && SendMessageA(state->hwndModeDecrypt, BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        return "decrypt";
    }

    return "encrypt";
}

static size_t get_selected_packet_index(ModuleWindowState *state)
{
    LRESULT selection;

    if (!state || !state->hwndPacketList)
    {
        return (size_t)-1;
    }

    selection = SendMessageA(state->hwndPacketList, LB_GETCURSEL, 0, 0);
    if (selection == LB_ERR)
    {
        return (size_t)-1;
    }

    return (size_t)selection;
}

static void update_packet_browser_from_selection(ModuleWindowState *state)
{
    char output[8192];
    size_t index = get_selected_packet_index(state);
    uint8_t swapMultiple;

    if (index == (size_t)-1)
    {
        set_text(state->hwndLog, "Select a packet from the list to inspect details.\r\n");
        return;
    }

    if (!get_packet_browser_swap_multiple(state, &swapMultiple))
    {
        return;
    }

    if (is_rich_edit_control(state->hwndLog))
    {
        render_packet_capture_report_rich(state, index, swapMultiple);
        return;
    }

    build_packet_capture_report(index, swapMultiple, output, sizeof(output));
    set_text(state->hwndLog, output);
}

static void build_text_report(const char *textValue, char *out, size_t outSize)
{
    char working[256];
    char eoHex[1024];
    char *converted = NULL;
    char convertedHex[1024];
    EoResult convertResult;
    const char *source = (textValue && textValue[0]) ? textValue : "Cafe - naive text";

    strncpy(working, source, sizeof(working) - 1);
    working[sizeof(working) - 1] = '\0';

    eo_encode_string((uint8_t *)working, strlen(working));
    bytes_to_hex((const uint8_t *)working, strlen(working), eoHex, sizeof(eoHex));
    eo_decode_string((uint8_t *)working, strlen(working));

    convertResult = eo_string_to_windows_1252(source, &converted);
    if (convertResult == EO_SUCCESS && converted)
    {
        bytes_to_hex((const uint8_t *)converted, strlen(converted), convertedHex, sizeof(convertedHex));
    }
    else
    {
        strcpy(convertedHex, "(none)");
    }

    snprintf(
        out,
        outSize,
        "Text Conversion\r\n"
        "===============\r\n"
        "Input text                : %s\r\n"
        "Windows-1252 result       : %s\r\n"
        "Windows-1252 bytes        : %s\r\n"
        "EO transform bytes        : %s\r\n"
        "EO roundtrip text         : %s\r\n",
        source,
        eo_result_string(convertResult),
        convertedHex,
        eoHex,
        working);

    if (converted)
    {
        free(converted);
    }
}

static void run_module_demo(ModuleWindowState *state)
{
    char inputs[MAX_FIELDS][256];
    char output[8192];
    int i;
    const char *encryptMode = NULL;

    for (i = 0; i < MAX_FIELDS; ++i)
    {
        inputs[i][0] = '\0';
        if (state->hwndEdits[i])
        {
            get_text(state->hwndEdits[i], inputs[i], sizeof(inputs[i]));
        }
    }

    switch (state->module)
    {
    case MODULE_RNG:
        build_rng_report(state, inputs[0], inputs[1], inputs[2], output, sizeof(output));
        break;
    case MODULE_SEQUENCER:
        build_sequencer_report(inputs[0], inputs[1], output, sizeof(output));
        break;
    case MODULE_DATA:
        build_data_report(inputs[0], inputs[1], inputs[2], inputs[3], output, sizeof(output));
        break;
    case MODULE_ENCRYPT:
        encryptMode = get_encrypt_mode(state);
        build_encrypt_report(encryptMode, inputs[1], inputs[2], output, sizeof(output));
        break;
    case MODULE_PACKET_BROWSER:
        update_packet_browser_from_selection(state);
        return;
    case MODULE_TEXT:
        build_text_report(inputs[0], output, sizeof(output));
        break;
    default:
        strcpy(output, "Unknown module.\r\n");
        break;
    }

    set_text(state->hwndLog, output);
}

static void populate_packet_browser_list(ModuleWindowState *state)
{
    size_t i;

    if (!state || !state->hwndPacketList)
    {
        return;
    }

    SendMessageA(state->hwndPacketList, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < sizeof(g_packet_capture_examples) / sizeof(g_packet_capture_examples[0]); ++i)
    {
        SendMessageA(state->hwndPacketList, LB_ADDSTRING, 0, (LPARAM)g_packet_capture_examples[i].label);
    }
}

static void reset_module_inputs(ModuleWindowState *state)
{
    const ModuleDefinition *def = get_module_definition(state->module);
    int i;

    if (!def)
    {
        return;
    }

    for (i = 0; i < def->fieldCount; ++i)
    {
        if (state->hwndEdits[i])
        {
            set_text(state->hwndEdits[i], def->fieldDefaults[i]);
        }
    }

    if (state->module == MODULE_ENCRYPT)
    {
        if (state->hwndModeEncrypt)
        {
            SendMessageA(state->hwndModeEncrypt, BM_SETCHECK, BST_CHECKED, 0);
        }
        if (state->hwndModeDecrypt)
        {
            SendMessageA(state->hwndModeDecrypt, BM_SETCHECK, BST_UNCHECKED, 0);
        }
    }

    if (state->module == MODULE_PACKET_BROWSER && state->hwndPacketList)
    {
        SendMessageA(state->hwndPacketList, LB_SETCURSEL, 0, 0);
        update_packet_browser_from_selection(state);
        return;
    }

    if (state->module == MODULE_RNG)
    {
        state->rngSeedInitialized = 0;
        state->rngSeedIsManual = 0;
        state->rngCurrentSeed = 0;
    }

    set_text(
        state->hwndLog,
        "Ready. Adjust the labeled inputs and click Run Demo.\r\n"
        "Each module window is independent, so you can leave several open side by side.\r\n");
}

static int measure_text_width(HWND hwnd, HFONT font, const char *text)
{
    HDC hdc;
    HFONT oldFont;
    SIZE size;

    if (!hwnd || !text)
    {
        return 0;
    }

    hdc = GetDC(hwnd);
    if (!hdc)
    {
        return 0;
    }

    oldFont = (HFONT)SelectObject(hdc, font ? font : (HFONT)GetStockObject(DEFAULT_GUI_FONT));
    GetTextExtentPoint32A(hdc, text, (int)strlen(text), &size);
    SelectObject(hdc, oldFont);
    ReleaseDC(hwnd, hdc);

    return size.cx;
}

static int measure_wrapped_text_height(HWND hwnd, HFONT font, const char *text, int width)
{
    HDC hdc;
    HFONT oldFont;
    RECT rc;
    int height;

    if (!hwnd || !text || width <= 0)
    {
        return 20;
    }

    hdc = GetDC(hwnd);
    if (!hdc)
    {
        return 20;
    }

    oldFont = (HFONT)SelectObject(hdc, font ? font : (HFONT)GetStockObject(DEFAULT_GUI_FONT));
    rc.left = 0;
    rc.top = 0;
    rc.right = width;
    rc.bottom = 0;
    DrawTextA(hdc, text, -1, &rc, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
    SelectObject(hdc, oldFont);
    ReleaseDC(hwnd, hdc);

    height = rc.bottom - rc.top;
    return (height < 20) ? 20 : height;
}

static void layout_module_window(HWND hwnd, ModuleWindowState *state)
{
    RECT rc;
    int width;
    int height;
    int margin = 18;
    int top;
    int labelWidth = 150;
    int rowHeight = 24;
    int rowGap = 34;
    int buttonTop;
    int logTop;
    int descHeight;
    int contentWidth;
    int compactMode;
    int editWidth;
    int i;
    const ModuleDefinition *def = get_module_definition(state->module);

    GetClientRect(hwnd, &rc);
    width = rc.right - rc.left;
    height = rc.bottom - rc.top;
    contentWidth = width - margin * 2;
    compactMode = contentWidth < 560;

    if (def)
    {
        for (i = 0; i < def->fieldCount; ++i)
        {
            int measured = measure_text_width(hwnd, state->labelFont, def->fieldLabels[i]);
            if (measured + 24 > labelWidth)
            {
                labelWidth = measured + 24;
            }
        }
    }

    if (labelWidth > 260)
    {
        labelWidth = 260;
    }

    if (compactMode)
    {
        labelWidth = contentWidth;
        rowGap = 58;
    }

    descHeight = measure_wrapped_text_height(hwnd, state->bodyFont, def ? def->description : "", contentWidth);
    if (state->hwndDesc)
    {
        MoveWindow(state->hwndDesc, margin, 84, contentWidth, descHeight, TRUE);
    }
    top = 84 + descHeight + 16;
    editWidth = compactMode ? contentWidth : (contentWidth - labelWidth);
    if (editWidth < 180)
    {
        editWidth = 180;
    }

    for (i = 0; def && i < def->fieldCount; ++i)
    {
        int rowY = top + i * rowGap;
        if (compactMode)
        {
            MoveWindow(state->hwndLabels[i], margin, rowY, labelWidth, 18, TRUE);
            if (state->module == MODULE_ENCRYPT && i == 0)
            {
                if (state->hwndModeEncrypt)
                {
                    MoveWindow(state->hwndModeEncrypt, margin, rowY + 20, 90, rowHeight, TRUE);
                }
                if (state->hwndModeDecrypt)
                {
                    MoveWindow(state->hwndModeDecrypt, margin + 104, rowY + 20, 90, rowHeight, TRUE);
                }
            }
            else
            {
                MoveWindow(state->hwndEdits[i], margin, rowY + 20, editWidth, rowHeight, TRUE);
            }
        }
        else
        {
            MoveWindow(state->hwndLabels[i], margin, rowY + 4, labelWidth - 10, 20, TRUE);
            if (state->module == MODULE_ENCRYPT && i == 0)
            {
                if (state->hwndModeEncrypt)
                {
                    MoveWindow(state->hwndModeEncrypt, margin + labelWidth, rowY, 90, rowHeight, TRUE);
                }
                if (state->hwndModeDecrypt)
                {
                    MoveWindow(state->hwndModeDecrypt, margin + labelWidth + 104, rowY, 90, rowHeight, TRUE);
                }
            }
            else
            {
                MoveWindow(state->hwndEdits[i], margin + labelWidth, rowY, editWidth, rowHeight, TRUE);
            }
        }

        if (state->module == MODULE_PACKET_BROWSER && i == 0)
        {
            int smallEditWidth = 64;
            int editX = margin;
            int editY = rowY + 20;

            if (!compactMode)
            {
                editX = margin + labelWidth;
                editY = rowY;
            }

            MoveWindow(state->hwndEdits[i], editX, editY, smallEditWidth, rowHeight, TRUE);
            if (state->hwndPacketRandomSwap)
            {
                MoveWindow(state->hwndPacketRandomSwap, editX + smallEditWidth + 8, editY, 90, 30, TRUE);
            }
        }
    }

    buttonTop = top + ((def ? def->fieldCount : 0) * rowGap) + (compactMode ? 0 : 8);
    if (state->module == MODULE_PACKET_BROWSER)
    {
        MoveWindow(state->hwndClose, width - margin - 100, buttonTop, 100, 30, TRUE);
        logTop = buttonTop + 42;
    }
    else if (contentWidth >= 420)
    {
        MoveWindow(state->hwndRun, margin, buttonTop, 120, 30, TRUE);
        MoveWindow(state->hwndReset, margin + 132, buttonTop, 120, 30, TRUE);
        MoveWindow(state->hwndClose, width - margin - 100, buttonTop, 100, 30, TRUE);
        logTop = buttonTop + 46;
    }
    else
    {
        MoveWindow(state->hwndRun, margin, buttonTop, contentWidth, 28, TRUE);
        MoveWindow(state->hwndReset, margin, buttonTop + 34, contentWidth, 28, TRUE);
        MoveWindow(state->hwndClose, margin, buttonTop + 68, contentWidth, 28, TRUE);
        logTop = buttonTop + 108;
    }

    if (state->module == MODULE_PACKET_BROWSER && state->hwndPacketList)
    {
        int listWidth;
        if (contentWidth >= 620)
        {
            listWidth = 260;
            MoveWindow(state->hwndPacketList, margin, logTop, listWidth, height - logTop - margin, TRUE);
            MoveWindow(state->hwndLog, margin + listWidth + 12, logTop, contentWidth - listWidth - 12, height - logTop - margin, TRUE);
        }
        else
        {
            int listHeight = 140;
            MoveWindow(state->hwndPacketList, margin, logTop, contentWidth, listHeight, TRUE);
            MoveWindow(state->hwndLog, margin, logTop + listHeight + 10, contentWidth, height - (logTop + listHeight + 10) - margin, TRUE);
        }
    }
    else
    {
        MoveWindow(state->hwndLog, margin, logTop, contentWidth, height - logTop - margin, TRUE);
    }
}

static void apply_font_to_children(HWND hwnd, HFONT font)
{
    HWND child = GetWindow(hwnd, GW_CHILD);
    while (child)
    {
        SendMessageA(child, WM_SETFONT, (WPARAM)font, TRUE);
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

static int is_scrollable_control(HWND hwnd)
{
    char className[32];
    LONG style;

    if (!hwnd)
    {
        return 0;
    }

    style = GetWindowLongA(hwnd, GWL_STYLE);
    if ((style & WS_VSCROLL) || (style & WS_HSCROLL))
    {
        return 1;
    }

    className[0] = '\0';
    GetClassNameA(hwnd, className, (int)sizeof(className));

    if (lstrcmpiA(className, "Edit") == 0 && (style & ES_MULTILINE))
    {
        return 1;
    }

    if (lstrcmpiA(className, "ListBox") == 0)
    {
        return 1;
    }

    return 0;
}

static int forward_wheel_to_scrollable_child(HWND parentHwnd, WPARAM wParam, LPARAM lParam)
{
    POINT screenPt;
    HWND hit;
    HWND cursorTarget;
    HWND focusTarget;

    screenPt.x = (SHORT)LOWORD(lParam);
    screenPt.y = (SHORT)HIWORD(lParam);
    hit = WindowFromPoint(screenPt);

    cursorTarget = hit;
    while (cursorTarget && cursorTarget != parentHwnd)
    {
        if (GetParent(cursorTarget) == parentHwnd)
        {
            break;
        }
        cursorTarget = GetParent(cursorTarget);
    }

    if (cursorTarget && cursorTarget != parentHwnd && is_scrollable_control(cursorTarget))
    {
        SendMessageA(cursorTarget, WM_MOUSEWHEEL, wParam, lParam);
        return 1;
    }

    focusTarget = GetFocus();
    if (focusTarget)
    {
        HWND walker = focusTarget;
        while (walker && walker != parentHwnd)
        {
            if (GetParent(walker) == parentHwnd)
            {
                break;
            }
            walker = GetParent(walker);
        }

        if (walker && walker != parentHwnd && is_scrollable_control(walker))
        {
            SendMessageA(walker, WM_MOUSEWHEEL, wParam, lParam);
            return 1;
        }
    }

    return 0;
}

static void draw_main_header(HWND hwnd, HDC hdc, MainState *state)
{
    RECT rc;
    RECT header;
    HBRUSH headerBrush;
    HFONT oldFont;

    GetClientRect(hwnd, &rc);
    header = rc;
    header.bottom = 112;

    headerBrush = CreateSolidBrush(COLOR_MAIN_HEADER);
    FillRect(hdc, &header, headerBrush);
    DeleteObject(headerBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(252, 246, 236));
    oldFont = (HFONT)SelectObject(hdc, state->titleFont);
    TextOutA(hdc, 22, 18, "EOLib Demo Station", (int)strlen("EOLib Demo Station"));
    SelectObject(hdc, state->subtitleFont);
    TextOutA(hdc, 22, 62, "Classic Endless Online protocol tools, client math, and packet experiments in one launcher.",
             (int)strlen("Classic Endless Online protocol tools, client math, and packet experiments in one launcher."));
    SelectObject(hdc, oldFont);
}

static void draw_module_header(HWND hwnd, HDC hdc, ModuleWindowState *state)
{
    RECT rc;
    RECT header;
    RECT accent;
    HBRUSH headerBrush;
    HBRUSH accentBrush;
    HFONT oldFont;
    const ModuleDefinition *def = get_module_definition(state->module);
    COLORREF color = module_color(state->module);

    GetClientRect(hwnd, &rc);
    header = rc;
    header.bottom = 72;
    accent.left = 0;
    accent.top = 72;
    accent.right = rc.right;
    accent.bottom = 78;

    headerBrush = CreateSolidBrush(color);
    accentBrush = CreateSolidBrush(RGB(255, 244, 214));
    FillRect(hdc, &header, headerBrush);
    FillRect(hdc, &accent, accentBrush);
    DeleteObject(headerBrush);
    DeleteObject(accentBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(250, 248, 243));
    oldFont = (HFONT)SelectObject(hdc, state->titleFont);
    TextOutA(hdc, 18, 14, def->title, (int)strlen(def->title));
    SelectObject(hdc, state->subtitleFont);
    TextOutA(hdc, 18, 44, def->subtitle, (int)strlen(def->subtitle));
    SelectObject(hdc, oldFont);
}

static void draw_owner_button(const DRAWITEMSTRUCT *dis, MainState *state)
{
    DemoModule module;
    const ModuleDefinition *def;
    RECT rc;
    HBRUSH fillBrush;
    COLORREF fillColor;
    COLORREF textColor;
    HFONT oldFont;

    if (!dis || !state)
    {
        return;
    }

    module = (DemoModule)(dis->CtlID - ID_MAIN_BUTTON_BASE);
    def = get_module_definition(module);
    if (!def)
    {
        return;
    }

    rc = dis->rcItem;
    fillColor = module_color(module);
    textColor = RGB(252, 248, 240);

    if (dis->itemState & ODS_SELECTED)
    {
        rc.left += 1;
        rc.top += 1;
        fillColor = RGB(
            (GetRValue(fillColor) > 20) ? GetRValue(fillColor) - 20 : GetRValue(fillColor),
            (GetGValue(fillColor) > 20) ? GetGValue(fillColor) - 20 : GetGValue(fillColor),
            (GetBValue(fillColor) > 20) ? GetBValue(fillColor) - 20 : GetBValue(fillColor));
    }

    fillBrush = CreateSolidBrush(fillColor);
    FillRect(dis->hDC, &rc, fillBrush);
    DeleteObject(fillBrush);

    FrameRect(dis->hDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, textColor);
    oldFont = (HFONT)SelectObject(dis->hDC, state->cardTitleFont);
    DrawTextA(dis->hDC, def->cardTitle, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dis->hDC, oldFont);

    if (dis->itemState & ODS_FOCUS)
    {
        RECT focusRect = rc;
        InflateRect(&focusRect, -3, -3);
        DrawFocusRect(dis->hDC, &focusRect);
    }
}

static void open_module_window(MainState *mainState, DemoModule module)
{
    ModuleWindowState *moduleState;
    const ModuleDefinition *def = get_module_definition(module);
    char windowTitle[128];
    HWND hwnd;

    if (!mainState || !def)
    {
        return;
    }

    if (mainState->moduleWindows[module] && IsWindow(mainState->moduleWindows[module]))
    {
        ShowWindow(mainState->moduleWindows[module], SW_SHOW);
        SetForegroundWindow(mainState->moduleWindows[module]);
        return;
    }

    moduleState = (ModuleWindowState *)calloc(1, sizeof(ModuleWindowState));
    if (!moduleState)
    {
        return;
    }

    moduleState->owner = mainState;
    moduleState->module = module;
    snprintf(windowTitle, sizeof(windowTitle), "%s - EOLib Demo", def->title);

    hwnd = CreateWindowA(
        MODULE_CLASS_NAME,
        windowTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        760,
        620,
        NULL,
        NULL,
        GetModuleHandleA(NULL),
        moduleState);

    if (!hwnd)
    {
        free(moduleState);
        return;
    }

    mainState->moduleWindows[module] = hwnd;
}

static LRESULT CALLBACK module_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ModuleWindowState *state = (ModuleWindowState *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
        ModuleWindowState *createdState = (ModuleWindowState *)cs->lpCreateParams;
        const ModuleDefinition *def;
        int i;

        if (!createdState)
        {
            return -1;
        }

        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)createdState);
        state = createdState;
        def = get_module_definition(state->module);
        if (!def)
        {
            return -1;
        }

        state->bgBrush = CreateSolidBrush(COLOR_MAIN_BG);
        state->panelBrush = CreateSolidBrush(COLOR_MAIN_PANEL);
        state->titleFont = CreateFontA(-24, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                       CLIP_DEFAULT_PRECIS, XP_FONT_QUALITY, DEFAULT_PITCH | FF_ROMAN, "Georgia");
        state->subtitleFont = CreateFontA(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, XP_FONT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
        state->bodyFont = CreateFontA(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, XP_FONT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
        state->labelFont = CreateFontA(-15, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                       CLIP_DEFAULT_PRECIS, XP_FONT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
        state->monoFont = CreateFontA(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, XP_FONT_QUALITY, FIXED_PITCH | FF_MODERN, "Courier New");

        if (state->module != MODULE_PACKET_BROWSER)
        {
            state->hwndDesc = CreateWindowA("STATIC", def->description, WS_CHILD | WS_VISIBLE,
                                            18, 84, 680, 34, hwnd, (HMENU)ID_MODULE_DESC, NULL, NULL);
        }

        for (i = 0; i < def->fieldCount; ++i)
        {
            state->hwndLabels[i] = CreateWindowA("STATIC", def->fieldLabels[i], WS_CHILD | WS_VISIBLE,
                                                 18, 110 + i * 34, 140, 20, hwnd, (HMENU)(ID_MODULE_FIRST_LABEL + i), NULL, NULL);
            if (state->module == MODULE_ENCRYPT && i == 0)
            {
                state->hwndModeEncrypt = CreateWindowA(
                    "BUTTON",
                    "Encrypt",
                    WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON,
                    170,
                    106 + i * 34,
                    90,
                    24,
                    hwnd,
                    (HMENU)ID_MODULE_MODE_ENCRYPT,
                    NULL,
                    NULL);
                state->hwndModeDecrypt = CreateWindowA(
                    "BUTTON",
                    "Decrypt",
                    WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                    274,
                    106 + i * 34,
                    90,
                    24,
                    hwnd,
                    (HMENU)ID_MODULE_MODE_DECRYPT,
                    NULL,
                    NULL);
            }
            else
            {
                state->hwndEdits[i] = CreateWindowA("EDIT", def->fieldDefaults[i], WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                                    170, 106 + i * 34, 400, 24, hwnd, (HMENU)(ID_MODULE_FIRST_EDIT + i), NULL, NULL);
            }
        }

        if (state->module == MODULE_PACKET_BROWSER)
        {
            state->hwndPacketRandomSwap = CreateWindowA("BUTTON", "Random", WS_CHILD | WS_VISIBLE,
                                                        18, 250, 120, 30, hwnd, (HMENU)ID_MODULE_RANDOM_SWAP, NULL, NULL);
            state->hwndClose = CreateWindowA("BUTTON", "Close", WS_CHILD | WS_VISIBLE,
                                             610, 250, 100, 30, hwnd, (HMENU)ID_MODULE_CLOSE, NULL, NULL);
        }
        else
        {
            state->hwndRun = CreateWindowA("BUTTON", "Run Demo", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                           18, 250, 120, 30, hwnd, (HMENU)ID_MODULE_RUN, NULL, NULL);
            state->hwndReset = CreateWindowA("BUTTON", "Load Defaults", WS_CHILD | WS_VISIBLE,
                                             150, 250, 120, 30, hwnd, (HMENU)ID_MODULE_RESET, NULL, NULL);
            state->hwndClose = CreateWindowA("BUTTON", "Close", WS_CHILD | WS_VISIBLE,
                                             610, 250, 100, 30, hwnd, (HMENU)ID_MODULE_CLOSE, NULL, NULL);
        }
        if (state->module == MODULE_PACKET_BROWSER)
        {
            const char *logClass = "EDIT";
            if (!g_richedit_lib)
            {
                g_richedit_lib = LoadLibraryA("Riched20.dll");
            }
            if (g_richedit_lib)
            {
                logClass = RICHEDIT_CLASSA;
            }

            state->hwndLog = CreateWindowA(
                logClass,
                "Ready.\r\n",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                18,
                296,
                680,
                260,
                hwnd,
                (HMENU)ID_MODULE_LOG,
                NULL,
                NULL);
        }
        else
        {
            state->hwndLog = CreateWindowA(
                "EDIT",
                "Ready.\r\n",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                18,
                296,
                680,
                260,
                hwnd,
                (HMENU)ID_MODULE_LOG,
                NULL,
                NULL);
        }

        if (state->module == MODULE_PACKET_BROWSER)
        {
            state->hwndPacketList = CreateWindowA(
                "LISTBOX",
                "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                18,
                296,
                240,
                260,
                hwnd,
                (HMENU)ID_MODULE_PACKET_LIST,
                NULL,
                NULL);
            populate_packet_browser_list(state);
        }

        apply_font_to_children(hwnd, state->bodyFont);
        if (state->hwndDesc)
        {
            SendMessageA(state->hwndDesc, WM_SETFONT, (WPARAM)state->bodyFont, TRUE);
        }
        if (state->hwndRun)
        {
            SendMessageA(state->hwndRun, WM_SETFONT, (WPARAM)state->labelFont, TRUE);
        }
        if (state->hwndReset)
        {
            SendMessageA(state->hwndReset, WM_SETFONT, (WPARAM)state->bodyFont, TRUE);
        }
        if (state->hwndClose)
        {
            SendMessageA(state->hwndClose, WM_SETFONT, (WPARAM)state->bodyFont, TRUE);
        }
        if (state->hwndPacketRandomSwap)
        {
            SendMessageA(state->hwndPacketRandomSwap, WM_SETFONT, (WPARAM)state->bodyFont, TRUE);
        }
        if (state->hwndModeEncrypt)
        {
            SendMessageA(state->hwndModeEncrypt, WM_SETFONT, (WPARAM)state->bodyFont, TRUE);
        }
        if (state->hwndModeDecrypt)
        {
            SendMessageA(state->hwndModeDecrypt, WM_SETFONT, (WPARAM)state->bodyFont, TRUE);
        }
        SendMessageA(state->hwndLog, WM_SETFONT, (WPARAM)state->monoFont, TRUE);
        if (is_rich_edit_control(state->hwndLog))
        {
            SendMessageA(state->hwndLog, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
        }
        if (state->hwndPacketList)
        {
            SendMessageA(state->hwndPacketList, WM_SETFONT, (WPARAM)state->bodyFont, TRUE);
        }
        for (i = 0; i < def->fieldCount; ++i)
        {
            SendMessageA(state->hwndLabels[i], WM_SETFONT, (WPARAM)state->labelFont, TRUE);
            if (state->hwndEdits[i])
            {
                SendMessageA(state->hwndEdits[i], WM_SETFONT, (WPARAM)state->bodyFont, TRUE);
            }
        }

        layout_module_window(hwnd, state);
        reset_module_inputs(state);
        run_module_demo(state);
        return 0;
    }

    case WM_SIZE:
        if (state)
        {
            layout_module_window(hwnd, state);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO *mmi = (MINMAXINFO *)lParam;
        mmi->ptMinTrackSize.x = 470;
        mmi->ptMinTrackSize.y = 460;
        return 0;
    }

    case WM_COMMAND:
        if (!state)
        {
            return 0;
        }

        switch (LOWORD(wParam))
        {
        case ID_MODULE_RUN:
            run_module_demo(state);
            return 0;
        case ID_MODULE_RESET:
            reset_module_inputs(state);
            return 0;
        case ID_MODULE_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case ID_MODULE_RANDOM_SWAP:
            if (state->module == MODULE_PACKET_BROWSER && state->hwndEdits[0])
            {
                char valueText[16];
                uint8_t value = eo_generate_swap_multiple();
                snprintf(valueText, sizeof(valueText), "%u", (unsigned int)value);
                set_text(state->hwndEdits[0], valueText);
                update_packet_browser_from_selection(state);
            }
            return 0;
        case ID_MODULE_PACKET_LIST:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                update_packet_browser_from_selection(state);
            }
            return 0;
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (forward_wheel_to_scrollable_child(hwnd, wParam, lParam))
        {
            return 0;
        }
        break;

    case WM_CTLCOLORSTATIC:
        if (state)
        {
            HDC hdc = (HDC)wParam;
            HWND ctl = (HWND)lParam;
            if (ctl == state->hwndLog)
            {
                SetBkMode(hdc, OPAQUE);
                SetBkColor(hdc, RGB(255, 255, 255));
                SetTextColor(hdc, RGB(26, 26, 26));
                return (LRESULT)(HBRUSH)(COLOR_WINDOW + 1);
            }

            if (ctl == state->hwndDesc)
            {
                SetBkMode(hdc, OPAQUE);
                SetBkColor(hdc, COLOR_MAIN_PANEL);
                SetTextColor(hdc, COLOR_TEXT_DARK);
                return (LRESULT)(state->panelBrush ? state->panelBrush : state->bgBrush);
            }

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, COLOR_TEXT_DARK);
            return (LRESULT)(state->panelBrush ? state->panelBrush : state->bgBrush);
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        RECT rc;
        RECT panel;
        RECT descRect;
        HFONT oldFont;
        const ModuleDefinition *def = state ? get_module_definition(state->module) : NULL;
        HDC hdc = BeginPaint(hwnd, &ps);

        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, state ? state->bgBrush : (HBRUSH)(COLOR_BTNFACE + 1));

        panel = rc;
        panel.top = 78;
        panel.left = 10;
        panel.right -= 10;
        panel.bottom -= 10;
        if (state && state->panelBrush)
        {
            FillRect(hdc, &panel, state->panelBrush);
        }

        if (state)
        {
            draw_module_header(hwnd, hdc, state);

            if (state->module == MODULE_PACKET_BROWSER && def && def->description)
            {
                descRect.left = 18;
                descRect.top = 84;
                descRect.right = rc.right - 18;
                descRect.bottom = descRect.top + measure_wrapped_text_height(hwnd, state->bodyFont, def->description, rc.right - 36);

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, COLOR_TEXT_DARK);
                oldFont = (HFONT)SelectObject(hdc, state->bodyFont);
                DrawTextA(hdc, def->description, -1, &descRect, DT_LEFT | DT_WORDBREAK);
                SelectObject(hdc, oldFont);
            }
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        if (state && state->owner)
        {
            state->owner->moduleWindows[state->module] = NULL;
        }
        return 0;

    case WM_NCDESTROY:
        if (state)
        {
            if (state->titleFont)
                DeleteObject(state->titleFont);
            if (state->subtitleFont)
                DeleteObject(state->subtitleFont);
            if (state->bodyFont)
                DeleteObject(state->bodyFont);
            if (state->labelFont)
                DeleteObject(state->labelFont);
            if (state->monoFont)
                DeleteObject(state->monoFont);
            if (state->bgBrush)
                DeleteObject(state->bgBrush);
            if (state->panelBrush)
                DeleteObject(state->panelBrush);
            free(state);
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)0);
        }
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK main_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MainState *state = (MainState *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CREATE:
    {
        CREATESTRUCTA *cs = (CREATESTRUCTA *)lParam;
        MainState *createdState = (MainState *)cs->lpCreateParams;
        int i;

        if (!createdState)
        {
            return -1;
        }

        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)createdState);
        state = createdState;
        state->hwnd = hwnd;

        state->bgBrush = CreateSolidBrush(COLOR_MAIN_BG);
        state->titleFont = CreateFontA(-30, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                       CLIP_DEFAULT_PRECIS, XP_FONT_QUALITY, DEFAULT_PITCH | FF_ROMAN, "Georgia");
        state->subtitleFont = CreateFontA(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, XP_FONT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
        state->cardTitleFont = CreateFontA(-18, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                           CLIP_DEFAULT_PRECIS, XP_FONT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
        state->cardBodyFont = CreateFontA(-14, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, XP_FONT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");

        for (i = 0; i < MODULE_COUNT; ++i)
        {
            CreateWindowA(
                "BUTTON",
                g_modules[i].cardTitle,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                20,
                140,
                200,
                92,
                hwnd,
                (HMENU)(ID_MAIN_BUTTON_BASE + i),
                NULL,
                NULL);
        }
        return 0;
    }

    case WM_SIZE:
    {
        int width;
        int left;
        int top;
        int cardWidth;
        int cardHeight = 96;
        int columnGap = 18;
        int rowGap = 18;
        int i;

        if (!state)
        {
            return 0;
        }

        width = LOWORD(lParam);
        left = 20;
        top = 146;
        cardWidth = (width - left * 2 - columnGap) / 2;
        if (cardWidth < 240)
        {
            cardWidth = 240;
        }

        for (i = 0; i < MODULE_COUNT; ++i)
        {
            int col = i % 2;
            int row = i / 2;
            HWND button = GetDlgItem(hwnd, ID_MAIN_BUTTON_BASE + i);
            MoveWindow(button, left + col * (cardWidth + columnGap), top + row * (cardHeight + rowGap), cardWidth, cardHeight, TRUE);
        }
        return 0;
    }

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO *mmi = (MINMAXINFO *)lParam;
        mmi->ptMinTrackSize.x = 620;
        mmi->ptMinTrackSize.y = 560;
        return 0;
    }

    case WM_COMMAND:
        if (state && HIWORD(wParam) != BN_CLICKED)
        {
            return 0;
        }

        if (state && LOWORD(wParam) >= ID_MAIN_BUTTON_BASE && LOWORD(wParam) < ID_MAIN_BUTTON_BASE + MODULE_COUNT)
        {
            open_module_window(state, (DemoModule)(LOWORD(wParam) - ID_MAIN_BUTTON_BASE));
            return 0;
        }
        return 0;

    case WM_DRAWITEM:
        if (state)
        {
            const DRAWITEMSTRUCT *dis = (const DRAWITEMSTRUCT *)lParam;
            if (dis && dis->CtlID >= ID_MAIN_BUTTON_BASE && dis->CtlID < ID_MAIN_BUTTON_BASE + MODULE_COUNT)
            {
                draw_owner_button(dis, state);
                return TRUE;
            }
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        RECT rc;
        RECT noteRect;
        HFONT oldFont;
        HDC hdc = BeginPaint(hwnd, &ps);

        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, state ? state->bgBrush : (HBRUSH)(COLOR_BTNFACE + 1));

        if (state)
        {
            draw_main_header(hwnd, hdc, state);
        }

        noteRect.left = 20;
        noteRect.top = 118;
        noteRect.right = rc.right - 20;
        noteRect.bottom = 140;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, COLOR_TEXT_SOFT);
        oldFont = (HFONT)SelectObject(hdc, state ? state->subtitleFont : (HFONT)GetStockObject(DEFAULT_GUI_FONT));
        DrawTextA(
            hdc,
            "Each launcher button opens a dedicated tool window with descriptive inputs, defaults, and an isolated output log.",
            -1,
            &noteRect,
            DT_LEFT | DT_WORDBREAK);
        SelectObject(hdc, oldFont);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int main(void)
{
    WNDCLASSA mainClass;
    WNDCLASSA moduleClass;
    HINSTANCE instance = GetModuleHandleA(NULL);
    HWND hwnd;
    MSG msg;
    MainState state;

    ZeroMemory(&state, sizeof(state));
    ZeroMemory(&mainClass, sizeof(mainClass));
    ZeroMemory(&moduleClass, sizeof(moduleClass));

    mainClass.lpfnWndProc = main_window_proc;
    mainClass.hInstance = instance;
    mainClass.lpszClassName = APP_CLASS_NAME;
    mainClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    mainClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    moduleClass.lpfnWndProc = module_window_proc;
    moduleClass.hInstance = instance;
    moduleClass.lpszClassName = MODULE_CLASS_NAME;
    moduleClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    moduleClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    if (!RegisterClassA(&mainClass) || !RegisterClassA(&moduleClass))
    {
        fprintf(stderr, "Failed to register window classes.\n");
        return 1;
    }

    hwnd = CreateWindowA(
        APP_CLASS_NAME,
        APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        820,
        700,
        NULL,
        NULL,
        instance,
        &state);

    if (!hwnd)
    {
        fprintf(stderr, "Failed to create main window.\n");
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (state.titleFont)
        DeleteObject(state.titleFont);
    if (state.subtitleFont)
        DeleteObject(state.subtitleFont);
    if (state.cardTitleFont)
        DeleteObject(state.cardTitleFont);
    if (state.cardBodyFont)
        DeleteObject(state.cardBodyFont);
    if (state.bgBrush)
        DeleteObject(state.bgBrush);

    return (int)msg.wParam;
}

#else

int main(void)
{
    printf("This demo is intended to run as a Win32 application.\n");
    return 0;
}

#endif