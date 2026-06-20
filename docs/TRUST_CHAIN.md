# MIC v0.1.1 Trust Chain

MIC separates intent, verification, execution evidence, and application state.

```text
Trusted public key
        ↓
Signed contract + signed engine manifest
        ↓
MIC Core validation
        ↓
Policy, task, action, network, and resource checks
        ↓
Verified engine hash
        ↓
Dry-run decision or separately controlled execution
        ↓
Audit record with contract, manifest, and policy hashes
        ↓
Application worker independently decides whether to apply state
```

## Fail-closed rules

The MIC Core rejects:

- unsupported versions
- missing required fields
- unknown engines, tasks, or actions
- blocked actions
- network requests prohibited by policy
- resource limits above policy ceilings
- invalid signatures
- missing or mismatched engine hashes
- unwritable required audit output

## Responsibility boundaries

Contracts describe intent and constraints. Manifests describe an engine and its
allowed capabilities. Policies impose organisation-level restrictions. The
MIC Core validates those relationships.

Engines produce evidence. Application workers remain responsible for business
state changes and must independently validate MIC output.
