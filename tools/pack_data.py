#!/usr/bin/env python3
"""
pack_data.py — Elifoot II → Sega Genesis data packer
======================================================
Converts EQUIPAS.EF2 and TREINAD.EF2 into fixed-size binary files
for embedding in the Genesis ROM via rescomp BIN resources.

Usage:
    python3 pack_data.py <path/to/EQUIPAS.EF2> <path/to/TREINAD.EF2> <output_dir>

Example:
    python3 pack_data.py original/EQUIPAS.EF2 original/TREINAD.EF2 data/

Output files:
    <output_dir>/teams.bin    — team + player records
    <output_dir>/coaches.bin  — coach name records

Binary layouts (matching C structs in game/types.h):
----------------------------------------------------

teams.bin
  Header (4 bytes):
    uint16_t  team_count       — always 29
    uint16_t  players_per_team — always 16

  Per team (54 bytes × team_count):
    char[20]  name             — NUL-terminated, ASCII transliterated, max 19 chars
    uint8_t   n1               — copa pot / seeding value
    uint8_t   n2               — budget/strength factor
    uint8_t   nac[3]           — 3-char nationality code (e.g. "POR"), no NUL
    uint8_t   _pad             — padding to align players array to even offset

  Per player (24 bytes × players_per_team, immediately after each team header):
    char[16]  name             — NUL-terminated, ASCII transliterated, max 15 chars
    uint8_t   pos              — 0=GR 1=DF 2=MD 3=AV
    uint8_t   nac[3]           — 3-char nationality code, no NUL
    uint8_t   _pad[4]          — padding to 24 bytes

coaches.bin
  Header (2 bytes):
    uint16_t  coach_count      — always 50

  Per coach (20 bytes × coach_count):
    char[20]  name             — NUL-terminated, ASCII transliterated, max 19 chars

Total sizes (for reference):
  teams.bin:    4 + 29 × (54 + 16 × 24) = 4 + 29 × 438 = 12 706 bytes
  coaches.bin:  2 + 50 × 20             = 1 002 bytes
  Both fit comfortably in SRAM budget and ROM space.
"""

import os
import re
import struct
import sys
import unicodedata


# ---------------------------------------------------------------------------
# Constants — must match game/types.h exactly
# ---------------------------------------------------------------------------

TEAM_COUNT         = 29
PLAYERS_PER_TEAM   = 16
COACH_COUNT        = 50

TEAM_NAME_LEN      = 20   # includes NUL terminator
PLAYER_NAME_LEN    = 16   # includes NUL terminator
COACH_NAME_LEN     = 20   # includes NUL terminator
PLAYER_PAD         = 4    # padding bytes at end of PlayerRecord

# Position encoding
POS_MAP = {'gr': 0, 'df': 1, 'md': 2, 'av': 3}

# Struct formats (big-endian, matching m68k / Genesis native byte order)
# teams.bin header
TEAMS_HDR_FMT    = '>HH'          # team_count, players_per_team
TEAMS_HDR_SIZE   = struct.calcsize(TEAMS_HDR_FMT)   # 4

# TeamRecord: name[20] + n1 + n2 + nac[3] + _pad  = 26 bytes
# Immediately followed by 16 × PlayerRecord (no gap)
TEAM_REC_FMT     = f'>{TEAM_NAME_LEN}sBB3sB'
TEAM_REC_SIZE    = struct.calcsize(TEAM_REC_FMT)     # 26

# PlayerRecord: name[16] + pos + nac[3] + pad[4] = 24 bytes
PLAYER_REC_FMT   = f'>{PLAYER_NAME_LEN}sB3s{PLAYER_PAD}s'
PLAYER_REC_SIZE  = struct.calcsize(PLAYER_REC_FMT)   # 24

# coaches.bin header
COACHES_HDR_FMT  = '>H'
COACHES_HDR_SIZE = struct.calcsize(COACHES_HDR_FMT)  # 2

