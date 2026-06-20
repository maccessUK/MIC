/*
 * MIC v0.1.1 public reference runtime.
 *
 * Validates a contract, engine manifest, and policy and writes JSON audit
 * output. It does not execute engines. Explicit placeholder hashes and
 * signatures are accepted only as an unverified example dry run.
 *
 * Build:
 *   cc -O2 -Wall -Wextra -Wpedantic core/mic_core.c \
 *      -o build/mic-core -lsodium
 */

#include <sodium.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define MIC_MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MIC_MKDIR(path) mkdir(path, 0755)
#endif

#define MIC_VERSION "0.1.1"
#define MIC_MAX_FILE_BYTES (4U * 1024U * 1024U)
#define MIC_MAX_VALUE 512U
#define MIC_MAX_ACTIONS 64U

/* Public Ed25519 verification key. This is not private signing material. */
static const unsigned char MIC_ROOT_PUBLIC_KEY[crypto_sign_PUBLICKEYBYTES] = {
    0xE3, 0x2A, 0x5B, 0x81, 0xA9, 0xD0, 0xD1, 0x98,
    0x41, 0x6D, 0x41, 0x1B, 0x5B, 0xA7, 0x65, 0xDA,
    0xEA, 0x72, 0xC5, 0x23, 0xB0, 0x07, 0xC7, 0x35,
    0xB9, 0x3B, 0x2C, 0x44, 0x9C, 0x42, 0x64, 0xC3
};

typedef struct { unsigned char *data; size_t length; } MicFile;
typedef struct { char values[MIC_MAX_ACTIONS][MIC_MAX_VALUE]; size_t count; } MicList;

typedef struct {
    char mic_version[MIC_MAX_VALUE];
    char contract_id[MIC_MAX_VALUE];
    char engine_id[MIC_MAX_VALUE];
    char task[MIC_MAX_VALUE];
    char signature[MIC_MAX_VALUE];
    int allow_network;
    int require_audit;
    int max_runtime_seconds;
    int max_memory_mb;
    MicList actions;
} MicContract;

typedef struct {
    char mic_version[MIC_MAX_VALUE];
    char engine_id[MIC_MAX_VALUE];
    char engine_version[MIC_MAX_VALUE];
    char runtime_compatibility[MIC_MAX_VALUE];
    char binary_path[MIC_MAX_VALUE];
    char binary_sha256[MIC_MAX_VALUE];
    char signature[MIC_MAX_VALUE];
    MicList allowed_tasks;
    MicList allowed_actions;
} MicManifest;

typedef struct {
    char policy_version[MIC_MAX_VALUE];
    char policy_id[MIC_MAX_VALUE];
    int fail_closed;
    int allow_network;
    int require_audit;
    int require_signed_contract;
    int require_signed_manifest;
    int require_engine_hash_match;
    int max_runtime_seconds;
    int max_memory_mb;
    MicList blocked_actions;
} MicPolicy;

static void usage(const char *program) {
    fprintf(stderr, "Usage: %s --contract FILE --manifest FILE --policy FILE --audit-out FILE\n", program);
}

static int read_file(const char *path, MicFile *out) {
    FILE *file = fopen(path, "rb");
    long size;
    size_t count;
    out->data = NULL;
    out->length = 0;
    if (!file) return 0;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
    size = ftell(file);
    if (size < 0 || (unsigned long)size > MIC_MAX_FILE_BYTES) { fclose(file); return 0; }
    rewind(file);
    out->data = calloc((size_t)size + 1U, 1U);
    if (!out->data) { fclose(file); return 0; }
    count = fread(out->data, 1U, (size_t)size, file);
    fclose(file);
    if (count != (size_t)size) { free(out->data); out->data = NULL; return 0; }
    out->length = (size_t)size;
    return 1;
}

