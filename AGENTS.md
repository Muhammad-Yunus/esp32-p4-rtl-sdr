# AGENT.md

> This file instructs any AI coding agent (OpenCode, Claude Code, Codex, etc.)
> working in this repository to always reference and comply with the project
> documentation in `docs/` before and during all development work.

---

## Mandatory Reading Before Writing Code

**Before making any code changes**, read the following documents in order:

1. **`docs/PRD.md`** — Understand what is in scope for the current phase.
   Check acceptance criteria before claiming a feature is "done".

2. **`docs/ARCHITECTURE.md`** — Understand system data flow, task architecture,
   core split, memory layout, and error recovery. Any new task, buffer, or
   component must fit this architecture. If a change requires an architecture
   deviation, document it in this file first (update the doc, then write code).

3. **`docs/TECH_STACK.md`** — Use the exact component versions, GPIO pin
   numbers, and API calls listed here. Do not substitute a different GPIO,
   sample rate, or component version without updating this file. The GPIO
   mapping table in Section 5 is authoritative for all hardware pins.

4. **`docs/CONVENTIONS.md`** — Follow all naming, CMakeLists, logging,
   memory, FreeRTOS task, sdkconfig, NVS, and git conventions. Do not deviate
   from the directory structure in Section 1. 

---

## Workflow Rules

### Phase Gates

- Do not start Phase N+1 work until all acceptance criteria for Phase N are
  verified (hardware smoke test + serial log + visual check).
- If a blocker is found, stop and report it; do not "work around" it by
  skipping a phase.
- Phase 1 exit = M1 milestone: display boots, RTL-SDR detected, IQ streaming
  confirmed in serial log.

### File Ownership

- `CMakeLists.txt` — top-level boilerplate only. Do not add logic.
- `main/CMakeLists.txt` — update `SRCS` list when adding/removing
  source files. Follow the pattern in CONVENTIONS.md Section 3.
- `sdkconfig.defaults` — base config for ESP32-P4. Do not commit
  `sdkconfig` (generated file).
- `main/idf_component.yml` — pin component versions as listed in
  TECH_STACK.md Section 3.1. Do not add new dependencies without checking
  ARCHITECTURE.md Section 10 (dependency graph).


### Logging

- Use `ESP_LOGE/W/I/D` only — no `printf`, no `ESP_LOGV` in production.
- No per-sample logging. Periodic metrics: minimum 1 Hz via `esp_timer`.

### Git

- Commit messages: `<type>: <description>` (see CONVENTIONS.md Section 10).
- Never commit `firmware/sdkconfig`, `firmware/build/`,
  `firmware/managed_components/`.
- Commit docs/ changes in the same PR as the code they document.

---


## If Documentation and Code Disagree

**The documentation is the source of truth.** If you find code that deviates
from `docs/`:

1. Stop and report the discrepancy.
2. Do not silently "fix" the code to match the doc — the doc may be stale.
3. Do not silently "fix" the doc to match the code — the code may be wrong.
4. Ask the user which is correct, then update the doc first, then fix the code.

---
