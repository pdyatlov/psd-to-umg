# Phase 21: Parser Correctness Fixes - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-28
**Phase:** 21-parser-correctness-fixes
**Areas discussed:** CJK fixture, FXFMT-01 VlLs depth, LFXC-02 UAT blocking, CJK fixture timing

---

## CJK Fixture

| Option | Description | Selected |
|--------|-------------|----------|
| Real PSD provided by user | Drop hand-authored Photoshop file into Tests/Fixtures/ | ✓ |
| Synthetic spec — no real PSD | Spec injects raw std::string with embedded \0 bytes directly | |
| Both — real PSD + sentinel-injection test | Real file + unit-level null-contaminated string test | |

**User's choice:** Real PSD provided by user
**Notes:** User will drop `RichTextCJK.psd` into `Source/PSD2UMG/Tests/Fixtures/` before execution starts.

---

## CJK Fixture Timing

| Option | Description | Selected |
|--------|-------------|----------|
| Before execution starts | File present before /gsd:execute-phase runs | ✓ |
| During execution | Plan includes explicit checkpoint to wait for file | |

**User's choice:** Before execution starts
**Notes:** Plans can assume fixture exists; no checkpoint needed.

---

## FXFMT-01 VlLs Depth

| Option | Description | Selected |
|--------|-------------|----------|
| Full extraction — populate stroke fields | Iterate VlLs items, extract enab/Sz/color same as Objc branch | ✓ |
| Safe iterate only — no extraction | Walk list without aborting; defer extraction to Phase 22 | |

**User's choice:** Full extraction
**Notes:** Phase 22 (Stroke Rendering) can immediately consume VlLs-origin stroke data without re-opening PsdParser.cpp.

---

## LFXC-02 UAT Blocking

| Option | Description | Selected |
|--------|-------------|----------|
| Hard gate — phase incomplete until UAT done | Phase 21 stays open; Phase 22 blocked | |
| Best-effort — code ships, UAT separate | Code changes ship as Phase 21; UAT logged as standalone task | ✓ |

**User's choice:** Best-effort, non-blocking
**Notes:** RTXT-01, LFXC-01, FXFMT-01 constitute phase completion. LFXC-02 does not gate Phase 22.

---

## Claude's Discretion

- Whether to extract a shared `ConvertLfx2Color` helper or inline dispatch in sofi/dsdw blocks
- Whether CJK spec is a new file or appended to existing rich-text spec
- Plan count and split

## Deferred Ideas

- VlLs stroke emission (Phase 22)
- CMYK full conversion (post-v1.3)
