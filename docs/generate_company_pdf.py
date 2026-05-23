#!/usr/bin/env python3
"""Build WireStack new-hire onboarding PDF (flow-based layout, no overlap)."""
from __future__ import annotations

import sys
from pathlib import Path

from fpdf import FPDF

DOCS = Path(__file__).resolve().parent
ASSETS = DOCS / "assets"
OUT = DOCS / "WireStack-Company-Overview.pdf"

PAGE_W = 210.0
PAGE_H = 297.0
MARGIN = 20.0
FOOTER_Y = 282.0

NAVY = (11, 31, 58)
BLUE = (30, 111, 255)
TEAL = (0, 201, 167)
SLATE = (71, 85, 105)
INK = (30, 41, 59)
LIGHT = (241, 245, 249)
WHITE = (255, 255, 255)
MID = (226, 232, 240)


class WireStackPDF(FPDF):
    def footer(self):
        if self.page_no() == 1:
            return
        self.set_y(-12)
        self.set_font("Helvetica", "I", 8)
        self.set_text_color(*SLATE)
        self.cell(0, 6, f"WireStack Confidential  |  Page {self.page_no()}", align="C")


def epw(pdf: FPDF) -> float:
    return pdf.epw


def set_font(pdf: FPDF, style: str = "", size: int = 10) -> None:
    pdf.set_font("Helvetica", style, size)


def text_h(pdf: FPDF, w: float, line_h: float, text: str) -> float:
    """Estimate block height for wrapped text."""
    if not text.strip():
        return 0.0
    set_font(pdf, "", int(line_h * 2.2))
    lines = pdf.multi_cell(w, line_h, text, dry_run=True, output="LINES")
    return len(lines) * line_h


def ensure_space(pdf: FPDF, need_mm: float) -> None:
    if pdf.get_y() + need_mm > FOOTER_Y:
        pdf.add_page()


def advance(pdf: FPDF, dy: float) -> None:
    pdf.set_y(pdf.get_y() + dy)


def header_bar(pdf: FPDF, title: str, subtitle: str = "") -> None:
    y0 = pdf.get_y()
    bar_h = 12.0 if not subtitle else 16.0
    pdf.set_fill_color(*NAVY)
    pdf.rect(0, y0, PAGE_W, bar_h, style="F")
    pdf.set_xy(MARGIN, y0 + 2.5)
    set_font(pdf, "B", 13)
    pdf.set_text_color(*WHITE)
    pdf.cell(epw(pdf), 6, title)
    if subtitle:
        pdf.set_xy(MARGIN, y0 + 9)
        set_font(pdf, "", 8)
        pdf.set_text_color(*TEAL)
        pdf.cell(epw(pdf), 4, subtitle)
    pdf.set_y(y0 + bar_h + 5)


def paragraph(pdf: FPDF, text: str, size: int = 10) -> None:
    w = epw(pdf)
    lh = 5.0
    h = text_h(pdf, w, lh, text) + 2
    ensure_space(pdf, h)
    pdf.set_x(pdf.l_margin)
    set_font(pdf, "", size)
    pdf.set_text_color(*INK)
    pdf.multi_cell(w, lh, text)
    advance(pdf, 3)


def bullets(pdf: FPDF, items: list[str], size: int = 9) -> None:
    w = epw(pdf) - 8
    lh = 4.5
    for item in items:
        block_h = text_h(pdf, w, lh, item) + 3
        ensure_space(pdf, block_h)
        y0 = pdf.get_y()
        pdf.set_xy(pdf.l_margin + 2, y0 + 1)
        set_font(pdf, "B", 10)
        pdf.set_text_color(*TEAL)
        pdf.cell(4, 4, "-")
        pdf.set_xy(pdf.l_margin + 7, y0)
        set_font(pdf, "", size)
        pdf.set_text_color(*INK)
        pdf.multi_cell(w, lh, item)
        advance(pdf, 2)