# CoachRecord: name[20]
COACH_REC_FMT    = f'>{COACH_NAME_LEN}s'
COACH_REC_SIZE   = struct.calcsize(COACH_REC_FMT)    # 20


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


# Manual substitution table for characters that survive NFD stripping
# but should map to a specific ASCII letter.
# Covers CP850 box-drawing chars that appear in the data due to encoding
# ambiguity (some lines were typed in Latin-1 / CP1252 context).
_MANUAL_SUBS: dict[str, str] = {
    # CP850 0xC1 = U+2534 BOX DRAWINGS LIGHT UP AND HORIZONTAL  → 'A' (Águas)
    '\u2534': 'A',
    # CP850 0xDA = U+250C BOX DRAWINGS LIGHT DOWN AND RIGHT     → 'U' (JaÚpor → JAÚPOR)
    '\u250c': 'U',
    # CP850 0xF4 = U+00B6 PILCROW SIGN — appears instead of 'ô' (Latin-1 confusion)
    '\u00b6': 'o',
    # CP850 0xE1 = U+00DF LATIN SMALL LETTER SHARP S — appears in some names
    '\u00df': 'ss',
    # Residual replacement character
    '\ufffd': '?',
}


def transliterate(s: str) -> str:
    """
    Convert a string (decoded from CP850 or Latin-1) to clean ASCII.

    Strategy:
      1. Apply manual substitution table for characters that NFD cannot handle
         (CP850 box-drawing chars mistakenly used for accented letters, pilcrow, etc.)
      2. NFD-decompose to separate base letters from combining accents.
      3. Drop all combining marks (Unicode category Mn).
      4. Encode to ASCII, replacing anything remaining with '?'.
      5. Strip leading/trailing whitespace.
    """
    # Step 1: manual substitutions
    for src, dst in _MANUAL_SUBS.items():
        s = s.replace(src, dst)

    # Step 2-3: NFD + strip combining marks
    nfd = unicodedata.normalize('NFD', s)
    stripped = ''.join(c for c in nfd if unicodedata.category(c) != 'Mn')

    # Step 4-5: ASCII encode + strip
    return stripped.encode('ascii', 'replace').decode('ascii').strip()


def pack_name(name: str, length: int) -> bytes:
    """
    Encode name as fixed-length NUL-terminated ASCII byte string.
    Truncates silently to (length - 1) chars to always leave room for NUL.
    Pads with NUL bytes to exactly `length` bytes.
    """
    encoded = name.encode('ascii', 'replace')[:length - 1]
    return encoded.ljust(length, b'\x00')


def warn(msg: str) -> None:
    print(f'  WARNING: {msg}', file=sys.stderr)


# ---------------------------------------------------------------------------
# Parser: EQUIPAS.EF2
# ---------------------------------------------------------------------------

