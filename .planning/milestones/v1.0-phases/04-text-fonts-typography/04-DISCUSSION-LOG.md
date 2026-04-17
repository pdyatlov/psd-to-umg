# Phase 4: Text, Fonts & Typography — Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md.

**Date:** 2026-04-09
**Phase:** 04-text-fonts-typography
**Areas discussed:** Font lookup, font fallback, bold/italic resolution, wrap trigger, parser depth

---

## Font Mapping Lookup

| Option | Description | Selected |
|--------|-------------|----------|
| Exact + case-insensitive | Exact PS name match, case-insensitive fallback, then DefaultFont | ✓ |
| Strip style suffix, match base | Strip -Bold/-Italic, look up base, apply typeface | |
| Both combined | Exact first, then stripped-base as fallback | |

**User's choice:** Exact + case-insensitive
**Notes:** Predictable, designer maintains full PS names.

---

## Font Fallback

| Option | Description | Selected |
|--------|-------------|----------|
| Engine default + warning | Use UE Roboto, log warning naming unmapped PS font | ✓ |
| Hard error, abort text layer | Refuse to create text widget | |
| Silent fallback | Engine default with no warning | |

**User's choice:** Engine default + warning
**Notes:** Text still readable; designer sees gap in Output Log.

---

## Bold/Italic Resolution

| Option | Description | Selected |
|--------|-------------|----------|
| PhotoshopAPI flags | bBold/bItalic fields from parser | |
| Parse PS name suffix | String-match -Bold/-Italic after lookup | |
| Both: flags first, suffix fallback | Most robust | ✓ |

**User's choice:** Both — flags primary, suffix fallback
**Notes:** Handles PhotoshopAPI coverage gaps gracefully.

---

## Wrap Trigger

| Option | Description | Selected |
|--------|-------------|----------|
| Explicit text box width | Paragraph text → wrap; point text → no wrap | ✓ |
| Always wrap | Simple but wraps short labels | |
| Opt-in via _wrap suffix | Designer-explicit, zero heuristic | |

**User's choice:** Explicit text box width
**Notes:** Matches PS semantics; parser must distinguish paragraph vs. point text.

---

## Parser Extraction Depth

| Option | Description | Selected |
|--------|-------------|----------|
| PhotoshopAPI only; partial delivery OK | Ship TEXT-02/05/06; TEXT-03/04 best-effort | ✓ |
| Manual TySh descriptor parsing | Parse raw PSD bytes for full coverage | |
| Defer outline/shadow entirely | Move TEXT-03/04 to a new decimal phase | |

**User's choice:** PhotoshopAPI only; partial delivery OK
**Notes:** Unblocks downstream phases. Gap closure via decimal phase if needed.

---

## Claude's Discretion

- Font resolver as free function vs class
- Suffix table location
- Unit test isolation vs end-to-end only
- Struct field ordering in FPsdTextRun
- OutlineColor / ShadowColor default (transparent vs black)

## Deferred Ideas

- Manual TySh descriptor parsing
- URichTextBlock / multi-run text
- Letter spacing, line height
- Font file auto-discovery
- Hard-error on missing mapping