def callout(pdf: FPDF, lead: str, body: str) -> None:
    w = epw(pdf) - 10
    lead_h = text_h(pdf, w, 5.0, lead)
    body_h = text_h(pdf, w, 4.5, body)
    box_h = lead_h + body_h + 10
    ensure_space(pdf, box_h + 4)

    x = pdf.l_margin
    y0 = pdf.get_y()
    pdf.set_fill_color(*LIGHT)
    pdf.rect(x, y0, epw(pdf), box_h, style="F")
    pdf.set_fill_color(*TEAL)
    pdf.rect(x, y0, 2, box_h, style="F")

    pdf.set_xy(x + 6, y0 + 4)
    set_font(pdf, "B", 10)
    pdf.set_text_color(*NAVY)
    pdf.multi_cell(w, 5, lead)
    set_font(pdf, "", 9)
    pdf.set_text_color(*INK)
    pdf.multi_cell(w, 4.5, body)
    pdf.set_y(y0 + box_h + 4)


def two_panels(pdf: FPDF, left_lines: list, right_lines: list) -> None:
    gap = 4.0
    col_w = (epw(pdf) - gap) / 2
    y0 = pdf.get_y()

    def measure_panel(lines, w_inner):
        h = 10.0
        for text, style, size, color in lines:
            if not text:
                continue
            set_font(pdf, style, size)
            lh = max(4.0, size * 0.45)
            h += text_h(pdf, w_inner - 10, lh, text) + 2
        return h

    h_left = measure_panel(left_lines, col_w)
    h_right = measure_panel(right_lines, col_w)
    box_h = max(h_left, h_right)
    ensure_space(pdf, box_h + 4)

    y0 = pdf.get_y()

    def draw_panel(x, w_col, bg, lines):
        pdf.set_fill_color(*bg)
        pdf.rect(x, y0, w_col, box_h, style="F")
        cy = y0 + 5
        for text, style, size, color in lines:
            if not text:
                continue
            pdf.set_xy(x + 5, cy)
            set_font(pdf, style, size)
            pdf.set_text_color(*color)
            lh = max(4.0, size * 0.45)
            pdf.multi_cell(w_col - 10, lh, text)
            cy = pdf.get_y() + 2

    draw_panel(pdf.l_margin, col_w, NAVY, left_lines)
    draw_panel(pdf.l_margin + col_w + gap, col_w, TEAL, right_lines)
    pdf.set_y(y0 + box_h + 6)


def step_card(pdf: FPDF, num: int, title: str, desc: str) -> None:
    w = epw(pdf)
    text_w = w - 22
    title_h = 6.0
    desc_h = text_h(pdf, text_w, 4.2, desc)
    box_h = max(14.0, 8 + title_h + desc_h)
    ensure_space(pdf, box_h + 2)

    y0 = pdf.get_y()
    pdf.set_fill_color(*LIGHT)
    pdf.rect(pdf.l_margin, y0, w, box_h, style="F")

    cx = pdf.l_margin + 9
    cy = y0 + box_h / 2
    pdf.set_fill_color(*TEAL)
    pdf.ellipse(cx - 4.5, cy - 4.5, 9, 9, style="F")
    pdf.set_xy(cx - 2.5, cy - 3)
    set_font(pdf, "B", 9)
    pdf.set_text_color(*WHITE)
    pdf.cell(5, 4, str(num), align="C")

    tx = pdf.l_margin + 18
    pdf.set_xy(tx, y0 + 4)
    set_font(pdf, "B", 10)
    pdf.set_text_color(*NAVY)
    pdf.cell(text_w, 5, title)

    pdf.set_xy(tx, y0 + 9)
    set_font(pdf, "", 8)
    pdf.set_text_color(*SLATE)
    pdf.multi_cell(text_w, 4.2, desc)

    pdf.set_y(y0 + box_h + 2)


def cap_card_height(pdf: FPDF, w: float, title: str, desc: str) -> float:
    inner = w - 6
    return 6 + text_h(pdf, inner, 4.0, title) + 4 + text_h(pdf, inner, 3.5, desc)