def parse_equipas(path: str) -> list[dict]:
    """
    Parse EQUIPAS.EF2 and return a list of team dicts.

    Each dict:
        name    : str  — transliterated team name
        nac     : str  — 3-char nationality/league code
        n1      : int  — copa pot value
        n2      : int  — budget factor
        players : list of dicts with keys: name, pos, nac

    Handles all documented anomalies:
      - Missing space between player name and position tag (rsplit approach).
      - Trailing whitespace on any line.
      - CP850 accented characters via transliteration.
      - 'POK' and other non-standard NAC codes (accepted as-is, max 3 chars).
    """
    with open(path, 'rb') as f:
        raw_bytes = f.read()

    # The file is nominally CP850, but some lines were typed in a Latin-1 / CP1252
    # context (e.g., 0xF4 = 'ô' in Latin-1, but U+00B6 PILCROW in CP850).
    # Strategy: decode each line independently — try Latin-1 first (it's a superset
    # of the printable ASCII range we care about), fall back to CP850 for lines that
    # contain CP850 box-drawing characters used as actual box-drawing (0xDA etc.).
    # Since we transliterate everything to ASCII anyway, either decoding is fine
    # as long as the transliteration table covers the differences.
    lines = raw_bytes.decode('latin-1').replace('\r\n', '\n').split('\n')

    teams = []
    i = 0

    while i < len(lines):
        line = lines[i]

        # --- Team header detection ---
        # Format: ' NAME...   NAC'  (leading space, trailing 3-letter code)
        hm = re.match(r'^[ ](.+?)\s+([A-Z]{3})\s*$', line)
        if not hm:
            i += 1
            continue

        # Validate that next two lines are integers (N1, N2)
        if i + 2 >= len(lines):
            i += 1
            continue
        n1_str = lines[i + 1].strip()
        n2_str = lines[i + 2].strip()
        if not re.match(r'^\d+$', n1_str) or not re.match(r'^\d+$', n2_str):
            i += 1
            continue

        team_name_raw = hm.group(1).strip()
        team_nac      = hm.group(2)[:3]
        n1            = int(n1_str)
        n2            = int(n2_str)
        team_name     = transliterate(team_name_raw)

        if len(team_name) > TEAM_NAME_LEN - 1:
            warn(f'Team name too long ({len(team_name)} chars), truncating: {team_name!r}')
            team_name = team_name[:TEAM_NAME_LEN - 1]

        # --- Parse players (until blank line or EOF) ---
        players = []
        j = i + 3

        while j < len(lines):
            pline = lines[j]
            if pline.strip() == '':
                break  # blank line = end of team block

            # Use regex that handles glued pos (e.g. 'Machadomd POR')
            # Anchored to end: last field = NAC, second-to-last = pos
            pm = re.match(r'^(.+?)\s*(gr|df|md|av)\s+([A-Z]{2,3})\s*$', pline)
            if pm:
                pname_raw = pm.group(1).strip()
                pos_str   = pm.group(2)
                p_nac     = pm.group(3)[:3]
                pname     = transliterate(pname_raw)

                if len(pname) > PLAYER_NAME_LEN - 1:
                    warn(f'Player name too long ({len(pname)} chars), truncating: {pname!r}')
                    pname = pname[:PLAYER_NAME_LEN - 1]

                if pos_str not in POS_MAP:
                    warn(f'Unknown position {pos_str!r} for player {pname!r} — defaulting to df')
                    pos_str = 'df'

                players.append({
                    'name': pname,
                    'pos':  POS_MAP[pos_str],
                    'nac':  p_nac,
                })
            else:
                warn(f'Cannot parse player line in team {team_name!r}: {pline!r}')

            j += 1

        if len(players) != PLAYERS_PER_TEAM:
            warn(
                f'Team {team_name!r} has {len(players)} players '
                f'(expected {PLAYERS_PER_TEAM})'
            )

        teams.append({
            'name':    team_name,
            'nac':     team_nac,
            'n1':      n1,
            'n2':      n2,
            'players': players,
        })

        i = j  # continue after the blank separator line

    return teams


# ---------------------------------------------------------------------------
# Parser: TREINAD.EF2
# ---------------------------------------------------------------------------

def parse_treinad(path: str) -> list[str]:
    """
    Parse TREINAD.EF2 and return a list of transliterated coach names.
    Stops at the '[EOF]' sentinel line. Strips trailing whitespace.
    """
    with open(path, 'rb') as f:
        raw = f.read()

    # Same encoding ambiguity as EQUIPAS — decode as Latin-1 and rely on
    # the manual substitution table in transliterate() for edge cases.
    lines = raw.decode('latin-1').replace('\r\n', '\n').split('\n')
    coaches = []

    for line in lines:
        line = line.strip()
        if not line or line.startswith('[EOF]'):
            break
        name = transliterate(line)
        if len(name) > COACH_NAME_LEN - 1:
            warn(f'Coach name too long ({len(name)} chars), truncating: {name!r}')
            name = name[:COACH_NAME_LEN - 1]
        coaches.append(name)

    return coaches


# ---------------------------------------------------------------------------
# Binary serialisers
# ---------------------------------------------------------------------------

