# Phase 1: UE5 Port - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-08
**Phase:** 01-ue5-port
**Areas discussed:** Rename depth, Python scripts, Library fate, Verification

---

## Rename Depth

| Option | Description | Selected |
|--------|-------------|----------|
| Files only | Rename uplugin, Build.cs, .cpp/.h files, macros — keep directory names | ✓ |
| Full restructure | Rename Source/AutoPSDUI/ → Source/PSD2UMG/, match plan layout exactly | |

**User's choice:** Files only — directory restructure deferred to when plugin is placed in a real UE project
**Notes:** Class rename is full (no typedef alias) — FAutoPSDUIModule → FPSD2UMGModule, etc.

---

## Python Scripts

| Option | Description | Selected |
|--------|-------------|----------|
| Keep as-is | Leave Content/Python/ untouched in Phase 1 | |
| Delete now | Remove Content/Python/ entirely | |
| Move to archive | Move to docs/python-reference/ | ✓ |

**User's choice:** Move to `docs/python-reference/` — keeps scripts as reference but out of active plugin folder

| Option | Description | Selected |
|--------|-------------|----------|
| Delete Source/ThirdParty/Mac/ now | Python packages replaced by PhotoshopAPI | ✓ |
| Keep for now | Leave until Phase 2 | |

**User's choice:** Delete Source/ThirdParty/Mac/ now — reduces repo noise immediately

---

## Library Fate

| Option | Description | Selected |
|--------|-------------|----------|
| Keep + rename, stub RunPyCmd | Rename to PSD2UMGLibrary, keep WBP helpers, replace RunPyCmd with warning stub | ✓ |
| Keep everything, just rename | Keep RunPyCmd behind #if WITH_PYTHON | |
| Strip to bare minimum | Delete AutoPSDUILibrary entirely | |

**User's choice:** Keep + rename with stub — WBP helpers (CreateWBP, MakeWidgetWithWBP, CompileAndSaveBP, etc.) preserved for Phase 3 reuse

---

## Verification

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, UE 5.7.4 installed | Can compile directly | ✓ |
| Not yet | Will test separately | |

| Option | Description | Selected |
|--------|-------------|----------|
| Standalone plugin in UE project | Plugin in Plugins/ folder | ✓ |
| Engine plugin | Plugin in Engine/Plugins/ | |

**User's choice:** UE 5.7.4 installed locally. Plugin will be tested as standalone in a UE project's Plugins/ folder.
**Notes:** Success criteria = zero compile errors, zero deprecated-API warnings on editor load.
