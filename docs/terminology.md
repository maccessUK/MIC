# MIC Terminology

## Intent

The desired outcome requested by a human, system, or AI agent.

## Contract

A signed machine-readable description of the task, inputs, allowed actions, constraints, and audit requirements.

## MIC Core

The small trusted component that verifies contracts, enforces policy, launches engines, captures results, and writes audit records.

## Engine

A focused executable or module that performs a specific approved task and produces evidence.

## Evidence

The result produced by an engine, used by an application worker to decide what should happen next.

## Audit Record

A tamper-evident record describing what contract ran, which engine ran, what hashes were involved, and what result occurred.

## Application Worker

The application-side process that consumes MIC evidence and applies business state changes if appropriate.

## Fail Closed

If validation, trust checks, policy checks, or required actions fail, execution stops and the action is not allowed.