def serialise_teams(teams: list[dict]) -> bytes:
    """
    Serialise team list to teams.bin format (big-endian).

    Layout:
        [HEADER 4B]
        For each team:
            [TeamRecord 26B]
            [PlayerRecord 24B] × 16
    """
    buf = bytearray()

    # Header
    buf += struct.pack(TEAMS_HDR_FMT, len(teams), PLAYERS_PER_TEAM)

    for team in teams:
        # TeamRecord
        name_b = pack_name(team['name'], TEAM_NAME_LEN)
        nac_b  = team['nac'].encode('ascii')[:3].ljust(3, b'\x00')
        buf += struct.pack(
            TEAM_REC_FMT,
            name_b,
            team['n1'] & 0xFF,
            team['n2'] & 0xFF,
            nac_b,
            0,   # _pad
        )

        # PlayerRecords
        players = team['players']
        # Pad to exactly PLAYERS_PER_TEAM if somehow short
        while len(players) < PLAYERS_PER_TEAM:
            players.append({'name': '', 'pos': 0, 'nac': 'POR'})

        for player in players[:PLAYERS_PER_TEAM]:
            p_name_b = pack_name(player['name'], PLAYER_NAME_LEN)
            p_nac_b  = player['nac'].encode('ascii')[:3].ljust(3, b'\x00')
            buf += struct.pack(
                PLAYER_REC_FMT,
                p_name_b,
                player['pos'] & 0xFF,
                p_nac_b,
                b'\x00' * PLAYER_PAD,
            )

    return bytes(buf)


def serialise_coaches(coaches: list[str]) -> bytes:
    """
    Serialise coach list to coaches.bin format (big-endian).

    Layout:
        [HEADER 2B]
        [CoachRecord 20B] × coach_count
    """
    buf = bytearray()
    buf += struct.pack(COACHES_HDR_FMT, len(coaches))

    for name in coaches:
        buf += struct.pack(COACH_REC_FMT, pack_name(name, COACH_NAME_LEN))

    return bytes(buf)


# ---------------------------------------------------------------------------
# Verification — print a human-readable summary after packing
# ---------------------------------------------------------------------------

def verify_teams(teams: list[dict]) -> None:
    print(f'\nteams.bin verification:')
    print(f'  Teams:   {len(teams)} (expected {TEAM_COUNT})')
    total_players = sum(len(t['players']) for t in teams)
    print(f'  Players: {total_players} (expected {TEAM_COUNT * PLAYERS_PER_TEAM})')
    print(f'\n  {"#":>2}  {"Name":<20} N1  N2  Ply')
    print(f'  {"--":>2}  {"-"*20} --  --  ---')
    for idx, team in enumerate(teams, 1):
        print(
            f'  {idx:>2}  {team["name"]:<20} '
            f'{team["n1"]:>2}  {team["n2"]:>2}  '
            f'{len(team["players"]):>3}'
        )


def verify_coaches(coaches: list[str]) -> None:
    print(f'\ncoaches.bin verification:')
    print(f'  Coaches: {len(coaches)} (expected {COACH_COUNT})')
    for idx, name in enumerate(coaches):
        print(f'  {idx:>2}: {name}')


def print_sizes(teams_bin: bytes, coaches_bin: bytes) -> None:
    expected_teams   = TEAMS_HDR_SIZE + TEAM_COUNT * (TEAM_REC_SIZE + PLAYERS_PER_TEAM * PLAYER_REC_SIZE)
    expected_coaches = COACHES_HDR_SIZE + COACH_COUNT * COACH_REC_SIZE

    print(f'\nBinary sizes:')
    print(f'  teams.bin:   {len(teams_bin):>6} bytes  (expected {expected_teams})')
    print(f'  coaches.bin: {len(coaches_bin):>6} bytes  (expected {expected_coaches})')

    ok = True
    if len(teams_bin) != expected_teams:
        warn(f'teams.bin size mismatch!')
        ok = False
    if len(coaches_bin) != expected_coaches:
        warn(f'coaches.bin size mismatch!')
        ok = False
    if ok:
        print('  All sizes correct. ✓')


