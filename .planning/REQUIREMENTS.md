# PSD2UMG — v1.0.1 Requirements

**Milestone:** v1.0.1 (patch release)
**Parent:** v1.0 (archived 2026-04-15)
**Scope:** Panel child attachment hotfix.

Fresh requirement table for v1.0.1. Previous v1.0 requirements are archived at `milestones/v1.0-REQUIREMENTS.md`.

## Active Requirements

### Panel Child Attachment (PANEL-*)

- [x] **PANEL-01**: `@vbox` group layers generate `UVerticalBox` containing children in PSD z-order. Children attached via `UPanelWidget::AddChild`, not `AddChildToCanvas`. No PSD positional data applied to child slots.
- [x] **PANEL-02**: `@hbox` group layers generate `UHorizontalBox` with children in PSD z-order. Same attachment contract as PANEL-01.
- [x] **PANEL-03**: `@scrollbox` group layers generate `UScrollBox` with children in PSD z-order. Default orientation (vertical); horizontal variant deferred.
- [x] **PANEL-04**: `@overlay` group layers generate `UOverlay` with children in PSD z-order. Default slot padding / alignment.
- [x] **PANEL-05**: Canvas group behavior (`@canvas` or default on group) is byte-identical to v1.0 — no regression in anchor / offset / z-order for existing fixtures.
- [x] **PANEL-06**: Any child whose attachment fails (unknown parent panel type, cast miss) emits a `UE_LOG(LogPSD2UMG, Warning, ...)` naming the parent and child. No silent drops.
- [ ] **PANEL-07**: Fixture PSD `Panels.psd` exercises nested `@vbox`, `@hbox`, `@scrollbox`, `@overlay` groups with mixed child types (images, text, buttons). Automation spec `FPanelAttachmentSpec` asserts `GetChildrenCount()` and child types per group.

## Traceability

| REQ-ID | Assigned Phase | Status |
|---|---|---|
| PANEL-01 | Phase 10 | Complete |
| PANEL-02 | Phase 10 | Complete |
| PANEL-03 | Phase 10 | Complete |
| PANEL-04 | Phase 10 | Complete |
| PANEL-05 | Phase 10 | Complete |
| PANEL-06 | Phase 10 | Complete |
| PANEL-07 | Phase 10 | Pending |

---

*Last updated: 2026-04-15*
