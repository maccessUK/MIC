# MIC v0.1.1 Signing

MIC uses detached Ed25519 signatures for contracts and engine manifests.

The reference core contains this public verification key:

```text
e32a5b81a9d0d198416d411b5ba765daea72c523b007c735b93b2c449c4264c3
```

This value is a public key. It cannot create signatures.

## Signed bytes

The MIC Core verifies all bytes before the top-level `signature:` line,
including the newline immediately before that line. Producers must preserve
byte-for-byte canonical content when signing.

## Signature format

Use a lowercase or uppercase 128-character hexadecimal Ed25519 detached
signature:

```yaml
signature: "0123...128 hexadecimal characters..."
```

Example files intentionally contain:

```yaml
signature: "PLACEHOLDER_SIGNATURE_REPLACE_AFTER_SIGNING"
```

That placeholder permits only an unverified dry run.

## Key handling

- Never commit private signing keys.
- Never place private keys in example contracts.
- Keep signing services separate from MIC Core verification.
- Rotate trust roots through an explicit, reviewed release process.
- Publish checksums and signed release metadata for compiled artifacts.
