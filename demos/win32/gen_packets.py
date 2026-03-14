import glob
import json
import os
import re
import base64
import xml.etree.ElementTree as ET

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(SCRIPT_DIR, "../../"))
OUT_PATH = os.path.join(SCRIPT_DIR, "packets.inc")


def norm_enum_name(text: str) -> str:
    text = text.replace("-", "_").replace(" ", "_")
    text = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", text)
    return text.upper()


FAMILY_VALUES = {
    "CONNECTION": 1,
    "ACCOUNT": 2,
    "CHARACTER": 3,
    "LOGIN": 4,
    "WELCOME": 5,
    "WALK": 6,
    "FACE": 7,
    "CHAIR": 8,
    "EMOTE": 9,
    "ATTACK": 11,
    "SPELL": 12,
    "SHOP": 13,
    "ITEM": 14,
    "STAT_SKILL": 16,
    "GLOBAL": 17,
    "TALK": 18,
    "WARP": 19,
    "JUKEBOX": 21,
    "PLAYERS": 22,
    "AVATAR": 23,
    "PARTY": 24,
    "REFRESH": 25,
    "NPC": 26,
    "PLAYER_RANGE": 27,
    "NPC_RANGE": 28,
    "RANGE": 29,
    "PAPERDOLL": 30,
    "EFFECT": 31,
    "TRADE": 32,
    "CHEST": 33,
    "DOOR": 34,
    "MESSAGE": 35,
    "BANK": 36,
    "LOCKER": 37,
    "BARBER": 38,
    "GUILD": 39,
    "MUSIC": 40,
    "SIT": 41,
    "RECOVER": 42,
    "BOARD": 43,
    "CAST": 44,
    "ARENA": 45,
    "PRIEST": 46,
    "MARRIAGE": 47,
    "ADMIN_INTERACT": 48,
    "CITIZEN": 49,
    "QUEST": 50,
    "BOOK": 51,
    "ERROR": 250,
    "INIT": 255,
}

ACTION_VALUES = {
    "REQUEST": 1,
    "ACCEPT": 2,
    "REPLY": 3,
    "REMOVE": 4,
    "AGREE": 5,
    "CREATE": 6,
    "ADD": 7,
    "PLAYER": 8,
    "TAKE": 9,
    "USE": 10,
    "BUY": 11,
    "SELL": 12,
    "OPEN": 13,
    "CLOSE": 14,
    "MSG": 15,
    "SPEC": 16,
    "ADMIN": 17,
    "LIST": 18,
    "TELL": 20,
    "REPORT": 21,
    "ANNOUNCE": 22,
    "SERVER": 23,
    "DROP": 24,
    "JUNK": 25,
    "OBTAIN": 26,
    "GET": 27,
    "KICK": 28,
    "RANK": 29,
    "TARGET_SELF": 30,
    "TARGET_OTHER": 31,
    "TARGET_GROUP": 33,
    "DIALOG": 34,
    "PING": 240,
    "PONG": 241,
    "NET242": 242,
    "NET243": 243,
    "NET244": 244,
    "ERROR": 250,
    "INIT": 255,
}


def c_escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')


def c_byte_rows(payload: bytes, per_line: int = 16) -> str:
    if not payload:
        return "    0x00"

    lines = []
    for i in range(0, len(payload), per_line):
        chunk = payload[i : i + per_line]
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk))

    return ",\n".join(lines)


def compact_text(text: str, max_len: int = 120) -> str:
    text = " ".join(text.strip().split())
    if len(text) <= max_len:
        return text
    return text[: max_len - 3].rstrip() + "..."


def load_packet_descriptions():
    descriptions = {}

    for side in ("client", "server"):
        path = os.path.join(ROOT, "eo-protocol", "xml", "net", side, "protocol.xml")
        if not os.path.exists(path):
            continue

        root = ET.parse(path).getroot()
        for packet in root.findall("packet"):
            family = packet.get("family")
            action = packet.get("action")
            comment = packet.find("comment")

            if not family or not action or comment is None or comment.text is None:
                continue

            desc = compact_text(comment.text)
            if not desc:
                continue

            key = (side, norm_enum_name(family), norm_enum_name(action))
            descriptions[key] = desc

    return descriptions


def collect_rows():
    descriptions = load_packet_descriptions()
    rows = []
    patterns = [
        os.path.join(ROOT, "eo-captured-packets", "client", "*.json"),
        os.path.join(ROOT, "eo-captured-packets", "server", "*.json"),
    ]
    for pattern in patterns:
        for path in sorted(glob.glob(pattern)):
            side_name = "client" if "/client/" in path else "server"
            side_value = 0 if side_name == "client" else 1
            with open(path, "r", encoding="utf-8") as fp:
                data = json.load(fp)

            family_name = norm_enum_name(data["family"])
            action_name = norm_enum_name(data["action"])
            family = FAMILY_VALUES[family_name]
            action = ACTION_VALUES[action_name]
            base = os.path.basename(path)[:-5]
            label = f"{base} ({side_name})"
            source = os.path.relpath(path, ROOT).replace("\\", "/")
            description = descriptions.get((side_name, family_name, action_name), base)
            expected = data["expected"]
            rows.append((family, side_value, action, label, source, description, expected))

    rows.sort(key=lambda r: (r[0], r[1], r[2], r[3]))
    return rows


def main():
    rows = collect_rows()
    with open(OUT_PATH, "w", encoding="ascii") as out:
        out.write("/* Generated from eo-captured-packets/client and eo-captured-packets/server */\n")
        out.write("/* Payload bytes are pre-decoded from JSON expected base64 values. */\n")
        for idx, (_, _, _, _, _, _, expected) in enumerate(rows):
            payload = base64.b64decode(expected, validate=True)
            out.write(f"static const uint8_t g_packet_payload_{idx}[] = {{\n")
            out.write(c_byte_rows(payload))
            out.write("\n};\n")

        out.write("static const PacketCaptureExample g_packet_capture_examples[] = {\n")
        for idx, (family, side, action, label, source, description, _) in enumerate(rows):
            out.write(
                f'    {{"{c_escape(label)}", {family}, {action}, {side}, "{c_escape(description)}", "{c_escape(source)}", g_packet_payload_{idx}, sizeof(g_packet_payload_{idx})}},\n'
            )
        out.write("};\n")

    print(f"wrote {len(rows)} entries to {OUT_PATH}")


if __name__ == "__main__":
    main()
