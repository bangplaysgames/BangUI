<!--
Sync Impact Report
Version change: 1.0.0 → 1.1.0
Modified principles: Added Operational Paradigm rules to Core Principles
Added sections: Operational Paradigm Coding Rules
Removed sections: None
Templates requiring updates: ⚠️ plan-template.md, ⚠️ spec-template.md, ⚠️ tasks-template.md
Follow-up TODOs: Confirm original ratification date
-->

# BangUI Constitution

## Core Principles

### Operational Paradigm Coding Rules
- Always refer to constitution, spec, Overview, Paradigm, and Operational Paradigm before interacting with code.
- Always take a "Production Ready" approach to code.
- Never attempt to make a stub or minimally viable product.
- Never attempt to shortcut around a problematic block of code by commenting it out.
- Always fix problems.
- Never assume that your changes have fixed the problem.
- Never guess. If you don't know the answer, research. If you need access to more documents to find the answer, ask for it.
- Never run git commands without permission.

### Library-First Foundation
Every feature must be implemented as a standalone, reusable library. Libraries MUST be self-contained, independently testable, and documented. Each library must have a clear purpose and avoid organizational-only code.

### Hybrid UI Hierarchy
UI elements are structured in a hybrid hierarchy: UIElement (base), UIElement2D (2D sprite-based), UIElement3D (3D mesh-based). All elements inherit global properties (docking, sizing, padding, visibility, etc.) and support both 2D and 3D layouts. 3D elements reuse 2D layout logic and add mesh/bone attachment features for diegetic UI.

### Component-Based Extensibility
The library uses object-oriented programming (OOP) augmented with component-based design. Features are implemented as composable modules (components) attached to core objects, promoting flexibility and reducing deep inheritance chains. Composition over inheritance is preferred for extensibility and maintainability.

### Test-First Discipline
Test-driven development (TDD) is mandatory. All features must have tests written and user-approved before implementation. The Red-Green-Refactor cycle is strictly enforced. Integration tests are required for new library contracts, contract changes, and inter-service communication.

### Observability & Simplicity
All libraries must support structured logging and debuggability. Simplicity is prioritized: avoid unnecessary complexity (YAGNI principle). Versioning follows MAJOR.MINOR.PATCH format, with clear documentation of breaking changes.

## Constraints
- Technology stack: C++ with Diligent Engine for rendering and SDL3 for windowing/input.
- UI elements must support both retained mode (persistent state) and immediate mode (debug overlays).
- Styling uses Bang Markup Language (BML), a YAML-inspired syntax with object separation and key-value pairs.
- All UI elements must expose global properties for layout, styling, and interaction.
- No CSS or web-specific dependencies allowed.

## Development Workflow
- All code changes require code review and compliance with constitution principles.
- Features must be specified, planned, and tested independently (see spec-template.md and plan-template.md).
- Tasks are organized by user story for independent implementation and testing (see tasks-template.md).
- Documentation must be updated with every principle or workflow change.

## Governance
- The constitution supersedes all other practices and documents.
- Amendments require documentation, approval, and a migration plan.
- All PRs/reviews must verify compliance with principles and constraints.
- Versioning policy: MAJOR for breaking changes, MINOR for new principles/sections, PATCH for clarifications.
- Compliance reviews are mandatory before release.
- Use README.md and BMLSyntax.md for runtime development guidance.

**Version**: 1.1.0 | **Ratified**: TODO(RATIFICATION_DATE): Confirm original adoption date if known | **Last Amended**: 2025-10-26
