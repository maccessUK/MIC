import assert from "node:assert/strict";
import { EXAMPLES, parseDocument, validateBundle } from "./validator.js";

function replace(source, from, to) {
  assert(source.includes(from), `Expected fixture to include ${from}`);
  return source.replace(from, to);
}

const valid = await validateBundle(EXAMPLES);
assert.equal(valid.valid, true, valid.errors?.join("\n"));
assert.match(valid.bundle_sha256, /^[a-f0-9]{64}$/);

const unsupported = await validateBundle({
  ...EXAMPLES,
  contract: replace(EXAMPLES.contract, 'mic_version: "0.1.1"', 'mic_version: "0.1.0"'),
});
assert.equal(unsupported.valid, false);
assert(unsupported.errors.includes("Unsupported MIC version"));

const networkDenied = await validateBundle({
  ...EXAMPLES,
  contract: replace(EXAMPLES.contract, "allow_network: false", "allow_network: true"),
});
assert.equal(networkDenied.valid, false);
assert(networkDenied.errors.includes("Network access is not allowed by policy"));

const networkAllowed = await validateBundle({
  ...EXAMPLES,
  contract: replace(EXAMPLES.contract, "allow_network: false", "allow_network: true"),
  policy: replace(EXAMPLES.policy, "allow_network: false", "allow_network: true"),
});
assert.equal(networkAllowed.valid, true, networkAllowed.errors?.join("\n"));

const missingManifestAction = await validateBundle({
  ...EXAMPLES,
  manifest: EXAMPLES.manifest.replace("  - verify_agent_identity\n", ""),
});
assert.equal(missingManifestAction.valid, false);
assert(
  missingManifestAction.errors.some((error) =>
    error.includes("Contract actions missing from manifest"),
  ),
);

const blocked = await validateBundle({
  ...EXAMPLES,
  contract: replace(
    EXAMPLES.contract,
    "  - write_audit_event",
    "  - write_audit_event\n  - execute_arbitrary_code",
  ),
  manifest: replace(
    EXAMPLES.manifest,
    "  - write_audit_event",
    "  - write_audit_event\n  - execute_arbitrary_code",
  ),
});
assert.equal(blocked.valid, false);
assert(blocked.errors.includes("Blocked action: execute_arbitrary_code"));

const changed = await validateBundle({
  ...EXAMPLES,
  runtime: replace(
    EXAMPLES.runtime,
    'runtime_name: "mic-reference-runtime"',
    'runtime_name: "mic-reference-runtime-modified"',
  ),
});
assert.notEqual(changed.bundle_sha256, valid.bundle_sha256);

assert.deepEqual(parseDocument('{"mic_version":"0.1.1"}'), {
  mic_version: "0.1.1",
});

console.log("MIC validator tests passed.");
