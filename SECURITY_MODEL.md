# MIC Security Model

## Principle

MIC is designed to fail closed.

If trust cannot be verified, execution must not continue.

## Trust Chain

```text
Trusted Root Key
      ↓
Signs Contracts
      ↓
Signs Engine Manifests
      ↓
MIC Core Verifies
      ↓
Engine Executes
      ↓
Evidence Produced
      ↓
Audit Written
```

## Contracts

Contracts describe what is being requested.

A contract should define:

- task
- input
- allowed actions
- constraints
- expected output
- audit requirements
- signature

Unsigned or modified contracts should be rejected when signing is required.

## Engine Manifests

An engine manifest describes a trusted engine.

It should include:

- engine ID
- engine version
- binary path or package reference
- binary hash
- allowed task
- signature

If the engine hash does not match the manifest, execution must be rejected.

## MIC Core Boundary

The MIC Core must remain small and generic.

It should verify, enforce, launch, capture, and audit.

It should not contain application-specific business logic.

## Engine Boundary

Engines perform focused work and produce evidence.

Engines should not directly modify business state.

Examples of engine work:

- verify a policy condition
- classify a document
- check an approval rule
- generate a compliance evidence file
- validate a change request
- calculate a risk result

## Application Worker Boundary

Application workers apply business state.

They consume MIC output and decide whether to update application records.

This separation prevents an engine from becoming an uncontrolled privileged actor.

## Network Policy

Contracts should declare whether network access is allowed.

Default: `allow_network: false`

Network access should only be allowed for engines that explicitly require it and have been reviewed.

## Audit Integrity

Audit records should include hashes of:

- contract
- plan
- engine manifest
- engine binary/package
- result/evidence output

This allows later verification that the audit corresponds to the exact contract and engine used.

## Private Keys

Private signing keys must never be committed to source control.

Recommended practice:

- keep root private keys offline where possible
- use separate signing keys for development and production
- rotate compromised keys
- support revocation lists in future versions

## Experimental Warning

MIC v0.1.1 is experimental.

It should not be treated as independently certified security software.

Use in regulated or safety-critical environments requires independent review.
