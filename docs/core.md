# MIC v0.1.1 Core

The public C core is a fail-closed reference validator. It reads a contract,
engine manifest, and policy from paths supplied on the command line.

It checks:

- MIC version `0.1.1`
- required contract, manifest, and policy fields
- contract engine/task relationships
- allowed and blocked actions
- network policy
- runtime and memory ceilings
- contract and manifest Ed25519 signatures
- engine SHA-256
- audit output

The current public implementation validates in `dry_run` mode. It does not
launch an engine or modify application state.

## Example mode

Files containing explicit `PLACEHOLDER_...` signatures or hashes may be used
for an unverified example dry run. Audit output records
`signatures_verified: false` and `engine_hash_verified: false`.

Placeholder mode must never be treated as production verification.

## Build

Install a C compiler, `make`, and libsodium development headers:

```bash
make build
```

Equivalent command:

```bash
cc -O2 -Wall -Wextra -Wpedantic \
  core/mic_core.c -o build/mic-core -lsodium
```

## Run

```bash
build/mic-core \
  --contract examples/contracts/compliance-check.contract.yaml \
  --manifest examples/manifests/compliance-check.engine.yaml \
  --policy examples/policies/default-fail-closed.policy.yaml \
  --audit-out audit/compliance-check.audit.json
```

Production callers should accept a result only when both signature and engine
hash verification are true.
