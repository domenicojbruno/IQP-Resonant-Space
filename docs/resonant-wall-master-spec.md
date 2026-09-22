# Resonant Wall — Master Node Simulation Spec

> Companion doc for the master-node simulation (artifact: "Resonant Wall").
> Purpose: capture the wall topology, motor addressing, and command semantics
> so this pairs cleanly with the peripheral (ESP32) code once it's written.

---

## 1. System overview

- **3 ESP32 boards**, each an independent peripheral driving one horizontal layer of the wall.
- **5 linear actuators (motors)** total per the physical layout: 5 top, 3 mid, 5 bottom (13 motors).
- **Motor driver:** BTS7960 H-bridge per motor (direction + PWM speed).
- **Master** decides *which motor gets which command* from a tracked point on the wall face.
- **Position sensing (real system):**
  - 4× VL53L0X TOF sensors — detect people crossing a threshold in the hallway (occupancy trigger).
  - 2× overhead cameras — track hand/limb position in space, mapped to a point on the wall face.
- **People move parallel to the wall**; the wall extends/retracts **perpendicular** to the hallway.

The simulation stands in for the camera stage: a cursor over the hex field == the mapped hand position. Everything downstream of that point (per-motor targets, thresholds, command emission) is the logic the master will actually run.

---

## 2. Wall topology — flat-top hex honeycomb

Motors are hexagonal and packed in a honeycomb. Rows are **zig-zagged**, not straight:
the middle layer nests into the gaps of the top and bottom layers.

```
   M1,1        M1,3        M1,5          <- Board A, "high" hexes
        M1,2        M1,4                 <- Board A, "low"  hexes
   M2,1        M2,2        M2,3          <- Board B, nested one step below A's high hexes
        M3,2        M3,4                 <- Board C, "high" hexes (raised into gaps)
   M3,1        M3,3        M3,5          <- Board C, "low"  hexes
```

- **Board A (top, 5):** cols 1,3,5 high; cols 2,4 dropped half a hex height.
- **Board B (mid, 3):** sit one full hex-height below A, each centered under a *high* A hex
  (M2,1 under M1,1, M2,2 under M1,3, M2,3 under M1,5).
- **Board C (bottom, 5):** mirrors A — cols 1,3,5 low; cols 2,4 raised into the gaps.

### Geometry constants (as used in the sim)

| Quantity | Value | Meaning |
|---|---|---|
| `R` | 44 | hex corner radius (arbitrary sim units) |
| column spacing | `1.5 * R` | horizontal distance between adjacent hex columns |
| zig-zag offset | `√3 * R / 2` | vertical half-step within a board |
| board spacing | `√3 * R` | vertical distance between stacked boards |
| neighbor center-to-center | `√3 * R` | adjacent hexes share a full edge |

Flat-top hex vertices are at 0°, 60°, 120°, 180°, 240°, 300°.

### Motor center coordinates (sim units, origin at M1,1)

| Motor | Board | x | y |
|---|---|---|---|
| M1,1 | A | 0   | 0     |
| M1,2 | A | 66  | 38.1  |
| M1,3 | A | 132 | 0     |
| M1,4 | A | 198 | 38.1  |
| M1,5 | A | 264 | 0     |
| M2,1 | B | 0   | 76.2  |
| M2,2 | B | 132 | 76.2  |
| M2,3 | B | 264 | 76.2  |
| M3,1 | C | 0   | 152.4 |
| M3,2 | C | 66  | 114.3 |
| M3,3 | C | 132 | 152.4 |
| M3,4 | C | 198 | 114.3 |
| M3,5 | C | 264 | 152.4 |

> These are relative positions for the falloff math. When the camera is added,
> the mapping is: camera-space hand position → wall-face point in this same
> coordinate frame. Nothing else in the pipeline changes.

---

## 3. Motor addressing

- Address form: **`M<row>,<col>`** where row ∈ {1,2,3} = {Board A, Board B, Board C}
  and col is left-to-right by x-position (1..5 for A/C, 1..3 for B).
- **Board = row.** Each ESP32 owns exactly one row, so a peripheral only ever
  cares about its own `col` index.
