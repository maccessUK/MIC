export const MIC_VERSION = "0.1.1";
export const GITHUB_URL = "https://github.com/maccessUK/MIC";

const BLOCKED_ACTIONS = new Set([
  "execute_arbitrary_code",
  "open_unrestricted_network",
  "write_unapproved_path",
  "modify_business_database_directly",
]);

export const EXAMPLES = {
  contract: `mic_version: "0.1.1"
contract_id: "example-ai-agent-approval-001"
engine_id: "example.ai_action.approval"
task: "approve_ai_requested_action"

agent:
  agent_id: "agent-demo-001"
  model_context_hash: "example-context-hash"

requested_action:
  action_type: "modify_record"
  target: "example-record-001"
  risk_level: "high"

constraints:
  allow_network: false
  require_audit: true
  require_human_approval: true
  require_policy_check: true
  require_reason: true
  max_runtime_seconds: 60
  max_memory_mb: 256

actions:
  - verify_agent_identity
  - verify_requested_action
  - verify_policy_rules
  - verify_human_approval
  - generate_action_decision
  - write_audit_event

output:
  format: "json"
  fields:
    - approved
    - denial_reason
    - required_human_approval
    - policy_snapshot_hash
    - audit_id

signature: "PLACEHOLDER_SIGNATURE_REPLACE_AFTER_SIGNING"`,

  manifest: `mic_version: "0.1.1"
engine_id: "example.ai_action.approval"
engine_version: "0.1.1"
runtime_compatibility: ">=0.1.1 <0.2.0"
binary_path: "examples/engines/ai-agent-approval-engine"
binary_sha256: "PLACEHOLDER_SHA256_REPLACE_AFTER_BUILD"
allowed_tasks:
  - approve_ai_requested_action
allowed_actions:
  - verify_agent_identity
  - verify_requested_action
  - verify_policy_rules
  - verify_human_approval
  - generate_action_decision
  - write_audit_event
signature: "PLACEHOLDER_SIGNATURE_REPLACE_AFTER_SIGNING"`,

  policy: `policy_version: "0.1.1"
policy_id: "mic.regulated_action"

defaults:
  fail_closed: true
  allow_network: false
  require_audit: true
  require_signed_contract: true
  require_signed_manifest: true
  require_engine_hash_match: true
  require_human_approval: true
  require_evidence_output: true

limits:
  max_runtime_seconds: 300
  max_memory_mb: 1024

blocked_actions:
  - execute_arbitrary_code
  - open_unrestricted_network
  - write_unapproved_path
  - modify_business_database_directly`,

  runtime: `runtime_name: "mic-reference-runtime"
runtime_version: "0.1.1"
supported_mic_versions:
  - "0.1.1"
supported_tasks:
  - approve_ai_requested_action
public_key_id: "mic-root-ed25519-2026-01"
runtime_sha256: "PLACEHOLDER_SHA256_REPLACE_AFTER_BUILD"`,
};

function parseScalar(raw) {
  const value = raw.trim();
  if (!value) return null;
  if (
    (value.startsWith('"') && value.endsWith('"')) ||
    (value.startsWith("'") && value.endsWith("'"))
  ) {
    return value.slice(1, -1);
  }
  if (value === "true") return true;
  if (value === "false") return false;
  if (value === "null" || value === "~") return null;
  if (/^-?\d+(?:\.\d+)?$/.test(value)) return Number(value);
  if (value.startsWith("[") || value.startsWith("{")) {
    try {
      return JSON.parse(value.replaceAll("'", '"'));
    } catch {
      return value;
    }
  }
  return value;
}

