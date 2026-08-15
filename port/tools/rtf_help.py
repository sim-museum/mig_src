#!/usr/bin/env python3
"""port/tools/rtf_help.py — extract the game's documentation from its own RTF source (S114, PO-10).

The compiled help (English/TEXT/MIG.HLP) is Hall-compressed, and two sprints failed to decode its
topic text: S99 solved four of five stages, S112 eliminated an 800-candidate opcode family against
the title oracle, and neither would ship a decoder that emits plausible nonsense.

The PO then pointed at the tree, and the answer was sitting in it: **the help SOURCE is in the
repo**. `SRC/<LANG>/HELP/MIG.RTF` is the WinHelp RTF that MIG.HLP was compiled FROM, `MIG.HPJ`
confirms the compression (`COMPRESS=12 Hall Zeck`), and `MIG.HM` maps the `HID_*`/`HIDD_*` symbols
to the context numbers the game passes to WinHelp. So the documentation does not have to be
decompressed at all -- it has to be read.

This writes a flat, inspectable text file the port loads at runtime:

    #TOPIC <context symbol>|<title>
    <body line>
    ...

  python3 port/tools/rtf_help.py SRC/ENGLISH/HELP/MIG.RTF > mig_help.txt
"""
import re, sys

def strip_rtf(s):
    """RTF -> plain text, enough for help body copy: drop control words, groups we do not want,
    keep paragraph breaks. Deliberately small: this runs offline and its output is committed and
    eyeballed, so a full RTF reader would be more machinery than the job needs."""
    out = []
    i, n = 0, len(s)
    skip_depth = 0          # inside a group we are discarding (footnotes, stylesheet, fonttbl...)
    depth = 0
    hidden = 0              # \v ... \v0 : WinHelp hides the JUMP TARGET after a hotspot's text,
                            # which is why an unfiltered strip reads "...can be designedHIDD_MISSION-
                            # GENERATION". The target is markup, not prose.
    hstack = []             # ...and RTF formatting is SCOPED TO ITS GROUP: a \v inside {...} ends
                            # at the closing brace. Without saving/restoring it, one hotspot hides
                            # the whole rest of the topic -- which is exactly what the first cut did
                            # (the Introduction topic stopped mid-sentence at item 5).
    while i < n:
        c = s[i]
        if c == '\\':
            m = re.match(r'\\([a-zA-Z]+)(-?\d+)? ?', s[i:])
            if m:
                word, arg = m.group(1), m.group(2)
                i += m.end()
                if skip_depth:
                    continue
                if word == 'v':
                    hidden = 0 if arg == '0' else 1
                    continue
                if hidden:
                    continue
                if word in ('par', 'line'):
                    out.append('\n')
                elif word == 'tab':
                    out.append('\t')
                elif word in ('fonttbl', 'colortbl', 'stylesheet', 'info', 'footnote', 'pict',
                              'header', 'footer', 'bkmkstart', 'bkmkend', 'xe', 'tc'):
                    skip_depth = depth      # discard to the end of this group
                elif word == 'u':           # \uN unicode escape
                    if arg:
                        try: out.append(chr(int(arg) & 0xFFFF))
                        except ValueError: pass
                continue
            if i + 1 < n and s[i+1] in "\\{}":
                if not skip_depth: out.append(s[i+1])
                i += 2; continue
            if s[i:i+2] == "\\'":
                try:
                    if not skip_depth: out.append(chr(int(s[i+2:i+4], 16)))
                except ValueError: pass
                i += 4; continue
            i += 1; continue
        if c == '{':
            depth += 1; hstack.append(hidden); i += 1; continue
        if c == '}':
            depth -= 1
            if hstack: hidden = hstack.pop()
            if skip_depth and depth < skip_depth: skip_depth = 0
            i += 1; continue
        if not skip_depth and not hidden:
            out.append(c)
        i += 1
    txt = ''.join(out)
    txt = re.sub(r'[ \t]+', ' ', txt)
    txt = re.sub(r'\n{3,}', '\n\n', txt)
    return '\n'.join(l.strip() for l in txt.split('\n')).strip()


def topics(rtf):
    """WinHelp RTF: topics are separated by \\page; each carries footnotes -- $ title, # context."""
    out = []
    for chunk in rtf.split('\\page'):
        if '\\footnote' not in chunk: continue
        mt = re.search(r'\$\{\\footnote.*?\$\}\s*([^}]*)\}', chunk, re.S)
        mc = re.search(r'#\{\\footnote.*?#\}\s*([^}\s]*)', chunk, re.S)
        title = strip_rtf(mt.group(1)) if mt else ''
        ctx   = (mc.group(1).strip() if mc else '')
        ctx   = ctx.split('\\')[0].strip()   # the capture can run into the next control word
        body  = strip_rtf(chunk)
        # the footnote text leads the body; drop a leading copy of the title
        if title and body.startswith(title):
            body = body[len(title):].lstrip()
        out.append((ctx, title, body))
    return out


def main():
    if len(sys.argv) < 2: raise SystemExit(__doc__)
    rtf = open(sys.argv[1], 'rb').read().decode('latin-1')
    # bare CR/LF in an RTF file are layout, not text -- they land mid-word otherwise ("Yo\nu are")
    rtf = rtf.replace('\r', '').replace('\n', '')
    n = 0
    for ctx, title, body in topics(rtf):
        if not title and not body: continue
        n += 1
        print(f"#TOPIC {ctx}|{title}")
        for line in body.split('\n'):
            line = line.strip()
            if not line: continue
            # the footnote markers (K keyword, $ title, # context) leave a "k$#Title" line, and the
            # Symbol-font bullets leave lines of "*.*()" -- both are markup residue, not text.
            if 'k$#' in line or line.startswith('$#') or line.startswith('#'): continue
            # Symbol-font bullet glyphs come through as runs of "*.*()"; a real sentence has far
            # more letters than punctuation.
            letters = sum(ch.isalpha() for ch in line)
            if letters < 3 or letters * 2 < len(line): continue
            print(line)
        print()
    print(f"# {n} topics", file=sys.stderr)

if __name__ == '__main__':
    main()
