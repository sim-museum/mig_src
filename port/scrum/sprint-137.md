# Sprint 137 — "A cap that silently dropped the handler" (PO-30) — ✅ CLOSED 2026-08-16 (goal MET)

**Planned 2026-08-16 (PO: continuous sprints on the campaign GUI).**
**Sprint Goal:** the map's red/blue filter buttons filter the map.

| Story | Pts | Result |
|---|---|---|
| S137-1 does the click reach the button? | 2 | ✅ it always did — that was the misleading part |
| S137-2 why does no handler run? | 3 | ⭐ the range registrar refused the span |
| S137-3 why does the map still not change? | 3 | ⭐ the port fired without toggling the button |

The PO: *"red and blue buttons at upper right do nothing — they're supposed to filter map icons."*

## The click was never the problem

`MA_TRACE_CLICK` showed the click routing correctly on the first attempt:

```
[tbclick] id=2245 rect=(1595,4,24,24) -> fire
```

which is exactly the half-truth that lets a defect like this survive. Two things were broken
*behind* the fire, and **either one alone leaves the map unchanged**, so fixing one and stopping
would have proved nothing.

## ⭐ 1. The range registrar refused the span

`CMapFilters` routes all 30 buttons through one handler:

```c
ON_EVENT_RANGE(CMapFilters, 1, 9999, 1 /* Clicked */, OnClickedFilter, VTS_I4)
```

S87 implemented `ON_EVENT_RANGE` by **expanding the span into one entry per id**, with a guard:

```c
if (idLast - idFirst > 4096) return;   /* refuse an absurd span rather than eat memory */
```

The span is 9998. So the registration was silently discarded and `ma_evt_fire` had nothing to
find — while `CBases` (2420..2478) and `CSqdnlist` (2350..2397) registered fine, which is why the
mechanism looked healthy. The fix is to **store the range as a range** and match on it, which
removes the reason for the cap instead of raising it.

*This is the trace-cap lesson in a new costume: a limit chosen for safety that silently drops
legitimate work, then presents as "the feature does nothing". Booked six times against traces;
this is the first time it was in a data structure.*

## ⭐ 2. The port fired the click without toggling the button

`CRButtonCtrl::OnLButtonUp` is, in full:

```c
m_LButtonDown=FALSE;
m_bPressed=!m_bPressed;      // <- the port never did this
...then fires Clicked
```

and the handler asks the button what state it is **now** in:

```c
CRButton* but=(CRButton*)GetDlgItem(id);
bool pressed=(but->GetPressed()==1);
if (pressed) Save_Data.mapfilters |= ...; else Save_Data.mapfilters %= ...;
```

so every filter click read "not pressed" and asked to clear a filter that was already clear. The
map was being told to change to the state it was already in. The toggle now happens in the click
path, where the control would have done it, and it also gives every toolbar button its pressed
artwork — the control picks that by the same flag.

## Evidence, on the map rather than the log

`port/ref/native/map_filters.png` — the same campaign map after one click on the red "all"
filter: the full set of enemy icons (factories, trucks, supply depots, airfields) appears.
**58,771 map pixels change.**

New gate **`port/map_filter.sh`** asserts exactly that: it locates the button by clicking along
the row and reading back the id the toolbar reports (the row is right-aligned, so its x moves
with the resolution), clicks it, and requires the MAP to change. Asserting on the log would have
passed throughout this defect's life.

*Gate-writing note, cost 8 minutes: the gate called `gl-lock` inside a script that is itself run
under `gl-lock`. The nested lock blocks until the timeout kills the run, which presents as an
empty log and a gate that "fails" without ever starting the game.*

## Gates

parity 5/5 byte-identical · sweep 9 OPEN/0 CRASH · map icon click · map drag · sysbox exit ·
help click · **map filter (new)**.