function yamlLines(text) {
  return text
    .replace(/\r\n?/g, "\n")
    .split("\n")
    .map((raw, lineNumber) => {
      const withoutComment = raw.replace(/\s+#.*$/, "");
      const content = withoutComment.trim();
      return {
        indent: withoutComment.length - withoutComment.trimStart().length,
        content,
        lineNumber: lineNumber + 1,
      };
    })
    .filter((line) => line.content);
}

function parseYamlBlock(lines, startIndex, indent) {
  const isArray = lines[startIndex]?.content.startsWith("- ");
  const result = isArray ? [] : {};
  let index = startIndex;

  while (index < lines.length) {
    const line = lines[index];
    if (line.indent < indent) break;
    if (line.indent > indent) {
      throw new Error(`Unexpected indentation on line ${line.lineNumber}`);
    }

    if (isArray) {
      if (!line.content.startsWith("- ")) break;
      const item = line.content.slice(2).trim();
      result.push(parseScalar(item));
      index += 1;
      continue;
    }

    if (line.content.startsWith("- ")) break;
    const separator = line.content.indexOf(":");
    if (separator < 1) {
      throw new Error(`Expected key/value pair on line ${line.lineNumber}`);
    }

    const key = line.content.slice(0, separator).trim();
    const rawValue = line.content.slice(separator + 1).trim();
    if (!key) throw new Error(`Missing key on line ${line.lineNumber}`);

    if (rawValue) {
      result[key] = parseScalar(rawValue);
      index += 1;
      continue;
    }

    const next = lines[index + 1];
    if (!next || next.indent <= indent) {
      result[key] = {};
      index += 1;
      continue;
    }

    const nested = parseYamlBlock(lines, index + 1, next.indent);
    result[key] = nested.value;
    index = nested.nextIndex;
  }

  return { value: result, nextIndex: index };
}

export function parseDocument(text) {
  const source = text.trim();
  if (!source) throw new Error("Input is empty");
  if (source.startsWith("{") || source.startsWith("[")) {
    return JSON.parse(source);
  }

  const lines = yamlLines(source);
  if (!lines.length) throw new Error("Input is empty");
  return parseYamlBlock(lines, 0, lines[0].indent).value;
}

function canonicaliseValue(value) {
  if (Array.isArray(value)) return value.map(canonicaliseValue);
  if (value && typeof value === "object") {
    return Object.keys(value)
      .sort()
      .reduce((result, key) => {
        result[key] = canonicaliseValue(value[key]);
        return result;
      }, {});
  }
  return value;
}

export function canonicalJson(value) {
  return JSON.stringify(canonicaliseValue(value));
}

export async function sha256(value) {
  const bytes = new TextEncoder().encode(value);
  const digest = await crypto.subtle.digest("SHA-256", bytes);
  return Array.from(new Uint8Array(digest), (byte) =>
    byte.toString(16).padStart(2, "0"),
  ).join("");
}

function requireField(value, label, errors, checks) {
  if (value === undefined || value === null || value === "") {
    errors.push(`${label} is required`);
    return false;
  }
  checks.push(`${label} present`);
  return true;
}

function requireVersion(value, label, errors, checks) {
  if (value !== MIC_VERSION) {
    errors.push(`Unsupported ${label} version`);
    return false;
  }
  checks.push(`${label} version supported`);
  return true;
}

export function validateParsedBundle({ contract, manifest, policy, runtime }) {
  const errors = [];
  const checks = [];
  const actions = Array.isArray(contract.actions) ? contract.actions : [];
  const policyDefaults = policy.defaults || {};
  const policyBlocked = Array.isArray(policy.blocked_actions)
    ? policy.blocked_actions
    : [];

  requireVersion(contract.mic_version, "MIC", errors, checks);
  requireField(contract.contract_id, "Contract ID", errors, checks);
  requireField(contract.engine_id, "Engine ID", errors, checks);
  requireField(contract.task, "Task", errors, checks);
  requireField(contract.constraints, "Constraints", errors, checks);
  requireField(contract.output, "Output", errors, checks);

  if (!actions.length) errors.push("Actions must be a non-empty array");
  else checks.push("Contract actions present");

  if (contract.constraints?.require_audit !== true) {
    errors.push("Contract must require audit output");
  } else checks.push("Contract audit required");

  const policyAllowsNetwork = policyDefaults.allow_network === true;
  if (contract.constraints?.allow_network === true && !policyAllowsNetwork) {
    errors.push("Network access is not allowed by policy");
  } else checks.push(
    contract.constraints?.allow_network === true
      ? "Network access explicitly allowed by policy"
      : "Network access disabled",
  );

  for (const action of actions) {
    if (BLOCKED_ACTIONS.has(action) || policyBlocked.includes(action)) {
      errors.push(`Blocked action: ${action}`);
    }
  }

  requireVersion(manifest.mic_version, "Manifest MIC", errors, checks);
  requireField(manifest.engine_id, "Manifest engine ID", errors, checks);
  requireField(manifest.engine_version, "Engine version", errors, checks);
  requireField(
    manifest.runtime_compatibility,
    "Runtime compatibility",
    errors,
    checks,
  );
  requireField(manifest.binary_sha256, "Engine binary SHA-256", errors, checks);
  requireField(manifest.signature, "Manifest signature", errors, checks);

  if (manifest.engine_id && contract.engine_id !== manifest.engine_id) {
    errors.push("Contract engine ID does not match manifest");
  } else if (manifest.engine_id) checks.push("Contract and manifest engine IDs match");

  if (!Array.isArray(manifest.allowed_tasks) || !manifest.allowed_tasks.includes(contract.task)) {
    errors.push("Contract task is not listed in manifest allowed_tasks");
  } else checks.push("Contract task allowed by manifest");

  const missingActions = actions.filter(
    (action) =>
      !Array.isArray(manifest.allowed_actions) ||
      !manifest.allowed_actions.includes(action),
  );
  if (missingActions.length) {
    errors.push(`Contract actions missing from manifest: ${missingActions.join(", ")}`);
  } else if (actions.length) checks.push("Contract actions allowed by manifest");

  requireVersion(policy.policy_version, "Policy", errors, checks);
  requireField(policy.policy_id, "Policy ID", errors, checks);
  const requiredPolicyFlags = [
    ["fail_closed", "Policy fails closed"],
    ["require_audit", "Policy requires audit"],
    ["require_signed_contract", "Policy requires signed contract"],
    ["require_signed_manifest", "Policy requires signed manifest"],
    ["require_engine_hash_match", "Policy requires engine hash match"],
  ];
  for (const [field, label] of requiredPolicyFlags) {
    if (policyDefaults[field] !== true) errors.push(`${label} must be true`);
    else checks.push(label);
  }

  requireField(runtime.runtime_name, "Runtime name", errors, checks);
  requireVersion(runtime.runtime_version, "Runtime", errors, checks);
  requireField(runtime.public_key_id, "Runtime public key ID", errors, checks);

  if (
    !Array.isArray(runtime.supported_mic_versions) ||
    !runtime.supported_mic_versions.includes(MIC_VERSION)
  ) {
    errors.push(`Runtime does not support MIC ${MIC_VERSION}`);
  } else checks.push("Runtime supports MIC version");

  if (
    !Array.isArray(runtime.supported_tasks) ||
    !runtime.supported_tasks.includes(contract.task)
  ) {
    errors.push("Runtime does not support contract task");
  } else checks.push("Runtime supports contract task");

  if (runtime.runtime_sha256) checks.push("Runtime SHA-256 supplied");

  return { valid: errors.length === 0, checks, errors };
}

export async function validateBundle(sourceBundle) {
  const parsed = {};
  const parseErrors = [];

  for (const key of ["contract", "manifest", "policy", "runtime"]) {
    try {
      parsed[key] = parseDocument(sourceBundle[key]);
    } catch (error) {
      parseErrors.push(`${key}: ${error.message}`);
    }
  }

  if (parseErrors.length) {
    return {
      mic_version: MIC_VERSION,
      valid: false,
      validated_at: new Date().toISOString(),
      checks_passed: [],
      errors: parseErrors,
    };
  }

  const canonical = {
    contract: canonicalJson(parsed.contract),
    manifest: canonicalJson(parsed.manifest),
    policy: canonicalJson(parsed.policy),
    runtime: canonicalJson(parsed.runtime),
  };
  const hashes = {
    contract_sha256: await sha256(canonical.contract),
    manifest_sha256: await sha256(canonical.manifest),
    policy_sha256: await sha256(canonical.policy),
    runtime_manifest_sha256: await sha256(canonical.runtime),
  };
  const bundle_sha256 = await sha256(
    [canonical.contract, canonical.manifest, canonical.policy, canonical.runtime].join(
      "\n",
    ),
  );
  const result = validateParsedBundle(parsed);

  return {
    mic_version: MIC_VERSION,
    valid: result.valid,
    validated_at: new Date().toISOString(),
    ...hashes,
    bundle_sha256,
    checks_passed: result.checks,
    errors: result.errors,
  };
}

function initBrowserValidator() {
  const state = { ...EXAMPLES };
  let activeTab = "contract";
  let latestReceipt = null;

  const editor = document.querySelector("#bundle-editor");
  const editorLabel = document.querySelector("#editor-label");
  const hashPreview = document.querySelector("#hash-preview");
  const receiptOutput = document.querySelector("#receipt-output");
  const receiptTitle = document.querySelector("#receipt-title");
  const receiptStatus = document.querySelector("#receipt-status");
  const copyButton = document.querySelector("#copy-receipt");
  const downloadButton = document.querySelector("#download-receipt");

  const labels = {
    contract: "Contract YAML / JSON",
    manifest: "Engine Manifest YAML / JSON",
    policy: "Policy YAML / JSON",
    runtime: "Runtime Manifest YAML / JSON",
  };

  async function updateHashPreview() {
    const currentValue = editor.value;
    if (!currentValue.trim()) {
      hashPreview.textContent = "SHA-256: waiting for input";
      return;
    }
    try {
      const parsed = parseDocument(currentValue);
      hashPreview.textContent = `SHA-256: ${await sha256(canonicalJson(parsed))}`;
    } catch {
      hashPreview.textContent = "SHA-256: fix parse errors to calculate";
    }
  }

  function showTab(tabName) {
    state[activeTab] = editor.value;
    activeTab = tabName;
    editor.value = state[activeTab];
    editorLabel.textContent = labels[activeTab];
    document.querySelectorAll(".tab").forEach((tab) => {
      const selected = tab.dataset.tab === activeTab;
      tab.classList.toggle("is-active", selected);
      tab.setAttribute("aria-selected", String(selected));
    });
    updateHashPreview();
  }

  document.querySelectorAll(".tab").forEach((tab) => {
    tab.addEventListener("click", () => showTab(tab.dataset.tab));
  });

  document.querySelector("#load-example").addEventListener("click", () => {
    state[activeTab] = EXAMPLES[activeTab];
    editor.value = state[activeTab];
    updateHashPreview();
  });

  document.querySelector("#clear-editor").addEventListener("click", () => {
    state[activeTab] = "";
    editor.value = "";
    updateHashPreview();
  });

  editor.addEventListener("input", () => {
    state[activeTab] = editor.value;
    updateHashPreview();
  });

  document.querySelector("#validate-bundle").addEventListener("click", async () => {
    state[activeTab] = editor.value;
    receiptStatus.textContent = "Checking";
    latestReceipt = await validateBundle(state);
    receiptOutput.textContent = JSON.stringify(latestReceipt, null, 2);
    receiptTitle.textContent = latestReceipt.valid
      ? "Bundle is structurally valid"
      : "Bundle needs attention";
    receiptStatus.textContent = latestReceipt.valid ? "Valid" : "Invalid";
    receiptStatus.className = `status-badge ${
      latestReceipt.valid ? "is-valid" : "is-invalid"
    }`;
    copyButton.disabled = false;
    downloadButton.disabled = false;
  });

  copyButton.addEventListener("click", async () => {
    if (!latestReceipt) return;
    await navigator.clipboard.writeText(JSON.stringify(latestReceipt, null, 2));
    copyButton.textContent = "Copied";
    window.setTimeout(() => {
      copyButton.textContent = "Copy Receipt JSON";
    }, 1400);
  });

  downloadButton.addEventListener("click", () => {
    if (!latestReceipt) return;
    const blob = new Blob([JSON.stringify(latestReceipt, null, 2)], {
      type: "application/json",
    });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = `mic-validation-${latestReceipt.bundle_sha256?.slice(0, 12) || "receipt"}.json`;
    link.click();
    URL.revokeObjectURL(url);
  });

  editor.value = state.contract;
  updateHashPreview();
}

if (typeof document !== "undefined") initBrowserValidator();
