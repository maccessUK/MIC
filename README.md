# MIC — Machine Intent Contracts

**Status:** Experimental v0.1.1

MIC is an experimental framework for describing machine work as a signed, auditable contract.

Instead of letting automation, AI agents, or background jobs perform actions directly, MIC defines:

- what is being requested
- what actions are allowed
- what safety limits apply
- what evidence must be produced
- what audit record must exist afterwards

MIC is not an AI model.

MIC is not a workflow engine.

MIC is a trust and evidence layer for automated decision-making and secure task execution.

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

## Basic Flow

```text
Human / System Intent
        ↓
Signed MIC Contract
        ↓
MIC Core Runtime
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

## Why Not Just Use A Workflow Engine?

Workflow engines orchestrate work.

MIC governs work.

Workflow engines are usually concerned with:

- steps
- queues
- retries
- branching
- scheduling

MIC is concerned with:

- intent
- authority
- allowed actions
- constraints
- evidence
- verification
- auditability

MIC can sit beside or underneath a workflow engine. It is not trying to replace every orchestration tool.

---

## Example Contract

```yaml
mic_version: "0.1"

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
│  ├─ ai-agent-approval.yaml
│  ├─ compliance-evidence-check.yaml
│  ├─ regulated-change-control.yaml
│  └─ secure-task-execution.yaml
├─ docs/
│  ├─ positioning.md
│  └─ terminology.md
├─ runtime/
│  └─ README.md
└─ validator/
   └─ README.md
```

---

## Current Status

MIC v0.1 is experimental.

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

