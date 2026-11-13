#!/usr/bin/env python3
"""Le Chat Express: le chat delivers envelopes across an open-plan office (meme game, phase 4).

You are le chat (a pixel cat). Bump a mailbox to take its envelope (each is tagged with a desk colour,
shown on the mailbox and on the envelopes you carry); you carry at most 2. Bump a desk of the matching
colour to deliver. Win when every envelope is delivered (global `left` == 0); lose after MAX_STEPS steps.

Office hazards (all deterministic):
  * rolling office chairs: walk into one and it rolls until it hits something (you follow one step);
  * coffee spills: slippery - stepping on one makes you slide on until you leave the spill or hit
    something (a slide into a mailbox/desk stops you next to it);
  * meeting rooms: their glass doors are closed for CLOSED ticks, then open for OPEN ticks, repeating
    (all doors share the office clock). Closed doors block you and chairs; you may stand in a doorway.

Validation: an exact Python model (Sim) of the GDY rules and a planner: a depth-first search over
"events" (pick up / deliver), where each event is reached by a BFS over full states (cat, chairs,
mailboxes, carried envelopes, clock phase). The winning run is written as a format-2 solution and
replayed in the engine (replay_solutions.mjs); `--diff` compares Sim with the engine step by step on
the solutions plus random walks.

Map chars: w wall, Q rack, L logo, P plant, T table (solid decor), A le chat, r/b/g/y mailbox
(red/blue/green/yellow envelope), R/B/G/Y desk, h chair, s coffee spill, d meeting door, c carpet.
"""
from __future__ import annotations

import collections
import json
import random
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common as C  # noqa: E402
import memekit as M  # noqa: E402
import office_kit as K  # noqa: E402

GID = "courier"
COLOURS = ["red", "blue", "green", "yellow"]
CI = {c: i + 1 for i, c in enumerate(COLOURS)}
MAIL_CH = dict(zip(COLOURS, "rbgy"))
DESK_CH = dict(zip(COLOURS, "RBGY"))
CH_MAIL = {v: CI[k] for k, v in MAIL_CH.items()}
CH_DESK = {v: CI[k] for k, v in DESK_CH.items()}
RGB = {"red": (225, 50, 50), "blue": (60, 120, 235), "green": (60, 180, 80), "yellow": (250, 205, 30)}
DIRS = K.DIRS
CLOSED, OPEN = 6, 5
PERIOD = CLOSED + OPEN
MAX_STEPS = 1500
SOLID = set("wQLPT")


def door_open(t):
    """Door state after t engine ticks (first opening at tick CLOSED, closing OPEN ticks later...)."""
    return (t - CLOSED) % PERIOD < OPEN


