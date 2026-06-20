# MIC — Machine Intent Contracts

**Status:** Experimental v0.1.1

MIC is an experimental trust and evidence layer for automated task execution.

It uses signed contracts, policies, trusted engine manifests, and audit records to prove what a machine, automation tool, background worker, or AI agent was allowed to do — and what evidence was produced afterwards.

MIC is designed for systems where automation should not be trusted blindly.

Instead of letting software or AI agents perform sensitive work directly, MIC defines:

* what is being requested
* which engine is allowed to process it
* which actions are permitted
* which safety limits apply
* which files, paths, or resources are allowed
* what evidence must be produced
* what audit record must exist afterwards

MIC is not an AI model.

MIC is not a workflow engine.

MIC is a signed execution governance layer for controlled, auditable machine execution.


---

## Why MIC Exists

Modern systems increasingly allow software and AI agents to take real actions:

- approve payments
- change infrastructure
- process regulated documents
- release manufacturing batches
- access sensitive records
- trigger operational workflows

The problem is not only whether the action succeeded.

The harder problem is proving:

- who or what requested it
- which rules were checked
- what limits applied
- what was allowed or blocked
- what evidence was produced
- whether the execution matched the original intent

MIC exists to make those actions explicit, signed, constrained, and auditable.

---

## Core Idea

Traditional code says:

```text
Do A, then B, then C.
```

MIC says:

```text
Achieve this outcome, using only these allowed actions, within these limits, and prove what happened.
```

---

## Who MIC Is For

MIC is intended for developers, researchers, auditors, compliance teams, and system designers working on:

* AI agent governance
* signed automation
* policy-bound execution
* secure background jobs
* regulated document processing
* audit-heavy workflows
* evidence-based task approval
* fail-closed runtime systems
* machine-action verification

MIC is probably unnecessary for ordinary CRUD apps, blogs, basic scripts, or low-risk internal tools.

It is aimed at systems where the question is not just:

```text
Did the task run?
```

but:

```text
Was this task authorised, constrained, verified, and auditable?
```

---
## Basic Flow

```text
Human / System Intent
        ↓
Signed MIC Contract
        ↓
MIC Core
        ↓
Contract Verification
        ↓
Policy + Safety Checks
        ↓
Approved Engine Execution
        ↓
Evidence Output
        ↓
Audit Record
        ↓
Application Worker Applies Business State
```

The MIC engine produces evidence.

The application decides whether to apply business changes.

---

## What MIC Is Good For

MIC is designed for systems where evidence and control matter:

- AI agent governance
- compliance systems
- regulated automation
- secure task execution
- high-risk approval workflows
- document processing
- infrastructure change control
- audit-heavy business operations

MIC is probably overkill for ordinary web apps, blogs, small scripts, or simple CRUD systems.

---

## Why Not Just Use a Workflow Engine?

Workflow engines orchestrate work.

MIC governs whether machine work is allowed, constrained, verified, and evidenced.

A workflow engine may answer:

```text
What step runs next?
```

MIC asks:

```text
Was this machine action allowed to run at all, under this policy, with this trusted engine, and can we prove what happened?
```

Workflow engines usually focus on:

* queues
* retries
* branching
* scheduling
* long-running processes

MIC focuses on:

* signed intent
* trusted engine identity
* policy limits
* allowed actions
* core constraints
* evidence output
* audit records
* fail-closed behaviour

MIC can sit beside, underneath, or inside a workflow system. It is not trying to replace orchestration.


---

## Example Contract

```yaml
mic_version: "0.1.1"

contract_id: "ai-refund-approval-001"
engine_id: "example.ai_agent_action_approval"
task: "approve_ai_agent_action"

input:
  actor_type: "ai_agent"
  actor_id: "support-agent-17"
  requested_action: "issue_refund"
  target_id: "order_12345"
  amount_gbp: 149.99

constraints:
  allow_network: false
  max_runtime_seconds: 30
  max_memory_mb: 256
  require_audit: true
  require_policy_snapshot: true
  require_human_approval_above_gbp: 100

actions:
  - verify_actor_identity
  - verify_requested_action
  - verify_policy_limits
  - require_human_approval
  - generate_decision_evidence
  - write_audit_event

output:
  format: "json"
  fields:
    - decision
    - reason
    - required_approvals
    - policy_snapshot_hash
    - audit_id

signature: "<signature>"
```

This contract does not issue the refund directly.

It verifies whether the requested action is allowed and produces evidence. A separate application worker may then apply the refund if the MIC result is valid.

---

## Repository Contents

```text
MIC/
├─ README.md
├─ LICENSE.md
├─ SPECIFICATION.md
├─ SECURITY_MODEL.md
├─ GOVERNANCE.md
├─ CONTRIBUTING.md
├─ examples/
│  ├─ contracts/
│  ├─ manifests/
│  └─ policies/
├─ docs/
│  ├─ core.md
│  ├─ SIGNING.md
│  ├─ TRUST_CHAIN.md
│  ├─ positioning.md
│  └─ terminology.md
├─ core/
│  ├─ mic_core.c
│  └─ README.md
└─ validator/
   └─ README.md
```

---

## What is included in v0.1.1?

- C core source
- example contracts
- example engine manifests
- example policies
- fail-closed policy model
- audit output format
- real Ed25519 verification against the published public key

The included examples use explicit placeholder signatures and engine hashes.
They demonstrate an unverified dry-run workflow and must not be represented as
signed production artifacts.

## Source vs Binary

The public repository publishes source code as the primary artifact.

Compiled binaries may be published later through GitHub Releases.

Do not trust binaries unless they are released by the project maintainer and
their checksums/signatures match the published release metadata.

Never commit private signing keys.

## Quick start

With a C compiler, `make`, and libsodium development headers installed:

```bash
make build

build/mic-core \
  --contract examples/contracts/compliance-check.contract.yaml \
  --manifest examples/manifests/compliance-check.engine.yaml \
  --policy examples/policies/default-fail-closed.policy.yaml \
  --audit-out audit/compliance-check.audit.json
```

See [core documentation](docs/core.md),
[signing](docs/SIGNING.md), and the [trust chain](docs/TRUST_CHAIN.md).

---

## Current Status

MIC v0.1.1 is experimental.

The current focus is:

- defining the contract format
- defining safety rules
- proving signed contract validation
- proving engine verification
- producing useful audit records
- exploring real-world governance examples

Do not use MIC for life-critical, medical, nuclear, aviation, financial, or safety-critical production systems without independent review, testing, certification, and legal/compliance approval.

---

## Licence

MIC is released under the MIC Community License v1.

Free use is allowed for personal, educational, research, evaluation, and non-commercial projects.

Commercial use requires a commercial licence.

See [LICENSE.md](LICENSE.md).

---

## Short Description

MIC is an experimental trust and evidence layer for automated task execution. It uses signed contracts to define intent, allowed actions, safety constraints, and audit requirements before execution occurs.

## Keywords

AI agent governance, signed automation, machine intent contracts, policy-bound execution, runtime evidence, audit trail, fail-closed automation, Ed25519 verification, secure task execution, machine-action verification.
