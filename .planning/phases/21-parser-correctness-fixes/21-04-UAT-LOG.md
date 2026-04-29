---
plan: 21-04
requirement: LFXC-02
status: pass
tested_on: UE 5.7.4, Windows 11
tested_at: 2026-04-29T00:00:00
tester: Pavel Dyatlov
---

# Phase 21 / LFXC-02 — Human UAT Log

## Subjects

| File                                       | Effects Applied             | ColorSpace |
|--------------------------------------------|-----------------------------|------------|
| Source/PSD2UMG/Tests/Fixtures/Effects.psd  | Color Overlay, Drop Shadow  | RGB (=0)   |

## Photoshop Reference Values

| Subject     | Color Overlay HEX | Drop Shadow HEX | Drop Shadow Offset (Px) |
|-------------|-------------------|-----------------|--------------------------|
| Effects.psd | (as authored)     | (as authored)   | (as authored)            |

## Observations

### Color Overlay Hue
Verified on real UE 5.7 host project. Color overlay hue matches Photoshop-authored RGB values within tolerance.

### Drop Shadow Hue
Drop shadow color matches Photoshop-authored RGB values within tolerance.

### Drop Shadow Offset / Direction
Drop shadow direction and offset match Photoshop reference.

## Screenshots

- ![Photoshop subject 1](21-04-photoshop-effects.png)
- ![UMG subject 1](21-04-umg-effects.png)

## Verdict

PASS — RGB color overlay and drop shadow render correct hue on UE 5.7 host project after Phase 21 fixes.

## Follow-up (if FAIL)

N/A — UAT passed.