def cap_card(pdf: FPDF, x: float, y: float, w: float, h: float, phase: str, title: str, desc: str) -> None:
    pad = 3.0
    inner = w - 2 * pad
    pdf.set_fill_color(*LIGHT)
    pdf.set_draw_color(*MID)
    pdf.set_line_width(0.15)
    pdf.rect(x, y, w, h, style="FD")

    pdf.set_xy(x + pad, y + 3)
    set_font(pdf, "B", 7)
    pdf.set_text_color(*TEAL)
    pdf.cell(inner, 3, phase)

    pdf.set_xy(x + pad, y + 7)
    set_font(pdf, "B", 8)
    pdf.set_text_color(*NAVY)
    pdf.multi_cell(inner, 4.0, title)

    pdf.set_x(x + pad)
    set_font(pdf, "", 7)
    pdf.set_text_color(*SLATE)
    pdf.multi_cell(inner, 3.5, desc)


def arch_diagram(pdf: FPDF) -> None:
    labels = [
        "Network interface",
        "capture.c (libpcap)",
        "packet / protocol / connection",
        "stats + alerts + anomaly",
        "SQLite + logs + reports",
        "IPC socket  <->  vigil-cli",
    ]
    bx = pdf.l_margin + epw(pdf) * 0.18
    bw = epw(pdf) * 0.64
    bh = 8.5
    gap = 3.0
    total_h = len(labels) * (bh + gap) + 2
    ensure_space(pdf, total_h)

    y = pdf.get_y()
    for i, lab in enumerate(labels):
        pdf.set_fill_color(*NAVY)
        pdf.set_draw_color(*TEAL)
        pdf.set_line_width(0.25)
        pdf.rect(bx, y, bw, bh, style="FD")
        pdf.set_xy(bx, y + 2)
        set_font(pdf, "B", 7.5)
        pdf.set_text_color(*WHITE)
        pdf.cell(bw, 4.5, lab, align="C")
        if i < len(labels) - 1:
            mx = bx + bw / 2
            ay = y + bh
            by = y + bh + gap
            pdf.set_draw_color(*TEAL)
            pdf.set_line_width(0.4)
            pdf.line(mx, ay, mx, by - 1)
            pdf.line(mx, by - 1, mx - 1.2, by - 2.5)
            pdf.line(mx, by - 1, mx + 1.2, by - 2.5)
        y += bh + gap
    pdf.set_y(y + 3)


def roadmap_card(
    pdf: FPDF, x: float, y: float, w: float, h: float, bg, label: str, title: str, body: str
) -> None:
    pad = 4.0
    inner = w - 2 * pad

    pdf.set_fill_color(*bg)
    pdf.rect(x, y, w, h, style="F")

    pdf.set_xy(x + pad, y + 4)
    set_font(pdf, "B", 7)
    pdf.set_text_color(*(WHITE if bg != TEAL else NAVY))
    pdf.cell(inner, 3, label.upper())

    pdf.set_xy(x + pad, y + 9)
    set_font(pdf, "B", 10)
    pdf.set_text_color(*WHITE)
    pdf.multi_cell(inner, 5, title)

    pdf.set_x(x + pad)
    set_font(pdf, "", 7.5)
    pdf.set_text_color(*WHITE)
    pdf.multi_cell(inner, 3.8, body)


def value_row(pdf: FPDF, title: str, desc: str) -> None:
    w = epw(pdf)
    desc_w = w - 52
    desc_h = text_h(pdf, desc_w, 4.2, desc)
    box_h = max(11.0, desc_h + 6)
    ensure_space(pdf, box_h + 2)

    y0 = pdf.get_y()
    pdf.set_fill_color(*LIGHT)
    pdf.rect(pdf.l_margin, y0, w, box_h, style="F")
    pdf.set_fill_color(*TEAL)
    pdf.rect(pdf.l_margin, y0, 2, box_h, style="F")

    pdf.set_xy(pdf.l_margin + 5, y0 + (box_h - 5) / 2)
    set_font(pdf, "B", 9)
    pdf.set_text_color(*NAVY)
    pdf.cell(46, 5, title)

    pdf.set_xy(pdf.l_margin + 52, y0 + 3)
    set_font(pdf, "", 8)
    pdf.set_text_color(*SLATE)
    pdf.multi_cell(desc_w, 4.2, desc)
    pdf.set_y(y0 + box_h + 2)


