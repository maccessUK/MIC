# MIC Runtime

This folder is reserved for the MIC Core Runtime implementation.

The runtime should:

- parse contracts
- validate contract schema
- verify signatures
- verify engine manifests
- enforce policies
- launch approved engines
- capture engine output
- write audit records
- fail closed

The runtime should not contain application-specific business logic.
