# Sprint 142 — "The gold video found it" (PO-36) — ✅ CLOSED 2026-08-16 (goal MET)

**Planned 2026-08-16, acting on S141's finding that our references may encode defects.**
**Sprint Goal:** audit the front-end screens against the gold videos, and fix what that finds.

| Story | Pts | Result |
|---|---|---|
| S142-1 pull the gold frames for our screens | 2 | ✅ both videos montaged and sampled |
| S142-2 the D.I.S. briefing window is missing | 5 | ✅ the paint walk was one level too shallow |
| S142-3 confirm nothing else moved | 1 | ✅ full suite green |

## The method, which is the point

S141 ended by noting that the parity references were captured from this port and can therefore
encode its defects. So this sprint compared the screens against the **gold videos** instead of
against our own captures — and the very first frame examined answered an open PO report.

The gold frame at t=18s of `260814_mig_alley_start_campaign_and_exit.mp4` shows the campaign map
with **two** D.I.S. windows: the dialog itself top-left, and a second one at the **bottom-left**
carrying the mission briefing:

> *The NKAF is posing a serious threat. On June 27, 2 Yaks strafed Kimpo airfield, hitting the
> tower, a fuel dump and a C54 transport plane.*
> *MISSION BRIEF: You are ordered to perform an armed-recon mission along the main rail line
> leading into Seoul from Munsan-ni.*

That is the PO's *"DIS dialog — no briefing text"*. Not missing text: a **missing window**.

## The enumeration was one level too shallow

`CDIS::OnInitDialog` ends by calling `OnClickedViewnotes()`, which is:

```c
RDialog* d = MakeTopDialog(Place(POSN_MIN,POSN_MAX,10,-10),
                           DialBox(FIL_MAP_INTELLIGENCE, new CDis_Note(text)));
LogChild(0, d);
```

— a **top-level dialog**, placed bottom-left (which is exactly where gold shows it), logged
against **the D.I.S. dialog**. The port's `ma_map_paint_oob` walks the logged children of the two
TOOLBARS, and then recurses each one's `fchild`/`dchild`/`sibling` tree. A dialog logged against
another *dialog* is in neither place, so it was constructed on every open and never painted:
`[oob] painted 1 open dialog(s)`.

The walk now also visits each painted dialog's own logged children, with the same de-duplication:
`[oob] painted 2 open dialog(s)`, and the briefing appears at the map's bottom-left, matching gold
word for word.

**This is S106's finding one level deeper** — there, the post-mission Mission Results panel was
logged against the debrief toolbar while the walk knew only about the main one. Recorded then as
*"the tree was fine and the ENUMERATION was too narrow"*; it was too narrow again, in the same
function, for the same reason. A dialog can be logged anywhere, so the walk has to follow the
logging, not a fixed pair of roots.

## Evidence

`port/ref/native/dis_briefing.png` (ours) beside `port/ref/gold/dis_briefing_gold.png` (the gold
video): same window, same position, same text.

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map icon click · help click · dialog scroll.

## Next from the same method

The title screen's black menu box (PO-35) came from this comparison too and is still open. The
remaining parity references — quickmission, prefs_3d, prefs_others — have not yet been read
against gold.