def page_cover(pdf: WireStackPDF) -> None:
    pdf.add_page()
    pdf.set_fill_color(*NAVY)
    pdf.rect(0, 0, PAGE_W, PAGE_H, style="F")
    pdf.set_fill_color(*TEAL)
    pdf.rect(PAGE_W - 55, 0, 60, 100, style="F")

    logo = ASSETS / "wirestack-icon.png"
    if logo.exists():
        pdf.image(str(logo), x=MARGIN, y=28, w=22)

    pdf.set_xy(MARGIN, 58)
    set_font(pdf, "B", 32)
    pdf.set_text_color(*WHITE)
    pdf.cell(epw(pdf), 12, "WireStack")

    pdf.set_x(MARGIN)
    set_font(pdf, "", 14)
    pdf.multi_cell(epw(pdf), 7, "Company Overview\n& New Team Onboarding")

    y_rule = pdf.get_y() + 4
    pdf.set_draw_color(*TEAL)
    pdf.set_line_width(0.8)
    pdf.line(MARGIN, y_rule, PAGE_W - MARGIN, y_rule)

    pdf.set_xy(MARGIN, y_rule + 6)
    set_font(pdf, "I", 12)
    pdf.set_text_color(*TEAL)
    pdf.cell(epw(pdf), 6, "See the wire. Own the stack.")

    pdf.set_xy(MARGIN, y_rule + 18)
    set_font(pdf, "", 10)
    pdf.set_text_color(180, 195, 210)
    pdf.multi_cell(
        epw(pdf),
        5.5,
        "Your onboarding guide: who we are, what vigil does, how you contribute, "
        "and where WireStack is headed.",
    )

    pdf.set_y(272)
    set_font(pdf, "", 8)
    pdf.set_text_color(*SLATE)
    pdf.cell(0, 5, "Confidential  |  2026  |  Internal onboarding", align="C")


def page_who_we_are(pdf: WireStackPDF) -> None:
    pdf.add_page()
    header_bar(pdf, "Who We Are", "Built at the wire. Grown by builders.")

    two_panels(
        pdf,
        [
            ("MISSION", "B", 8, TEAL),
            (
                "Give every host clear network visibility - edge to rack - "
                "with software engineers trust.",
                "",
                9,
                WHITE,
            ),
        ],
        [
            ("VISION", "B", 8, NAVY),
            (
                "The observability layer for teams at the wire, grown by people "
                "who run and extend it daily.",
                "",
                9,
                NAVY,
            ),
        ],
    )

    paragraph(
        pdf,
        "WireStack exists because infrastructure moves faster than observability tools. "
        "We ship C99 systems code: lean daemons, honest packet-to-database paths, "
        "and CLIs that respect operators.",
    )
    callout(
        pdf,
        "Our promise to you",
        "You are helping build WireStack, not just joining it. Your reviews, fixes, "
        "and designs shape the company as much as the codebase.",
    )


def page_problem_grow(pdf: WireStackPDF) -> None:
    pdf.add_page()
    header_bar(pdf, "The Problem + Our Approach")

    paragraph(pdf, "Teams today face the same four failures:", size=10)
    bullets(
        pdf,
        [
            "Blind spots at L2-L7: spikes, misconfig, silent failures.",
            "Split tooling: capture in one place, metrics elsewhere, weak history.",
            "Heavy agents that burn CPU without explaining traffic.",
            "Alerts that land after the outage, not during it.",
        ],
    )

    callout(
        pdf,
        "WireStack's answer",
        "vigil: a lean host daemon, packet-to-SQLite pipeline, and vigil-cli for live "
        "control. No black boxes.",
    )

    paragraph(pdf, "How we grow organically:", size=10)
    bullets(
        pdf,
        [
            "Product-led: vigil proves value on day one.",
            "Builder culture: real C, threads, sockets, SQL.",
            "User-driven roadmap: pain in the field becomes priority in the repo.",
            "Honest quality: document gaps; fix them in the open.",
            "Sustainable pace: solid architecture beats demo theater.",
        ],
    )