# ------------------------------------------------------------------------------ simulation
class Sim:
    """State: (cat, chairs (sorted tuple), mail (frozenset of (pos, colour)), e1, e2, left, t)."""

    def __init__(self, rows):
        self.rows = rows
        self.h, self.w = len(rows), len(rows[0])
        cell = lambda chs: {(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c in chs}
        self.solid = cell(SOLID)
        self.spill = cell("s")
        self.doors = cell("d")
        self.desks = {(x, y): CH_DESK[c] for y, r in enumerate(rows) for x, c in enumerate(r) if c in CH_DESK}
        mail = frozenset(((x, y), CH_MAIL[c]) for y, r in enumerate(rows) for x, c in enumerate(r) if c in CH_MAIL)
        self.mailboxes = {p for p, _ in mail}  # empty mailboxes stay solid
        self.start = (C.find(rows, "A")[0], tuple(sorted(cell("h"))), mail, 0, 0, len(mail), 0)

    def blocked_static(self, p):
        return (p in self.solid or p in self.desks or p in self.mailboxes
                or not (0 <= p[0] < self.w and 0 <= p[1] < self.h))

    def step(self, s, d):
        cat, chairs, mail, e1, e2, left, t = s
        dv = DIRS[d]
        dopen = door_open(t)
        chairs_set = set(chairs)
        nxt = K.add(cat, dv)
        if nxt in self.mailboxes:
            m = next((c for p, c in mail if p == nxt), None)
            if m is not None and (e1 == 0 or e2 == 0):
                mail = mail - {(nxt, m)}
                if e1 == 0:
                    e1 = m
                else:
                    e2 = m
        elif nxt in self.desks:
            c = self.desks[nxt]
            if e1 == c:
                e1, e2, left = e2, 0, left - 1
            elif e2 == c:
                e2, left = 0, left - 1
        elif nxt in chairs_set:
            p = nxt
            while True:  # the chair rolls until something stops it
                q = K.add(p, dv)
                if self.blocked_static(q) or q in chairs_set or q == cat or (q in self.doors and not dopen):
                    break
                chairs_set.discard(p)
                chairs_set.add(q)
                p = q
            if p != nxt:
                cat = nxt
            chairs = tuple(sorted(chairs_set))
        elif self.blocked_static(nxt) or (nxt in self.doors and not dopen):
            pass
        else:
            cat = nxt
            while cat in self.spill:  # slide
                q = K.add(cat, dv)
                if self.blocked_static(q) or q in chairs_set or (q in self.doors and not dopen):
                    break
                cat = q
        return cat, chairs, mail, e1, e2, left, t + 1

    def key(self, s):
        return s[:6] + ((s[6] % PERIOD) if self.doors else 0,)


def events_from(sim, s, cap):
    """BFS from s to every nearest state where the mail/carry changed. Returns [(path, state)]."""
    k0 = sim.key(s)
    par = {k0: None}
    q = collections.deque([s])
    found = []
    seen_ev = set()
    while q:
        cur = q.popleft()
        for d in DIRS:
            n = sim.step(cur, d)
            kn = sim.key(n)
            if kn in par:
                continue
            par[kn] = (sim.key(cur), d)
            if n[2:6] != cur[2:6]:
                ek = (n[0], n[2:6])
                if ek in seen_ev:
                    continue
                seen_ev.add(ek)
                path, k = [], kn
                while par[k] is not None:
                    k, dd = par[k]
                    path.append(dd)
                found.append((path[::-1], n))
                continue
            if len(par) > cap:
                return found
            q.append(n)
    return found


def plan(rows, cap=60_000, branch=4, budget=3000):
    sim = Sim(rows)
    seen = set()
    calls = [0]

    def dfs(s):
        if s[5] == 0:
            return []
        calls[0] += 1
        if calls[0] > budget:
            return None
        k = sim.key(s)
        if k in seen:
            return None
        seen.add(k)
        evs = events_from(sim, s, cap)
        # deliveries first, then nearest
        evs.sort(key=lambda e: (e[1][5] >= s[5], len(e[0])))
        for path, n in evs[:branch]:
            rest = dfs(n)
            if rest is not None:
                return path + rest
        return None

    return dfs(sim.start), calls[0]


# ------------------------------------------------------------------------------ levels
LEVELS = [
    ("First Delivery", "Bump a mailbox to take its envelope (2 max), bump the matching desk to deliver.", """
wwwwwwwwwwwwwwwwwwwwww
wQQ....L.....L....PPPw
w....................w
wr...RRR......BBB...rw
w....................w
w.A..TTT......TTT...bw
w....................w
wb...BBB......RRR....w
w....................w
wPP....L.....L....QQQw
wwwwwwwwwwwwwwwwwwwwww
"""),
    ("Coffee Break", "Spilled coffee is slippery: you slide until you leave it - or hit something.", """
wwwwwwwwwwwwwwwwwwwwwwwwww
wQQ.....L.........L..PPPPw
w........................w
w..RRR....wwwwwwwww...GG.w
w.........wsssssssw......w
w.A.......sssTsssssr..GG.w
wr........sssssssgw......w
w.........ssssssTsw...RR.w
wg..GGG...wsTsssssw......w
w.........wssssssyw......w
wPP.......wwwwwwwwwYYY..Pw
wwwwwwwwwwwwwwwwwwwwwwwwww
"""),
    ("Chair Race", "Office chairs roll until they hit something. Clear the aisles - carefully.", """
wwwwwwwwwwwwwwwwwwwwwwwwwwww
wQQQ....L..........L...PPPPw
w..........................w
wRRRRR.TTTTTTT.wBBBBBBBBBB.w
w.h.........h.....h........w
wTTTTTTTT.TTTTTTT.TTTTTTTT.w
wb.......h...........h....rw
wTTTTTTT.TTTTTTTTTT.TTTTTTTw
w..h.......h.........h.....w
wGGGGGGGGGGGG.TTTTTTT.RRRRRw
w..A.......................w
wPPP...g....L.......L...yQQw
wYYYYYYYYYYYYYYYYYYYYYYYYYYw
wwwwwwwwwwwwwwwwwwwwwwwwwwww
"""),
    ("All-Hands", "Meeting rooms open and close on the office clock. Time your deliveries.", """
wwwwwwwwwwwwwwwwwwwwwwwwwwwwww
wcccccRwcccccBwQQ.L..L.QQPPPPw
wccTccc.cccTccw..............w
wccTcccwcccTccw..r.........b.w
wcccccGwcccccRw..............w
wwwdwwwwwwdwwww......A.......w
w............................w
w..g.........y...wwwdwwwwdwwww
w................wcccccwccccYw
w..PP.....PP.....wcTTccwccTTcw
wQQQQ....LLL.....wBcccc.ccccGw
wwwwwwwwwwwwwwwwwwwwwwwwwwwwww
"""),
    ("Rush Hour", "Chairs, coffee and meetings - eight envelopes before the end of the day.", """
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
wcccccRccwQQQ...L....L...QQwccccYw
wccTTTTccw...............h.dccTTcw
wcccccccBw..ssss...r.........cccBw
wwwwdwwwww..ssss.......TTTT.wwwwww
w.....h.....ssss....h..TTTT......w
wb............ss.................w
wTTTTTT.TTTTT.....TTTTTT.TTTTTT.yw
w...........h.........h..........w
wRRRRRR..GGGGGG..sss.YYYYYY..BBBBw
w..A.............sss.............w
wg..........h....sss.......r....Pw
wPPP.....LLL.............LLL....Pw
wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww
"""),
]


# ------------------------------------------------------------------------------ art
def rgba(c):
    return tuple(c[:3]) + (255,)


def mini_envelope(px, ox, oy, col):
    for y in range(oy, oy + 7):
        for x in range(ox, ox + 9):
            edge = y in (oy, oy + 6) or x in (ox, ox + 8)
            px[y][x] = M.P["outline"] if edge else (244, 238, 222, 255)
    for i in range(4):
        px[oy + 1 + i][ox + 1 + i] = (180, 170, 150, 255)
        px[oy + 1 + i][ox + 7 - i] = (180, 170, 150, 255)
    for y in range(oy + 3, oy + 6):
        for x in range(ox + 3, ox + 6):
            px[y][x] = rgba(RGB[col])


def avatar_sprites():
    base = M.cat("pixel")
    out = []
    for e1 in range(5):
        for e2 in range(5):
            for rel, d in zip(base, M.FACINGS):
                if e1 == 0 and e2 == 0 or (e1 == 0 and e2 > 0):
                    out.append(rel)
                    continue

                def draw(px, e1=e1, e2=e2):
                    mini_envelope(px, 0, 16, COLOURS[e1 - 1])
                    if e2:
                        mini_envelope(px, 15, 16, COLOURS[e2 - 1])
                out.append(K.overlay(rel, f"courier-chat-{e1}{e2}-{d}", draw))
    return out


def mailbox_sprite(col):
    px = M.blank()
    M.rect(px, 10, 13, 13, 22, M.P["steel_dark"])  # post
    M.rect(px, 3, 4, 20, 13, (60, 70, 90, 255))
    M.rect(px, 3, 4, 20, 5, (100, 110, 130, 255))
    M.rect(px, 5, 7, 18, 8, M.P["outline"])  # slot
    if col:
        M.rect(px, 19, 1, 20, 9, M.P["steel"])  # flag up
        M.rect(px, 21, 1, 23, 4, rgba(RGB[col]))
        mini_envelope(px, 7, 2, col)
    else:
        M.rect(px, 19, 9, 22, 10, M.P["steel"])  # flag down
    M.outline(px)
    return M.save(f"courier-mailbox-{col or 'empty'}", px)


def desk_sprite(col):
    px = M.blank()
    M.rect(px, 1, 9, 22, 13, (176, 130, 84, 255))
    M.rect(px, 1, 9, 22, 9, (206, 160, 110, 255))
    M.rect(px, 2, 14, 3, 21, (120, 86, 52, 255))
    M.rect(px, 20, 14, 21, 21, (120, 86, 52, 255))
    M.rect(px, 7, 1, 16, 7, M.P["outline"])  # monitor
    M.rect(px, 8, 2, 15, 6, rgba(RGB[col]))
    M.rect(px, 9, 3, 11, 3, K.blend(RGB[col], (255, 255, 255), 0.4))
    M.rect(px, 11, 8, 12, 8, M.P["outline"])
    M.rect(px, 17, 10, 20, 11, (240, 240, 240, 255))  # keyboard-ish note
    M.outline(px)
    return M.save(f"courier-desk-{col}", px)


def table_sprite():
    px = M.blank()
    M.rect(px, 0, 7, 23, 14, (150, 154, 168, 255))
    M.rect(px, 0, 7, 23, 8, (196, 200, 214, 255))
    M.rect(px, 2, 15, 3, 22, M.P["steel_dark"])
    M.rect(px, 20, 15, 21, 22, M.P["steel_dark"])
    M.ellipse(px, 6, 5.5, 2.2, 2.0, (250, 250, 250, 255))  # coffee cup
    M.rect(px, 5, 5, 7, 5, (110, 70, 40, 255))
    M.outline(px)
    return M.save("courier-table", px)


def chair_sprite():
    px = M.blank()
    M.rect(px, 6, 2, 17, 11, (50, 52, 64, 255))
    M.rect(px, 7, 3, 16, 10, (80, 84, 104, 255))
    M.rect(px, 5, 12, 18, 15, (50, 52, 64, 255))
    M.rect(px, 11, 16, 12, 18, M.P["steel_dark"])
    for x in (5, 11, 18):
        M.rect(px, x - 1, 19, x + 1, 19, M.P["steel"])
        M.put(px, x, 21, M.P["black"])
        M.put(px, x, 20, M.P["black"])
    M.outline(px)
    M.ground_shadow(px, ry=1.0, cy=22.5, rx=8)
    return M.save("courier-chair", px)


def spill_sprite():
    px = M.blank()
    for cx, cy, r in ((12, 12, 8.5), (5, 7, 3.5), (18, 18, 3.5), (19, 6, 2.5), (6, 19, 2.5)):
        M.ellipse(px, cx, cy, r, r * 0.85, (120, 72, 36, 230), (170, 116, 70, 230), (86, 50, 24, 230))
    M.ellipse(px, 9, 9, 2, 1.2, (220, 190, 150, 255))
    return M.save("courier-spill", px)


def door_sprites():
    closed = M.blank((150, 200, 230, 255))
    for y in range(24):
        closed[y][0] = closed[y][23] = M.P["steel_dark"]
        closed[y][11] = closed[y][12] = M.P["steel"]
    for x in range(24):
        closed[0][x] = closed[23][x] = M.P["steel_dark"]
    for i in range(6):
        closed[4 + i][4 + i] = (220, 240, 255, 255)
    M.rect(closed, 4, 15, 19, 19, M.P["outline"])  # "IN MEETING" sign
    M.rect(closed, 5, 16, 18, 18, M.P["led_red"])
    open_ = M.blank((150, 156, 166, 255))
    for y in range(24):
        for x in (0, 1, 22, 23):
            open_[y][x] = (150, 200, 230, 255)
    M.rect(open_, 8, 10, 15, 13, M.P["outline"])
    M.rect(open_, 9, 11, 14, 12, M.P["led_green"])
    return [M.save("courier-door-closed", closed), M.save("courier-door-open", open_)]


def carpet_sprite():
    px = M.blank((92, 70, 120, 255))
    for y in range(24):
        for x in range(24):
            if (x + y) % 6 == 0:
                px[y][x] = (104, 82, 134, 255)
    return M.save("courier-carpet", px)


# ------------------------------------------------------------------------------ GDY
def actions():
    retile = ("{set: [tb, 0]}, "
              + ", ".join(f"{{if: {{Conditions: {{eq: [e1, {i}]}}, OnTrue: [{{add: [tb, {20 * i}]}}]}}}}" for i in range(1, 5))
              + ", "
              + ", ".join(f"{{if: {{Conditions: {{eq: [e2, {i}]}}, OnTrue: [{{add: [tb, {4 * i}]}}]}}}}" for i in range(1, 5))
              + ", {set: [ft, fc]}, {add: [ft, tb]}, {set_tile: ft}")
    face = "{set_tile: meta.facing}, {set: [fc, meta.facing]}, {set: [ft, fc]}, {add: [ft, tb]}, {set_tile: ft}"
    floor = "[_empty, carpet]"
    beh = [
        f"      - Src: {{Object: chat, Commands: [{face}, {{mov: _dest}}]}}\n        Dst: {{Object: {floor}}}\n",
        f"      - Src: {{Object: chat, Commands: [{face}, {{mov: _dest}}, {{exec: {{Action: slide}}}}]}}\n"
        f"        Dst: {{Object: spill}}\n",
        f"      - Src: {{Object: chat, Preconditions: [{{eq: [dst.open, 1]}}], Commands: [{face}, {{mov: _dest}}]}}\n"
        f"        Dst: {{Object: door}}\n",
        f"      - Src: {{Object: chat, Preconditions: [{{eq: [dst.open, 0]}}], Commands: [{face}]}}\n"
        f"        Dst: {{Object: door}}\n",
        f"      - Src: {{Object: chat, Commands: [{face}, {{mov: _dest}}]}}\n"
        f"        Dst: {{Object: chair, Commands: [{{cascade: _dest}}]}}\n",
        f"      - Src: {{Object: chat, Commands: [{face}]}}\n"
        f"        Dst: {{Object: [wall, rack, logo, plant, table, mailbox_empty]}}\n",
    ]
    for c in COLOURS:
        i = CI[c]
        beh += [
            f"      - Src: {{Object: chat, Preconditions: [{{eq: [src.e1, 0]}}], Commands: [{face}, {{set: [e1, {i}]}}, "
            f"{retile}, {{reward: 1}}]}}\n        Dst: {{Object: mailbox_{c}, Commands: [{{change_to: mailbox_empty}}]}}\n",
            f"      - Src: {{Object: chat, Preconditions: [{{gte: [src.e1, 1]}}, {{eq: [src.e2, 0]}}], Commands: [{face}, "
            f"{{set: [e2, {i}]}}, {retile}, {{reward: 1}}]}}\n"
            f"        Dst: {{Object: mailbox_{c}, Commands: [{{change_to: mailbox_empty}}]}}\n",
            f"      - Src: {{Object: chat, Preconditions: [{{gte: [src.e1, 1]}}, {{gte: [src.e2, 1]}}], Commands: [{face}]}}\n"
            f"        Dst: {{Object: mailbox_{c}}}\n",
            f"      - Src: {{Object: chat, Preconditions: [{{eq: [src.e1, {i}]}}], Commands: [{face}, {{set: [e1, e2]}}, "
            f"{{set: [e2, 0]}}, {{decr: left}}, {retile}, {{reward: 3}}]}}\n        Dst: {{Object: desk_{c}}}\n",
            f"      - Src: {{Object: chat, Preconditions: [{{neq: [src.e1, {i}]}}, {{eq: [src.e2, {i}]}}], Commands: [{face}, "
            f"{{set: [e2, 0]}}, {{decr: left}}, {retile}, {{reward: 3}}]}}\n        Dst: {{Object: desk_{c}}}\n",
            f"      - Src: {{Object: chat, Preconditions: [{{neq: [src.e1, {i}]}}, {{neq: [src.e2, {i}]}}], Commands: [{face}]}}\n"
            f"        Dst: {{Object: desk_{c}}}\n",
        ]
    roll = [
        f"      - Src: {{Object: chair, Commands: [{{mov: _dest}}, {{exec: {{Action: roll}}}}]}}\n"
        f"        Dst: {{Object: [_empty, carpet, spill]}}\n",
        f"      - Src: {{Object: chair, Preconditions: [{{eq: [dst.open, 1]}}], Commands: [{{mov: _dest}}, {{exec: {{Action: roll}}}}]}}\n"
        f"        Dst: {{Object: door}}\n",
    ]
    slide = [
        f"      - Src: {{Object: chat, Commands: [{{mov: _dest}}]}}\n        Dst: {{Object: {floor}}}\n",
        f"      - Src: {{Object: chat, Commands: [{{mov: _dest}}, {{exec: {{Action: slide}}}}]}}\n        Dst: {{Object: spill}}\n",
        f"      - Src: {{Object: chat, Preconditions: [{{eq: [dst.open, 1]}}], Commands: [{{mov: _dest}}]}}\n"
        f"        Dst: {{Object: door}}\n",
    ]
    return f"""
Actions:
{C.face_action("chat")}  # Walk; bump mailboxes (take an envelope) and desks (deliver); push chairs; slip on coffee.
  - Name: move
{C.MOVE_INPUTS}    Behaviours:
{"".join(beh + roll)}  # Chairs keep rolling (exec without ActionId reuses the push direction).
  - Name: roll
    InputMapping:
      Internal: true
    Behaviours:
{"".join(roll)}  # Sliding on coffee: keep going while on a spill.
  - Name: slide
    InputMapping:
      Internal: true
    Behaviours:
{"".join(slide)}  # Meeting doors: closed {CLOSED} ticks, open {OPEN} ticks (the top object of the door cell may be
  # le chat or a chair standing in the doorway).
  - Name: meet
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: open, VectorToDest: [0, 0], MetaData: {{o: 1, nx: 2, dl: {OPEN}}}}}
        2: {{Description: close, VectorToDest: [0, 0], MetaData: {{o: 0, nx: 1, dl: {CLOSED}}}}}
    Behaviours:
      - Src: {{Object: door, Commands: [{{set: [open, meta.o]}}, {{set_tile: meta.o}}, {{exec: {{Action: meet, ActionId: meta.nx, Delay: meta.dl}}}}]}}
        Dst: {{Object: [door, chat, chair]}}
  - Name: count_mail
    InputMapping:
      Internal: true
      Inputs:
        1: {{Description: count, VectorToDest: [0, 0]}}
    Behaviours:
      - Src: {{Object: [{", ".join(f"mailbox_{c}" for c in COLOURS)}], Commands: [{{incr: left}}]}}
        Dst: {{Object: [{", ".join(f"mailbox_{c}" for c in COLOURS)}]}}
"""


def objects():
    acts, decor = K.decor_objects()
    count = "    InitialActions:\n      - Action: count_mail\n        ActionId: 1\n"
    out = [K.wall_object(), decor,
           C.obj("table", "T", 1, C.img(table_sprite()), "[0.6, 0.6, 0.65]"),
           C.obj("carpet", "c", 0, C.img(carpet_sprite()), "[0.35, 0.27, 0.47]"),
           C.obj("spill", "s", 0, C.img(spill_sprite()), "[0.45, 0.28, 0.14]", "square", 0.8),
           C.obj("door", "d", 0, C.imgs(door_sprites()), "[0.6, 0.8, 0.9]", "square", 0.9,
                 "    Variables:\n      - Name: open\n        InitialValue: 0\n"
                 f"    InitialActions:\n      - Action: meet\n        ActionId: 1\n        Delay: {CLOSED}\n", tiles=2),
           C.obj("chair", "h", 1, C.img(chair_sprite()), "[0.3, 0.3, 0.4]", "triangle", 0.6),
           C.obj("mailbox_empty", None, 1, C.img(mailbox_sprite(None)), "[0.3, 0.3, 0.3]", "square", 0.5)]
    for c in COLOURS:
        rgb = "[%.2f, %.2f, %.2f]" % tuple(v / 255 for v in RGB[c])
        out.append(C.obj(f"mailbox_{c}", MAIL_CH[c], 1, C.img(mailbox_sprite(c)), rgb, "triangle", 0.6, count))
        out.append(C.obj(f"desk_{c}", DESK_CH[c], 1, C.img(desk_sprite(c)), rgb, "square", 1.0))
    av_vars = "    Variables:\n" + "".join(f"      - Name: {v}\n        InitialValue: {iv}\n"
                                          for v, iv in (("e1", 0), ("e2", 0), ("tb", 0), ("fc", 2), ("ft", 2)))
    out.append(C.obj("chat", "A", 1, C.imgs(avatar_sprites()), "[1.0, 0.6, 0.2]", "triangle", 0.8,
                     av_vars + C.FACE_INITIAL_ACTION, tiles=100))
    return acts, "\nObjects:\n" + "".join(out)


# ------------------------------------------------------------------------------ main
def level_rows(text):
    return text.strip("\n").split("\n")


def validate(name, rows):
    C.check_rect(name, rows)
    assert sum(r.count("A") for r in rows) == 1, f"{name}: one avatar"
    sim = Sim(rows)
    cols = {c for _, c in sim.start[2]}
    assert cols <= set(sim.desks.values()), f"{name}: envelope without desk"
    sol, calls = plan(rows)
    assert sol, f"{name}: planner failed ({calls} event searches)"
    s = sim.start
    for d in sol:
        s = sim.step(s, d)
    assert s[5] == 0 and len(sol) < MAX_STEPS
    return sim, sol, calls


def diff(gdy, runs, levels):
    here = Path(__file__).resolve().parent
    with tempfile.TemporaryDirectory() as td:
        gp, jp, op = Path(td, "g.yaml"), Path(td, "j.json"), Path(td, "o.json")
        gp.write_text(gdy)
        keep = ["chat", "chair", "door"] + [f"mailbox_{c}" for c in COLOURS]
        jp.write_text(json.dumps([{"level": li, "actions": [["move", d] for d in run], "keep": keep}
                                  for run, li in zip(runs, levels)]))
        subprocess.run(["node", str(here / "office_dump.mjs"), str(gp), str(jp), str(op)], check=True,
                       stdout=subprocess.DEVNULL)
        dumps = json.loads(op.read_text())
    bad = 0
    for j, (run, states, li) in enumerate(zip(runs, dumps, levels)):
        sim = Sim(level_rows(LEVELS[li][2]))
        s = sim.start
        for k, st in enumerate(states):
            if k:
                s = sim.step(s, run[k - 1])
            o = st["objs"]
            av = next(x for x in o if x[0] == "chat")
            eng = ((av[1], av[2]), tuple(sorted((x[1], x[2]) for x in o if x[0] == "chair")),
                   frozenset(((x[1], x[2]), CI[x[0][8:]]) for x in o if x[0].startswith("mailbox_")),
                   av[3]["e1"], av[3]["e2"], st["globals"]["left"])
            dopen = [x[3]["open"] for x in o if x[0] == "door"]
            term = "Win" if s[5] == 0 else None
            if eng != s[:6] or any(v != door_open(s[6]) for v in dopen) or st["term"] != term:
                print(f"  run {j} (L{li}) step {k} {run[k - 1] if k else ''}: sim={s} engine={eng} doors={dopen} "
                      f"{st['term']}", file=sys.stderr)
                bad += 1
                break
        else:
            print(f"  run {j} (L{li}): sim == engine over {len(states)} states", file=sys.stderr)
    return bad


def main():
    parts = [C.header("Le Chat Express", """
        Le Chat Express: le chat delivers envelopes across the open-plan office. Bump a mailbox to take
        its envelope (you carry 2 at most), bump a desk of the matching colour to deliver it. Office
        chairs roll away when pushed, spilled coffee makes you slide, and meeting-room doors open and
        close on the office clock. Deliver every envelope to win.
        """.replace("        ", ""), "chat", ["eq: [left, 0]"], [f"eq: [_steps, {MAX_STEPS}]"],
        background=K.floor_bg(), variables="    - Name: left\n      InitialValue: 0\n")]
    runs = []
    for i, (name, blurb, text) in enumerate(LEVELS):
        rows = level_rows(text)
        sim, sol, calls = validate(name, rows)
        runs.append(sol)
        parts.append(C.level_block(name, blurb, rows))
        print(f"{i}. {name:15s} {len(rows[0])}x{len(rows)} envelopes={sim.start[5]} chairs={len(sim.start[1])} "
              f"spills={len(sim.spill)} doors={len(sim.doors)} -> planner win in {len(sol)} moves ({calls} event "
              f"searches)", file=sys.stderr)
    acts, objs = objects()
    gdy = "".join(parts) + actions() + acts + objs
    if "--diff" in sys.argv:
        rng = random.Random(5)
        fr, fl = list(runs), list(range(len(runs)))
        for li in range(len(runs)):
            for _ in range(6):
                fr.append([rng.choice([1, 2, 3, 4]) for _ in range(150)])
                fl.append(li)
        sys.exit(1 if diff(gdy, fr, fl) else 0)
    K.write_solutions2(GID, [[("move", d) for d in run] for run in runs])
    C.emit(GID, gdy, {
        "name": "Le Chat Express",
        "tagline": "Le chat delivers the mail. Mind the chairs, the coffee and the meetings.",
        "description": "Take envelopes from the mailboxes (two at a time) and deliver each to the desk of its "
                       "colour. Office chairs roll away when pushed, spilled coffee is slippery and meeting "
                       "rooms only open now and then.",
        "tags": ["meme", "puzzle", "delivery"],
        "difficulty": "medium",
        "original": "new (Mistral office meme)",
        "accent": "#38bdf8",
        "icon": "✉️",
        "controls": {"move": "move", "extra": []},
        "hud": [{"label": "To deliver", "kind": "global", "variable": "left", "tone": "sky"},
                {"label": "Envelope 1", "kind": "var", "object": "chat", "variable": "e1", "tone": "amber"},
                {"label": "Envelope 2", "kind": "var", "object": "chat", "variable": "e2", "tone": "amber"}],
        "win_text": "Every envelope delivered. Le chat est le meilleur facteur.",
        "lose_text": "The office closed before all the mail was delivered.",
    })


if __name__ == "__main__":
    main()