def print_c_header(teams: list[dict], coaches: list[str]) -> None:
    """
    Print the C struct definitions that must match types.h exactly.
    Copy-paste this into game/types.h and verify it compiles.
    """
    print("""
/* -----------------------------------------------------------------------
 * game/types.h — data structs matching teams.bin / coaches.bin layout
 * Generated by pack_data.py — do not edit offsets manually.
 * All integers big-endian (m68k native). int = 16 bits on m68k/SGDK!
 * ----------------------------------------------------------------------- */

#define TEAM_COUNT        29
#define PLAYERS_PER_TEAM  16
#define COACH_COUNT       50

#define TEAM_NAME_LEN     20
#define PLAYER_NAME_LEN   16
#define COACH_NAME_LEN    20

typedef enum {
    POS_GR = 0,   /* Guarda-redes */
    POS_DF = 1,   /* Defensor     */
    POS_MD = 2,   /* Medio        */
    POS_AV = 3,   /* Avancado     */
} Position;

typedef struct {
    char    name[PLAYER_NAME_LEN];  /* offset  0 — NUL-terminated ASCII */
    u8      pos;                    /* offset 16 — Position enum         */
    char    nac[3];                 /* offset 17 — e.g. "POR" (no NUL)  */
    u8      _pad[4];                /* offset 20 — reserved, always 0   */
    /* sizeof = 24 bytes */
} PlayerRecord;

typedef struct {
    char          name[TEAM_NAME_LEN];              /* offset  0 */
    u8            n1;                               /* offset 20 */
    u8            n2;                               /* offset 21 */
    char          nac[3];                           /* offset 22 */
    u8            _pad;                             /* offset 25 */
    PlayerRecord  players[PLAYERS_PER_TEAM];        /* offset 26 */
    /* sizeof = 26 + 16*24 = 410 bytes */
} TeamRecord;

typedef struct {
    char    name[COACH_NAME_LEN];   /* NUL-terminated ASCII */
    /* sizeof = 20 bytes */
} CoachRecord;

/* In data.c — declared extern, defined by rescomp BIN resources: */
/* extern const u8 teams_data[];   */
/* extern const u8 coaches_data[]; */
""")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    if len(sys.argv) != 4:
        print(
            'Usage: python3 pack_data.py <EQUIPAS.EF2> <TREINAD.EF2> <output_dir>',
            file=sys.stderr,
        )
        return 1

    equipas_path  = sys.argv[1]
    treinad_path  = sys.argv[2]
    output_dir    = sys.argv[3]

    for path in (equipas_path, treinad_path):
        if not os.path.isfile(path):
            print(f'Error: file not found: {path}', file=sys.stderr)
            return 1

    os.makedirs(output_dir, exist_ok=True)

    # --- Parse ---
    print('Parsing EQUIPAS.EF2 ...')
    teams = parse_equipas(equipas_path)

    print('Parsing TREINAD.EF2 ...')
    coaches = parse_treinad(treinad_path)

    # --- Validate counts ---
    if len(teams) != TEAM_COUNT:
        warn(f'Expected {TEAM_COUNT} teams, got {len(teams)}')
    if len(coaches) != COACH_COUNT:
        warn(f'Expected {COACH_COUNT} coaches, got {len(coaches)}')

    # --- Serialise ---
    print('Serialising ...')
    teams_bin   = serialise_teams(teams)
    coaches_bin = serialise_coaches(coaches)

    # --- Write ---
    teams_out   = os.path.join(output_dir, 'teams.bin')
    coaches_out = os.path.join(output_dir, 'coaches.bin')

    with open(teams_out, 'wb') as f:
        f.write(teams_bin)
    with open(coaches_out, 'wb') as f:
        f.write(coaches_bin)

    print(f'Written: {teams_out}')
    print(f'Written: {coaches_out}')

    # --- Verify ---
    verify_teams(teams)
    verify_coaches(coaches)
    print_sizes(teams_bin, coaches_bin)
    print_c_header(teams, coaches)

    return 0


if __name__ == '__main__':
    sys.exit(main())