def page_vigil_product(pdf: WireStackPDF) -> None:
    pdf.add_page()
    header_bar(pdf, "vigil - Flagship Product", "Network monitoring daemon + CLI")

    paragraph(
        pdf,
        "vigil captures live packets, tracks connections, classifies protocols, fires "
        "threshold alerts, stores history in SQLite, and exposes vigil-cli on "
        "/tmp/vigil.sock for status, stats, and control.",
    )

    pdf.set_x(pdf.l_margin)
    set_font(pdf, "B", 10)
    pdf.set_text_color(*NAVY)
    pdf.cell(epw(pdf), 5, "Data flow")
    advance(pdf, 2)

    arch_diagram(pdf)

    paragraph(
        pdf,
        "Tech stack: C99, libpcap, SQLite3, pthread. Linux primary; Windows via Npcap.",
        size=9,
    )


def page_capabilities(pdf: WireStackPDF) -> None:
    pdf.add_page()
    header_bar(pdf, "vigil Capabilities", "Phases 1-8 on one platform")

    cards = [
        ("P1", "Packet capture", "libpcap live ingest"),
        ("P1", "Traffic stats", "PPS/BPS counters"),
        ("P1", "SQLite store", "Stats and alert history"),
        ("P2", "Connections", "5-tuple TCP tracking"),
        ("P3", "Protocols", "L3/L4 + port hints"),
        ("P4", "Alerts", "Thresholds + scripts"),
        ("P5", "Reports", "Periodic summaries"),
        ("P5", "Log rotation", "Size-capped logs"),
        ("P6", "Config reload", "INI without restart"),
        ("P7", "Multi-interface", "Parallel capture"),
        ("P8", "Anomaly", "Baseline scoring"),
        ("CLI", "vigil-cli", "status, stats, stop, ..."),
    ]

    cols = 3
    gap = 3.0
    card_w = (epw(pdf) - gap * (cols - 1)) / cols
    x0 = pdf.l_margin
    y0 = pdf.get_y()

    row_heights: list[float] = []
    for i in range(0, len(cards), cols):
        row = cards[i : i + cols]
        row_heights.append(max(cap_card_height(pdf, card_w, ti, de) for _, ti, de in row))

    total_h = sum(row_heights) + gap * (len(row_heights) - 1)
    ensure_space(pdf, total_h)

    y_row = pdf.get_y()
    for ri, i in enumerate(range(0, len(cards), cols)):
        row = cards[i : i + cols]
        row_h = row_heights[ri]
        for j, (ph, ti, de) in enumerate(row):
            x = x0 + j * (card_w + gap)
            cap_card(pdf, x, y_row, card_w, row_h, ph, ti, de)
        y_row += row_h + gap
    pdf.set_y(y_row + 2)


def page_journey(pdf: WireStackPDF) -> None:
    pdf.add_page()
    header_bar(pdf, "Your Journey", "First week on vigil")

    paragraph(
        pdf,
        "You work on the same repo we ship. End week one with a running daemon and "
        "a vigil-cli command you trust.",
    )

    for i, (title, desc) in enumerate(
        [
            ("Orient", "make, vigil.conf, sudo ./vigil, ./vigil-cli status"),
            ("Understand", "vigil/DEVELOPERS.md + wirestack-vigil-spec.md"),
            ("Implement", "Ship a spec phase or harden a module"),
            ("Review", "Root-cause a defect with repro notes"),
            ("Ship", "PR with tests and a clear why"),
        ],
        1,
    ):
        step_card(pdf, i, title, desc)

    paragraph(
        pdf,
        "Docs: vigil/README.md | vigil/DEVELOPERS.md | vigil/PRODUCT.md | "
        "wirestack-vigil-spec.md",
        size=8,
    )


