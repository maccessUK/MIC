# MIC v0.1 Specification

## Name

MIC = Machine Intent Contracts

## Status

Experimental v0.1.1

## Purpose

MIC is a machine-readable contract format and runtime model for describing intended machine work, safety limits, allowed actions, execution constraints, and audit requirements.

MIC sits between human/system intent and execution.

```text
Intent
  ↓
MIC Contract
  ↓
Validator
  ↓
Runtime
  ↓
Engine
  ↓
Evidence
  ↓
Audit
  ↓
Application Worker
```

## Non-Goals

MIC v0.1.1 does not aim to:

- replace programming languages
- replace workflow engines
- execute arbitrary code
- replace application business logic
- replace legal, compliance, or safety review
- directly modify business databases
- act as a certification standard yet

## Core Contract Fields

A MIC v0.1.1 contract should include:

```yaml
mic_version: "0.1.1"
contract_id: "example-contract-001"
engine_id: "example.engine"
task: "example_task"

input:
  # task-specific input

constraints:
  allow_network: false
  max_runtime_seconds: 60
  max_memory_mb: 512
  require_audit: true

actions:
  - verify_input
  - execute_allowed_check
  - generate_evidence
  - write_audit_event

output:
  format: "json"
  fields:
    - decision
    - evidence_id
    - audit_id

signature: "<signature>"
```

## Required Runtime Behaviour

A MIC runtime must:

- fail closed
- reject unknown MIC versions unless explicitly supported
- reject unknown tasks unless explicitly supported
- reject unknown actions unless explicitly supported
- verify contract signatures where signing is enabled
- verify engine manifests where engine verification is enabled
- enforce runtime limits
- enforce network policy
- produce a run ID
- produce a contract hash
- produce an audit record
- stop execution on failed required checks

## Core Runtime Responsibilities

The MIC Core Runtime is responsible for:

- contract parsing
- contract validation
- signature verification
- engine manifest verification
- engine hash verification
- policy enforcement
- resource limit enforcement
- launching approved engines
- capturing engine results
- writing audit records

The MIC Core Runtime must not contain application-specific business logic.

## Engine Responsibilities

MIC engines are responsible for processing inputs and producing evidence.

Engines may:

- read contract input
- read approved input files
- perform approved checks
- produce output files
- produce evidence
- produce engine-level audit data

Engines must not:

- bypass the MIC Core
- directly modify application business databases
- create users
- change permissions
- grant access
- silently ignore required constraints

## Application Worker Responsibilities

Application workers consume MIC output and decide whether business state should change.

Workers may:

- validate MIC output
- verify audit IDs and hashes
- update application databases
- notify users
- record application-level audit events
- reject MIC output that does not satisfy business rules

## Audit Record

A MIC audit record should include:

```json
{
  "run_id": "run_001",
  "timestamp": "2026-01-01T00:00:00Z",
  "mic_version": "0.1.1",
  "contract_id": "example-contract-001",
  "contract_hash": "sha256:...",
  "engine_id": "example.engine",
  "engine_version": "0.1.0",
  "engine_hash": "sha256:...",
  "task": "example_task",
  "status": "success",
  "steps_total": 4,
  "steps_failed": 0,
  "result_hash": "sha256:..."
}
```

## Safety Rules

MIC v0.1.1 should default to:

- no network access unless explicitly allowed
- limited runtime duration
- limited memory
- explicit allowed actions
- required audit logging
- deterministic evidence where possible
- clear failure states
- no silent partial success

## Versioning

MIC contracts must include `mic_version`.

Breaking changes should increment the major contract version.

Experimental implementations should clearly state supported versions.

