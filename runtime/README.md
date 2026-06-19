# MIC v0.1.1 Reference Runtime

`mic_runtime.c` is the public C reference validator for MIC v0.1.1.

It validates contracts, manifests, policies, allowed tasks/actions, network
policy, resource limits, Ed25519 signatures, and engine SHA-256 values. It
writes audit JSON and fails closed.

The current public runtime performs validation in `dry_run` mode and does not
launch engines. Explicit `PLACEHOLDER_...` values are accepted only as
unverified examples; the audit record marks signatures and engine hashes as
unverified.

Build with a C compiler and libsodium:

```bash
make build
```

Run:

```bash
build/mic-runtime \
  --contract examples/contracts/compliance-check.contract.yaml \
  --manifest examples/manifests/compliance-check.engine.yaml \
  --policy examples/policies/default-fail-closed.policy.yaml \
  --audit-out audit/compliance-check.audit.json
```