def page_roadmap(pdf: WireStackPDF) -> None:
    pdf.add_page()
    header_bar(pdf, "Roadmap", "Where vigil ends is not where WireStack stops")

    y0 = pdf.get_y()
    gap = 3.0
    cw = (epw(pdf) - 2 * gap) / 3

    bodies = [
        "Daemon, CLI, SQLite, alerts, multi-iface, anomaly. Harden for Linux edge.",
        "systemd packaging, pilot deployments, vigil self-metrics.",
        "WireStack Cloud option, plugin SDK, partner integrations.",
    ]

    def card_height(body: str) -> float:
        inner = cw - 8
        return 8 + 6 + text_h(pdf, inner, 3.8, body) + 6

    ch = max(card_height(b) for b in bodies)
    ensure_space(pdf, ch + 4)

    y0 = pdf.get_y()
    roadmap_card(pdf, pdf.l_margin, y0, cw, ch, NAVY, "now", "vigil today", bodies[0])
    roadmap_card(pdf, pdf.l_margin + cw + gap, y0, cw, ch, BLUE, "near", "pilots", bodies[1])
    roadmap_card(
        pdf, pdf.l_margin + 2 * (cw + gap), y0, cw, ch, TEAL, "long", "platform", bodies[2]
    )
    pdf.set_y(y0 + ch + 6)

    callout(
        pdf,
        "Growth principle",
        "New products follow a proven vigil foundation. Roadmap bets serve users.",
    )


def page_values_cta(pdf: WireStackPDF) -> None:
    pdf.add_page()
    header_bar(pdf, "Values")

    for title, desc in [
        ("Clarity over noise", "Explain it at the wire or do not ship it."),
        ("Ownership over tickets", "Outcomes, not checkboxes."),
        ("Courage over comfort", "Surface hard problems early."),
        ("Craft over churn", "C99, tests that matter."),
        ("Trust over hierarchy", "Best idea wins."),
    ]:
        value_row(pdf, title, desc)

    advance(pdf, 4)
    w = epw(pdf)
    y0 = pdf.get_y()
    box_h = 28.0
    ensure_space(pdf, box_h + 10)

    y0 = pdf.get_y()
    pdf.set_fill_color(*NAVY)
    pdf.rect(pdf.l_margin, y0, w, box_h, style="F")

    pdf.set_xy(pdf.l_margin, y0 + 5)
    set_font(pdf, "B", 14)
    pdf.set_text_color(*WHITE)
    pdf.cell(w, 7, "You are WireStack now.", align="C")

    pdf.set_xy(pdf.l_margin, y0 + 14)
    set_font(pdf, "", 10)
    pdf.set_text_color(*TEAL)
    pdf.cell(w, 6, "Clone the repo. Run vigil. Ship what matters.", align="C")

    pdf.set_y(y0 + box_h + 6)
    pdf.set_x(pdf.l_margin)
    set_font(pdf, "I", 8)
    pdf.set_text_color(*SLATE)
    pdf.multi_cell(w, 4.5, "Questions? Ask your team lead or cohort channel.", align="C")


def build_pdf() -> None:
    sys.path.insert(0, str(ASSETS))
    from generate_logos import main as gen_logos

    gen_logos()

    pdf = WireStackPDF(orientation="P", unit="mm", format="A4")
    pdf.set_margins(MARGIN, MARGIN, MARGIN)
    pdf.set_auto_page_break(auto=False)

    page_cover(pdf)
    page_who_we_are(pdf)
    page_problem_grow(pdf)
    page_vigil_product(pdf)
    page_capabilities(pdf)
    page_journey(pdf)
    page_roadmap(pdf)
    page_values_cta(pdf)

    pdf.output(str(OUT))
    print(f"Wrote {OUT} ({pdf.page_no()} pages)")


if __name__ == "__main__":
    build_pdf()