static char *trim(char *value) {
    char *end;
    while (*value && isspace((unsigned char)*value)) value++;
    end = value + strlen(value);
    while (end > value && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    if (value[0] == '"' && end > value + 1 && end[-1] == '"') {
        value++;
        end[-1] = '\0';
    }
    return value;
}

static int scalar(const char *raw, const char *key, char *out, size_t out_size) {
    const char *line = raw;
    size_t key_length = strlen(key);
    while (*line) {
        const char *end = strchr(line, '\n');
        size_t length = end ? (size_t)(end - line) : strlen(line);
        const char *start = line;
        while ((size_t)(start - line) < length && isspace((unsigned char)*start)) start++;
        if ((size_t)(start - line) + key_length + 1U <= length &&
            strncmp(start, key, key_length) == 0 && start[key_length] == ':') {
            size_t value_length = length - (size_t)(start - line) - key_length - 1U;
            char buffer[MIC_MAX_VALUE];
            char *clean;
            if (value_length >= sizeof(buffer)) return 0;
            memcpy(buffer, start + key_length + 1U, value_length);
            buffer[value_length] = '\0';
            clean = trim(buffer);
            if (!*clean || strlen(clean) >= out_size) return 0;
            strcpy(out, clean);
            return 1;
        }
        if (!end) break;
        line = end + 1;
    }
    return 0;
}

static int bool_value(const char *raw, const char *key, int *out) {
    char value[MIC_MAX_VALUE];
    if (!scalar(raw, key, value, sizeof(value))) return 0;
    if (strcmp(value, "true") == 0) { *out = 1; return 1; }
    if (strcmp(value, "false") == 0) { *out = 0; return 1; }
    return 0;
}

static int int_value(const char *raw, const char *key, int *out) {
    char value[MIC_MAX_VALUE];
    char *end = NULL;
    long parsed;
    if (!scalar(raw, key, value, sizeof(value))) return 0;
    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno || !end || *end || parsed < 1 || parsed > 65536) return 0;
    *out = (int)parsed;
    return 1;
}

static int list_value(const char *raw, const char *key, MicList *out) {
    const char *section = strstr(raw, key);
    const char *line;
    if (!section || !(section = strchr(section, '\n'))) return 0;
    line = section + 1;
    while (*line) {
        const char *end = strchr(line, '\n');
        size_t length = end ? (size_t)(end - line) : strlen(line);
        size_t indent = 0;
        char buffer[MIC_MAX_VALUE];
        char *clean;
        while (indent < length && line[indent] == ' ') indent++;
        if (indent == 0 || indent >= length || line[indent] != '-') break;
        if (out->count >= MIC_MAX_ACTIONS || length - indent - 1U >= sizeof(buffer)) return 0;
        memcpy(buffer, line + indent + 1U, length - indent - 1U);
        buffer[length - indent - 1U] = '\0';
        clean = trim(buffer);
        if (!*clean || strlen(clean) >= MIC_MAX_VALUE) return 0;
        strcpy(out->values[out->count++], clean);
        if (!end) break;
        line = end + 1;
    }
    return out->count > 0;
}

static int list_contains(const MicList *list, const char *value) {
    size_t index;
    for (index = 0; index < list->count; index++)
        if (strcmp(list->values[index], value) == 0) return 1;
    return 0;
}

