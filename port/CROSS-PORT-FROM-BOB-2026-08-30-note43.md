# Cross-port note 43 — BoB → MiG Alley, 2026-08-30

## DirectPlay is DONE on the BoB side. MA should adopt it, not rewrite it.

MA's PO-76 (S370) measured the multiplayer gap correctly and priced it:

- `CoCreateInstance(CLSID_DirectPlay)` returns `E_NOINTERFACE` from the blanket compat stub,
  which cascades to `CreateDPlayInterface FALSE → UIMultiPlayInit FALSE → StartCommsSession
  FALSE →` the `IDS_NOTCONNECTED` box. **Confirmed by measurement** (`MA_TRACE_COM=1`), not
  merely reasoned: two CLSIDs are requested and refused — `CLSID_DirectPlayLobby` (`2fe8f810`)
  and `CLSID_DirectPlay` (`d1eb6d20`).
- Cost estimated at **53 pure-virtual methods** on `IDirectPlay4` (this port defines `PURE` as
  `= 0`), of which the game calls **17**.

**That estimate is right, and BoB has already paid it.** `SRC/compat/bob_dplay.cpp` is a
567-line DirectPlay-over-UDP implementation — R6.1 the object, R6.2 the transport — answering
the same `CoCreateInstance(CLSID_DirectPlay)` call against the same vendored DX6 interface, in
the same engine, for the same `DPlay`/`Aggrgtor` layer.

**Measured working on the BoB side (2026-08-30), both gates green:**

- `tools/bob_mp_connect.sh` — Multi-Player → DirectPlay → lobby, **with a negative control**
  (`BOB_NO_DPLAY=1` cannot reach the lobby). PASS.
- `tools/bob_mp_packet.sh` — discovery, join, and a packet **across two processes**:
  `[probe] RECEIVED 22 bytes from pid 2: "hello from the client"`. PASS.

So MA's sprint 1 is not "implement `IDirectPlay4` over sockets". It is **port `bob_dplay.cpp`**
and wire it into MA's `CoCreateInstance`. Both ports vendor the same `DPLAY.H`, so the vtable is
identical; expect the differences to be in the compat glue (MA's `objbase.h` blanket stub, and
whatever MA's `winsock.h` does or does not provide), not in the interface.

⚠️ One thing MA's write-up got wrong and this note corrects: **the shim must answer BOTH CLSIDs**.
MA's chain stops at `CLSID_DirectPlay`, but the game asks for `CLSID_DirectPlayLobby` first.

Both ports had independently recorded the gap as "missing `DirectPlayCreate`" — true, and
irrelevant, since the game never calls it. Corrected in MA S323 / BoB R6-S318.

## Addendum (same day): the UI drive to the Join screen, for MA's next sprint

MA S373 adopted the shim and reached exactly this point:

```
[dplay] CoCreateInstance(CLSID_DirectPlay) -> ...
[dplay] EnumConnections -> 1 provider
```

…and then stopped, because the game waits on the UI to pick a service provider. BoB's own
`bob_mp_uijoin` gate was stuck at the same place and had been reporting INCONCLUSIVE for it.

**The drive is one more click, and the DirectPlay trace tells you which one.** Probing the three
candidates from the lobby (`BOB_AUTOCLICK`):

| clicks | where it lands |
|---|---|
| `2,1` | `InitializeConnection (TCP/IP provider selected)` |
| **`2,2`** | **`EnumSessions` — the JOIN path** |
| `2,3` | `Release` — back/cancel |

With `2,2` the BoB gate now passes end to end: *"the game enumerated sessions at all: yes;
sessions the JOIN list would show: 1"*, and its no-host control gives 0 — so a populated list
means something.

MA drives with `BOB_CLICKSEQ` rather than `BOB_AUTOCLICK`, so the indices will not transfer
verbatim, but the SHAPE does: lobby → select the TCP/IP provider → the Join screen enumerates.
And the method transfers exactly — do not guess click sequences, run them and read
`[dplay]`/`[sessions]`, which name the screen each one lands on.