- ⚠️ **Numbering caveat:** in the sim, col is assigned strictly by x-position.
  For Board C that means `M3,1` is the leftmost *low* hex. If the physical wiring
  numbers motors by high/low pairing instead, remap col at the peripheral.

---

## 4. Actuation model & command semantics

Each motor has an **extension state** `ext ∈ [0, 1]`:

- `ext = 1.0` → **fully extended** (wall pushed out to max). Rendered **white**.
- `ext = 0.0` → **fully retracted** (wall pulled inward). Rendered **bright red**.
- Intermediate `ext` → interpolated white→pink→red.

**Retraction amount** `retract = 1 - ext`. This drives both color and PWM.

### Per-motor target from the tracked point

For a tracked point `(px, py)` and motor at `(cx, cy)`:

```
d2        = (px-cx)^2 + (py-cy)^2
influence = exp( -d2 / (2 * sigma^2) )   // Gaussian falloff
target    = 1 - influence                // 1 = extended, 0 = fully retracted at the point
ext      += (target - ext) * smoothing   // slew toward target each tick
```

- `sigma` = **reactivity radius** — how far the recoil spreads to neighbors.
  Directly under the hand → full retract; surrounding hexes retract less
  aggressively (the "surrounding sections respond but not as aggressively" behavior).
- `smoothing` = **response speed** — first-order slew rate per tick (0..1).
- When hallway is **unoccupied** (no TOF threshold crossed) → all `target = 1`
  (wall parks at max extension, camera tracking idle).

### Command emission (what the master sends to peripherals)

Direction is derived from the H-bridge convention:

| Condition | Command | Meaning |
|---|---|---|
| motor crosses **retract threshold** (`ext ≤ 0.12`) | `REVERSE` | pull wall inward |
| retracted motor crosses **extend threshold** (`ext ≥ 0.85`) | `FORWARD` | push wall back out |
| between thresholds | (engaging, no new command) | slewing |

- **Hysteresis** (0.12 / 0.85) prevents FORWARD/REVERSE chatter when a hand hovers.
- **PWM** = `round(retract * 255)` — deeper retract = faster/harder drive (0..255 for `analogWrite`/LEDC).
- Per-tick **Dir** field (FWD/REV/HOLD) reflects instantaneous motion for telemetry;
  the emitted **command** (FORWARD/REVERSE) is the latched threshold event.

Example emitted command (log format in the sim):

```
Board B → M2,2 CMD:REVERSE PWM:214
Board A → M1,3 CMD:FORWARD PWM:242
```

---

## 5. Master → peripheral interface (to be finalized with peripheral code)

The sim currently logs commands as human-readable lines. For the real link,
the master should send a compact frame per motor state change. **Not yet
finalized** — decide alongside the peripheral code. Candidate fields:

- `board` (A/B/C) — routes to the right ESP32 (or is implicit per serial channel).
- `col` — which motor on that board.
- `cmd` — FORWARD | REVERSE | STOP.
- `pwm` — 0..255.

Open questions for the peripheral side:

1. **Transport:** single master UART fanned out, vs. per-board channels / addressed bus.
2. **Push vs. poll:** master pushes on threshold events, or peripherals poll for target `ext`.
3. **Target vs. command:** send discrete FORWARD/REVERSE+PWM (current sim model),
   or stream continuous target `ext` and let the peripheral close the loop with its own TOF feedback.
4. **Closed-loop position:** each actuator's real position comes from its board's TOF sensors —
   the peripheral likely runs a local position controller; the master then sends *setpoints*, not raw PWM.
5. **STOP / limits:** end-stop handling and what the master sends when a target is reached.

---

## 6. Simulation ↔ real-world seam

The single clean integration point is the **tracked point**:

- Sim: `pointer.x / pointer.y` come from the mouse in SVG coordinate space.
- Real: replace with the camera-mapped hand position in the same frame
  (Section 2 coordinates). Two cameras → fuse to one point (or two points; the
  falloff math already supports multiple influence sources by summing/maxing influence).

Everything after the point — falloff, slew, thresholds, command emission — is
identical between sim and hardware, which is the whole reason the sim exists:
it lets the CV mapping be validated against known motor commands before any
peripheral firmware runs.