static void sha256_hex(const unsigned char *data, size_t length, char out[65]) {
    unsigned char hash[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(hash, data, (unsigned long long)length);
    sodium_bin2hex(out, 65U, hash, sizeof(hash));
}

static int is_hex(const char *value, size_t length) {
    size_t index;
    if (strlen(value) != length) return 0;
    for (index = 0; index < length; index++)
        if (!isxdigit((unsigned char)value[index])) return 0;
    return 1;
}

static int is_placeholder(const char *value) {
    return strncmp(value, "PLACEHOLDER_", 12U) == 0;
}

static int is_safe_relative_path(const char *value) {
    if (!value || !*value || value[0] == '/' || value[0] == '\\') return 0;
    if (strstr(value, "../") || strstr(value, "..\\")) return 0;
#ifdef _WIN32
    if (strlen(value) > 1U && value[1] == ':') return 0;
#endif
    return 1;
}

static int verify_document_signature(const MicFile *file, const char *signature) {
    const char *marker = strstr((const char *)file->data, "\nsignature:");
    unsigned char decoded[crypto_sign_BYTES];
    size_t decoded_length = 0;
    if (!marker || !is_hex(signature, crypto_sign_BYTES * 2U)) return 0;
    if (sodium_hex2bin(decoded, sizeof(decoded), signature, strlen(signature),
            NULL, &decoded_length, NULL) != 0 || decoded_length != sizeof(decoded)) return 0;
    return crypto_sign_verify_detached(decoded, file->data,
        (unsigned long long)((marker + 1) - (const char *)file->data),
        MIC_ROOT_PUBLIC_KEY) == 0;
}

static int parse_contract(const MicFile *file, MicContract *contract) {
    const char *raw = (const char *)file->data;
    return scalar(raw, "mic_version", contract->mic_version, sizeof(contract->mic_version)) &&
        scalar(raw, "contract_id", contract->contract_id, sizeof(contract->contract_id)) &&
        scalar(raw, "engine_id", contract->engine_id, sizeof(contract->engine_id)) &&
        scalar(raw, "task", contract->task, sizeof(contract->task)) &&
        bool_value(raw, "allow_network", &contract->allow_network) &&
        bool_value(raw, "require_audit", &contract->require_audit) &&
        int_value(raw, "max_runtime_seconds", &contract->max_runtime_seconds) &&
        int_value(raw, "max_memory_mb", &contract->max_memory_mb) &&
        list_value(raw, "actions:", &contract->actions) &&
        scalar(raw, "signature", contract->signature, sizeof(contract->signature));
}

static int parse_manifest(const MicFile *file, MicManifest *manifest) {
    const char *raw = (const char *)file->data;
    return scalar(raw, "mic_version", manifest->mic_version, sizeof(manifest->mic_version)) &&
        scalar(raw, "engine_id", manifest->engine_id, sizeof(manifest->engine_id)) &&
        scalar(raw, "engine_version", manifest->engine_version, sizeof(manifest->engine_version)) &&
        scalar(raw, "runtime_compatibility", manifest->runtime_compatibility, sizeof(manifest->runtime_compatibility)) &&
        scalar(raw, "binary_path", manifest->binary_path, sizeof(manifest->binary_path)) &&
        scalar(raw, "binary_sha256", manifest->binary_sha256, sizeof(manifest->binary_sha256)) &&
        list_value(raw, "allowed_tasks:", &manifest->allowed_tasks) &&
        list_value(raw, "allowed_actions:", &manifest->allowed_actions) &&
        scalar(raw, "signature", manifest->signature, sizeof(manifest->signature));
}

static int parse_policy(const MicFile *file, MicPolicy *policy) {
    const char *raw = (const char *)file->data;
    return scalar(raw, "policy_version", policy->policy_version, sizeof(policy->policy_version)) &&
        scalar(raw, "policy_id", policy->policy_id, sizeof(policy->policy_id)) &&
        bool_value(raw, "fail_closed", &policy->fail_closed) &&
        bool_value(raw, "allow_network", &policy->allow_network) &&
        bool_value(raw, "require_audit", &policy->require_audit) &&
        bool_value(raw, "require_signed_contract", &policy->require_signed_contract) &&
        bool_value(raw, "require_signed_manifest", &policy->require_signed_manifest) &&
        bool_value(raw, "require_engine_hash_match", &policy->require_engine_hash_match) &&
        int_value(raw, "max_runtime_seconds", &policy->max_runtime_seconds) &&
        int_value(raw, "max_memory_mb", &policy->max_memory_mb) &&
        list_value(raw, "blocked_actions:", &policy->blocked_actions);
}

static int ensure_parent(const char *path) {
    char buffer[MIC_MAX_VALUE];
    size_t index;
    if (strlen(path) >= sizeof(buffer)) return 0;
    strcpy(buffer, path);
    for (index = 1; buffer[index]; index++) {
        if (buffer[index] == '/' || buffer[index] == '\\') {
            char saved = buffer[index];
            buffer[index] = '\0';
            if (*buffer && MIC_MKDIR(buffer) != 0 && errno != EEXIST) return 0;
            buffer[index] = saved;
        }
    }
    return 1;
}

static int write_audit(const char *path, int accepted, const char *reason,
    const MicContract *contract, const MicManifest *manifest,
    const char *contract_hash, const char *manifest_hash, const char *policy_hash,
    int signatures_verified, int engine_hash_verified) {
    FILE *file;
    if (!ensure_parent(path) || !(file = fopen(path, "wb"))) return 0;
    fprintf(file,
        "{\n"
        "  \"accepted\": %s,\n"
        "  \"reason\": \"%s\",\n"
        "  \"timestamp_unix\": %lld,\n"
        "  \"mic_version\": \"%s\",\n"
        "  \"contract_id\": \"%s\",\n"
        "  \"engine_id\": \"%s\",\n"
        "  \"engine_version\": \"%s\",\n"
        "  \"task\": \"%s\",\n"
        "  \"execution_mode\": \"dry_run\",\n"
        "  \"contract_hash\": \"sha256:%s\",\n"
        "  \"manifest_hash\": \"sha256:%s\",\n"
        "  \"policy_hash\": \"sha256:%s\",\n"
        "  \"signatures_verified\": %s,\n"
        "  \"engine_hash_verified\": %s\n"
        "}\n",
        accepted ? "true" : "false", reason, (long long)time(NULL), MIC_VERSION,
        contract->contract_id, contract->engine_id, manifest->engine_version,
        contract->task, contract_hash, manifest_hash, policy_hash,
        signatures_verified ? "true" : "false",
        engine_hash_verified ? "true" : "false");
    return fclose(file) == 0;
}

static int reject(const char *audit_path, const char *reason,
    const MicContract *contract, const MicManifest *manifest,
    const char *contract_hash, const char *manifest_hash, const char *policy_hash) {
    int written = write_audit(audit_path, 0, reason, contract, manifest,
        contract_hash, manifest_hash, policy_hash, 0, 0);
    printf("{\"accepted\":false,\"reason\":\"%s\",\"audit_written\":%s}\n",
        reason, written ? "true" : "false");
    return 1;
}

int main(int argc, char **argv) {
    const char *contract_path = NULL, *manifest_path = NULL;
    const char *policy_path = NULL, *audit_path = NULL;
    MicFile contract_file = {0}, manifest_file = {0}, policy_file = {0}, engine_file = {0};
    MicContract contract = {0};
    MicManifest manifest = {0};
    MicPolicy policy = {0};
    char contract_hash[65] = {0}, manifest_hash[65] = {0}, policy_hash[65] = {0};
    char engine_hash[65] = {0};
    int placeholders, signatures_verified = 0, engine_hash_verified = 0;
    size_t index;

    if (sodium_init() < 0) { fputs("libsodium initialisation failed\n", stderr); return 2; }
    for (index = 1; index < (size_t)argc; index++) {
        if (strcmp(argv[index], "--contract") == 0 && index + 1U < (size_t)argc) contract_path = argv[++index];
        else if (strcmp(argv[index], "--manifest") == 0 && index + 1U < (size_t)argc) manifest_path = argv[++index];
        else if (strcmp(argv[index], "--policy") == 0 && index + 1U < (size_t)argc) policy_path = argv[++index];
        else if (strcmp(argv[index], "--audit-out") == 0 && index + 1U < (size_t)argc) audit_path = argv[++index];
        else { usage(argv[0]); return 2; }
    }
    if (!contract_path || !manifest_path || !policy_path || !audit_path) { usage(argv[0]); return 2; }
    if (!read_file(contract_path, &contract_file) || !read_file(manifest_path, &manifest_file) ||
        !read_file(policy_path, &policy_file)) { fputs("MIC input file could not be read\n", stderr); return 2; }
    sha256_hex(contract_file.data, contract_file.length, contract_hash);
    sha256_hex(manifest_file.data, manifest_file.length, manifest_hash);
    sha256_hex(policy_file.data, policy_file.length, policy_hash);
    if (!parse_contract(&contract_file, &contract) || !parse_manifest(&manifest_file, &manifest) ||
        !parse_policy(&policy_file, &policy))
        return reject(audit_path, "MIC input rejected: missing or invalid required field",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash);
    if (strcmp(contract.mic_version, MIC_VERSION) || strcmp(manifest.mic_version, MIC_VERSION) ||
        strcmp(policy.policy_version, MIC_VERSION))
        return reject(audit_path, "MIC input rejected: unsupported version",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash);
    if (!policy.fail_closed || !policy.require_audit || !contract.require_audit)
        return reject(audit_path, "MIC input rejected: fail-closed audit policy is required",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash);
    if (strcmp(contract.engine_id, manifest.engine_id) ||
        !list_contains(&manifest.allowed_tasks, contract.task))
        return reject(audit_path, "MIC contract rejected: unknown task or engine",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash);
    if (!strstr(manifest.runtime_compatibility, MIC_VERSION))
        return reject(audit_path, "MIC manifest rejected: incompatible runtime version",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash);
    if (!is_safe_relative_path(manifest.binary_path))
        return reject(audit_path, "MIC manifest rejected: unsafe engine path",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash);
    if (contract.allow_network && !policy.allow_network)
        return reject(audit_path, "MIC contract rejected: network access is not allowed by policy",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash);
    if (contract.max_runtime_seconds > policy.max_runtime_seconds ||
        contract.max_memory_mb > policy.max_memory_mb)
        return reject(audit_path, "MIC contract rejected: resource limit exceeds policy",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash);
    for (index = 0; index < contract.actions.count; index++)
        if (!list_contains(&manifest.allowed_actions, contract.actions.values[index]) ||
            list_contains(&policy.blocked_actions, contract.actions.values[index]))
            return reject(audit_path, "MIC contract rejected: unknown or blocked action",
                &contract, &manifest, contract_hash, manifest_hash, policy_hash);

    placeholders = is_placeholder(contract.signature) && is_placeholder(manifest.signature) &&
        is_placeholder(manifest.binary_sha256);
    if (!placeholders && (is_placeholder(contract.signature) ||
        is_placeholder(manifest.signature) || is_placeholder(manifest.binary_sha256)))
        return reject(audit_path, "MIC input rejected: mixed placeholder and verified fields",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash);
    if (!placeholders) {
        if (policy.require_signed_contract && !verify_document_signature(&contract_file, contract.signature))
            return reject(audit_path, "MIC contract rejected: signature verification failed",
                &contract, &manifest, contract_hash, manifest_hash, policy_hash);
        if (policy.require_signed_manifest && !verify_document_signature(&manifest_file, manifest.signature))
            return reject(audit_path, "MIC manifest rejected: signature verification failed",
                &contract, &manifest, contract_hash, manifest_hash, policy_hash);
        signatures_verified = 1;
        if (policy.require_engine_hash_match) {
            if (!read_file(manifest.binary_path, &engine_file))
                return reject(audit_path, "MIC manifest rejected: engine binary could not be read",
                    &contract, &manifest, contract_hash, manifest_hash, policy_hash);
            sha256_hex(engine_file.data, engine_file.length, engine_hash);
            if (strcmp(engine_hash, manifest.binary_sha256))
                return reject(audit_path, "MIC manifest rejected: engine hash mismatch",
                    &contract, &manifest, contract_hash, manifest_hash, policy_hash);
            engine_hash_verified = 1;
        }
    }
    if (!write_audit(audit_path, 1,
            placeholders ? "accepted for unverified example dry run" : "accepted",
            &contract, &manifest, contract_hash, manifest_hash, policy_hash,
            signatures_verified, engine_hash_verified)) {
        fputs("MIC audit output could not be written\n", stderr);
        return 2;
    }
    printf("{\"accepted\":true,\"mic_version\":\"%s\",\"contract_valid\":true,"
        "\"manifest_valid\":true,\"policy_valid\":true,\"execution_mode\":\"dry_run\","
        "\"signatures_verified\":%s,\"engine_hash_verified\":%s,\"audit_written\":true}\n",
        MIC_VERSION, signatures_verified ? "true" : "false",
        engine_hash_verified ? "true" : "false");
    free(contract_file.data); free(manifest_file.data); free(policy_file.data); free(engine_file.data);
    return 0;
}
