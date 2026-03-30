---
spec_type: module_architecture_spec
module_name: "{{module_name}}"
module_id: "{{module_id}}"
version: "{{version}}"
revision: "{{revision}}"
status: "{{status}}"
owner: "{{owner}}"
author: "{{author}}"
author_email: "{{author_email}}"
reviewers:
  - "{{reviewer_1}}"
  - "{{reviewer_2}}"
last_updated: "{{last_updated}}"
target_architecture: "{{target_architecture}}"
target_level: "{{target_level}}"
verifyarch_manifest_version: "{{verifyarch_manifest_version}}"
functional_model_path: "{{functional_model_path}}"
performance_model_path: "{{performance_model_path}}"
energy_model_path: "{{energy_model_path}}"
primary_parameter_source: "{{primary_parameter_source}}"
primary_register_source: "{{primary_register_source}}"
primary_interface_source: "{{primary_interface_source}}"
---

# {{module_name}} Architecture Specification

<!--directive: AI agents should read the YAML header and Section 0 first. Keep metadata complete and machine-readable. -->

## 0. AI-Readable Summary

<!--directive: Fill this section with short, high-signal facts only. Avoid long prose. -->

| Key | Value |
| --- | --- |
| Module Name | {{module_name}} |
| Module ID | {{module_id}} |
| Architecture | {{target_architecture}} |
| Level | {{target_level}} |
| Version | {{version}} |
| Revision | {{revision}} |
| Status | {{status}} |
| Owner | {{owner}} |
| Author | {{author}} |
| Author Email | {{author_email}} |
| Last Updated | {{last_updated}} |
| Functional Model Path | {{functional_model_path}} |
| Performance Model Path | {{performance_model_path}} |
| Energy Model Path | {{energy_model_path}} |
| Primary Parameter Source | {{primary_parameter_source}} |
| Primary Register Source | {{primary_register_source}} |
| Primary Interface Source | {{primary_interface_source}} |

## 1. Revision History

<!--directive: Record only meaningful document changes. Keep the latest revision first. -->

| Revision | Date | Description | Author |
| --- | --- | --- | --- |
| {{revision}} | {{last_updated}} | {{change_summary}} | {{author_email}} |
| {{older_revision}} | {{older_date}} | {{older_change_summary}} | {{older_author_email}} |

## 2. Preface and Conventions

<!--directive: Define scope, audience, and document conventions needed to read this module spec correctly. -->

### 2.1. Scope

<!--directive: State exactly what this module spec covers and what is intentionally out of scope. -->

This document specifies the architecture of {{module_name}} at the {{target_level}} level.
It is the reference for architecture intent, interfaces, data structures, temporal behavior, parameters, constraints, and verification expectations.

### 2.2. Audience

<!--directive: List the engineering roles that should rely on this document. -->

- {{audience_role_1}}
- {{audience_role_2}}
- {{audience_role_3}}
- AI agents using the `verifyarch` workflow

### 2.3. Basic Conventions

<!--directive: Keep only conventions that are necessary to interpret this spec consistently. -->

- Hexadecimal values use `0x` prefix.
- Binary values use `0b` prefix.
- Bit ranges use `[msb:lsb]` notation.
- Template placeholders use `{{double_braces}}`.
- Directive text uses `<!--directive: ... -->` comments.

### 2.4. Normative Terms

<!--directive: Define the meaning of must, should, and may for future AI-driven checks. -->

- `Must` means required architecture behavior or structure.
- `Should` means preferred behavior with documented exceptions.
- `May` means optional or implementation-defined behavior.

## 3. Glossary and References

<!--directive: Define local terms and point to the key external references needed to interpret the spec. -->

### 3.1. Glossary and Acronyms

| Term / Acronym | Description |
| --- | --- |
| {{term_1}} | {{description_1}} |
| {{term_2}} | {{description_2}} |
| {{term_3}} | {{description_3}} |

### 3.2. References

<!--directive: Reference the authoritative sources for interfaces, registers, parameters, and related architecture documents. -->

| Reference Type | Path / Identifier | Purpose |
| --- | --- | --- |
| Architecture Overview | {{architecture_overview_ref}} | {{architecture_overview_purpose}} |
| Interface Source | {{primary_interface_source}} | {{interface_source_purpose}} |
| Register Source | {{primary_register_source}} | {{register_source_purpose}} |
| Parameter Source | {{primary_parameter_source}} | {{parameter_source_purpose}} |
| Functional Model | {{functional_model_path}} | {{functional_model_purpose}} |
| Performance Model | {{performance_model_path}} | {{performance_model_purpose}} |
| Energy Model | {{energy_model_path}} | {{energy_model_purpose}} |

## 4. Overview

<!--directive: Give a compact architectural summary before detailed sections. Keep this chapter readable without implementation knowledge. -->

### 4.1. Motivation and Function

<!--directive: Explain why the module exists, what value it provides, where it sits in the system, and how success is measured. -->

- Motivation: {{motivation}}
- System Role: {{system_role}}
- Primary Function: {{primary_function}}
- Key KPI(s): {{key_kpis}}

### 4.2. Architectural Overview

<!--directive: Describe what the module does and how it interacts with the rest of the architecture at a high level. -->

{{architectural_overview}}

### 4.3. Block Diagram

<!--directive: Provide the high-level block diagram. Names should match the terminology used elsewhere in the spec. -->

```mermaid
graph TD
    {{block_diagram_content}}
```

## 5. Functional Description

<!--directive: Document the module feature by feature. Each feature should be traceable to interfaces, parameters, and verification items. -->

### 5.1. Feature List

<!--directive: Assign stable IDs and section references for every feature supported by the module. -->

| Feature Name | Feature ID | New / Update / Existing | Definition Status (%) | Section(s) |
| --- | --- | --- | --- | --- |
| {{feature_name_0}} | {{feature_id_0}} | {{feature_status_0}} | {{feature_definition_status_0}} | {{feature_section_0}} |
| {{feature_name_1}} | {{feature_id_1}} | {{feature_status_1}} | {{feature_definition_status_1}} | {{feature_section_1}} |

### 5.2. {{feature_name_0}}

<!--directive: Repeat this subsection structure for each feature. Keep numbering stable once published. -->

#### 5.2.1. Overview

<!--directive: Summarize use case, value, KPI, function pipeline, units affected, and risks. -->

{{feature_overview_0}}

#### 5.2.2. Functional Pipeline

<!--directive: Describe the feature pipeline from input to completion in architecture-neutral terms. -->

{{feature_pipeline_0}}

#### 5.2.3. Units Affected

<!--directive: List all units, interfaces, memories, or state elements touched by the feature. -->

- {{feature_unit_0_a}}
- {{feature_unit_0_b}}
- {{feature_unit_0_c}}

#### 5.2.4. New Features, Algorithms, and Risks

<!--directive: Record algorithms, arbitration policies, sequencing rules, corner cases, and key risks in structured form. -->

| Algorithm / Policy | Purpose | Trigger | Rule | Corner Case | Spec ID |
| --- | --- | --- | --- | --- | --- |
| {{algo_name_0}} | {{algo_purpose_0}} | {{algo_trigger_0}} | {{algo_rule_0}} | {{algo_corner_case_0}} | {{algo_spec_id_0}} |

#### 5.2.5. Software Interface

<!--directive: Describe ISA, API, register, descriptor, metadata, compiler, or toolchain-visible changes. -->

{{software_interface_0}}

#### 5.2.6. Functional Description

<!--directive: Explain how the feature maps to architectural units, including inputs, outputs, flow control, and operation modes. -->

{{feature_functional_description_0}}

#### 5.2.7. Assumptions

<!--directive: List assumptions, limitations, unsupported scenarios, and interactions with legacy or other new features. -->

- {{feature_assumption_0_a}}
- {{feature_assumption_0_b}}

#### 5.2.8. Performance Requirement

<!--directive: State the required performance behavior of the feature and any unit-level or interface-level constraints. -->

{{feature_performance_requirement_0}}

#### 5.2.9. Area Estimation

<!--directive: Record area impact, estimate source, process assumption, and confidence level. -->

{{feature_area_estimation_0}}

#### 5.2.10. Context Switch or Pre-emption Impact

<!--directive: Describe what state must be preserved, flushed, invalidated, or restored across context switch or pre-emption. -->

{{feature_context_switch_impact_0}}

#### 5.2.11. Continuations or FSP Debugger Impact

<!--directive: Describe debugger, firmware, continuation, or recovery implications if they exist. -->

{{feature_debugger_impact_0}}

#### 5.2.12. Security Impact

<!--directive: Describe privilege implications, isolation rules, sensitive state, and information-leak considerations. -->

{{feature_security_impact_0}}

#### 5.2.13. Tool Impact

<!--directive: Describe impacts on compiler, assembler, profiler, trace tools, emulators, validators, or test generators. -->

{{feature_tool_impact_0}}

## 6. Architecture and Data Structures

<!--directive: Describe the module architecture following the block diagram and capture all major sub-units, structures, and temporal protocols. -->

### 6.1. Block Diagram

<!--directive: Present the unit-level architectural block diagram with all major sub-units and architecturally relevant interfaces. -->

```mermaid
graph TD
    {{architecture_block_diagram_content}}
```

### 6.2. Data Structures

<!--directive: Describe architecturally visible structures such as descriptors, tables, queues, metadata layouts, or context records. -->

| Structure | Purpose | Producer | Consumer | Key Fields | Lifetime |
| --- | --- | --- | --- | --- | --- |
| {{data_structure_name_0}} | {{data_structure_purpose_0}} | {{data_structure_producer_0}} | {{data_structure_consumer_0}} | {{data_structure_fields_0}} | {{data_structure_lifetime_0}} |

### 6.3. Temporal Algorithms and Protocols

<!--directive: Describe sequencing rules, arbitration policies, fairness rules, retry rules, ordering rules, and temporal protocols between blocks. -->

| Protocol / Arbitration ID | Scope | Inputs | Policy | Ordering Rule | Tie-Break Rule | Source Section |
| --- | --- | --- | --- | --- | --- | --- |
| {{protocol_id_0}} | {{protocol_scope_0}} | {{protocol_inputs_0}} | {{protocol_policy_0}} | {{protocol_ordering_0}} | {{protocol_tiebreak_0}} | {{protocol_source_section_0}} |

### 6.4. Subunits

<!--directive: Enumerate all sub-units and assign stable IDs. The names here should match the rest of the document and future code mappings. -->

| Subunit ID | Subunit Name | Responsibility | Optional? | Parent |
| --- | --- | --- | --- | --- |
| {{subunit_id_0}} | {{subunit_name_0}} | {{subunit_role_0}} | {{subunit_optional_0}} | {{subunit_parent_0}} |
| {{subunit_id_1}} | {{subunit_name_1}} | {{subunit_role_1}} | {{subunit_optional_1}} | {{subunit_parent_1}} |

## 7. Microarchitecture

<!--directive: Describe the microarchitecture that realizes the architecture. Keep architecture contract and implementation choice clearly separated. -->

### 7.1. Functional Description

<!--directive: Explain sub-block divisions, operating modes, and microarchitectural responsibilities. -->

{{microarchitecture_functional_description}}

### 7.2. Data Structures

<!--directive: Describe microarchitectural storage, FIFOs, buffers, tables, scoreboards, or state arrays needed to understand correctness or performance. -->

{{microarchitecture_data_structures}}

### 7.3. Temporal Algorithms

<!--directive: Describe scheduling, arbitration, throttling, replay, flush, wakeup, retirement, and related temporal behavior. -->

{{microarchitecture_temporal_algorithms}}

### 7.4. Non-deterministic Interfaces

<!--directive: Identify interfaces or behaviors that permit reordering, race windows, or timing-dependent outcomes and state the invariants that must still hold. -->

{{microarchitecture_nondeterministic_interfaces}}

### 7.5. Compatibility

<!--directive: Describe backward compatibility, forward compatibility assumptions, and interactions with legacy behavior. -->

{{microarchitecture_compatibility}}

### 7.6. Unit Verification Strategy

<!--directive: Define how the architecture should be verified and which checks can be automated. -->

| Verification Item ID | Category | Intent | Evidence Source | Pass Condition |
| --- | --- | --- | --- | --- |
| {{verification_item_id_0}} | {{verification_category_0}} | {{verification_intent_0}} | {{verification_evidence_0}} | {{verification_pass_condition_0}} |

### 7.7. Unit Area Estimates

<!--directive: Estimate area by major block and indicate confidence, source, and process assumptions. -->

{{unit_area_estimates}}

### 7.8. Clocks and Resets

<!--directive: Define clock domains, reset domains, sequencing, dependencies, and assumptions. -->

| Domain | Type | Source | Consumers | Sequencing Rule | Notes |
| --- | --- | --- | --- | --- | --- |
| {{clock_or_reset_domain_0}} | {{clock_or_reset_type_0}} | {{clock_or_reset_source_0}} | {{clock_or_reset_consumers_0}} | {{clock_or_reset_rule_0}} | {{clock_or_reset_notes_0}} |

### 7.9. Unit Power Management

<!--directive: Describe architecture-visible power management behavior and state transitions. -->

#### 7.9.1. Block Level Clock Gating

<!--directive: Describe what is gated, under what conditions, and what state must remain valid. -->

{{block_level_clock_gating}}

#### 7.9.2. Block Level Power Gating

<!--directive: Describe power-gated domains, retention requirements, wakeup rules, and forbidden transitions. -->

{{block_level_power_gating}}

#### 7.9.3. Tolerance for Clock Ratios

<!--directive: Describe supported or assumed inter-domain clock ratios and what breaks outside them. -->

{{clock_ratio_tolerance}}

#### 7.9.4. Activities Based on Performance Modes

<!--directive: Describe which behaviors vary with performance or power modes and which guarantees remain constant. -->

{{performance_mode_activities}}

#### 7.9.5. Interaction with System PMU

<!--directive: Describe interface points, requests, acknowledgments, policy hooks, and ownership boundaries. -->

{{pmu_interaction}}

### 7.10. Unit Context Switching

<!--directive: Describe what state is relevant to save, restore, invalidate, or lazily reconstruct during context switching. -->

{{unit_context_switching}}

### 7.11. Clock Domain Crossing

<!--directive: Describe architecturally meaningful CDC boundaries, assumptions, ordering rules, and lossless or lossy behavior. -->

{{clock_domain_crossing}}

### 7.12. Detailed Microarchitecture of Subunits

<!--directive: Repeat the following subsection structure for every major sub-unit. Keep names stable for future AI parsing. -->

#### 7.12.1. {{subunit_name_0}}

<!--directive: Use the actual sub-unit name here. -->

##### 7.12.1.1. Functional Description

<!--directive: Describe the sub-unit role, inputs, outputs, and local responsibilities. -->

{{subunit_functional_description_0}}

##### 7.12.1.2. Pipeline Description

<!--directive: Describe stages, state transitions, hazards, replay rules, and flow control. -->

{{subunit_pipeline_description_0}}

##### 7.12.1.3. Interfaces

<!--directive: Describe interface semantics, payloads, ordering guarantees, and exceptional behavior. -->

{{subunit_interfaces_0}}

##### 7.12.1.4. Memories and Register Arrays

<!--directive: Describe memories, FIFOs, queues, tables, arrays, and register files used by the sub-unit. -->

{{subunit_memories_and_arrays_0}}

### 7.13. Layout Requirements

<!--directive: Record architecturally relevant physical requirements such as locality assumptions, aspect-ratio constraints, or placement sensitivities. -->

{{layout_requirements}}

## 8. Performance Characteristics

<!--directive: Define target performance behavior and the architectural mechanisms needed to sustain it. -->

### 8.1. Plans of Record

<!--directive: Record official performance commitments, milestone targets, or design points. -->

{{plans_of_record}}

### 8.2. Peak Performance Characteristics

<!--directive: Record theoretical peak throughput, capacity, concurrency, or bandwidth characteristics and the assumptions behind them. -->

{{peak_performance_characteristics}}

### 8.3. Performance Regression for Previous Architectures

<!--directive: Compare against previous architectures only when relevant and explicitly identify expected regressions or non-goals. -->

{{performance_regression_vs_previous_architectures}}

### 8.4. Performance Description

<!--directive: Explain how the module is expected to achieve its performance behavior under representative conditions. -->

{{performance_description}}

### 8.5. Performance Rationale

<!--directive: Explain why the documented performance approach is architecturally sound and what tradeoffs were accepted. -->

{{performance_rationale}}

### 8.6. Latency

<!--directive: Describe end-to-end and internal latency expectations, including dependencies on other units and contention assumptions. -->

#### 8.6.1. Internal Latency

<!--directive: Record latency through the module or critical sub-paths that are architecturally relevant. -->

{{internal_latency}}

#### 8.6.2. Latency Dependencies on Other Units

<!--directive: Record latency dependencies on upstream, downstream, shared-resource, or arbitration behavior. -->

{{latency_dependencies_on_other_units}}

## 9. Scalability

<!--directive: Describe what scales, what does not scale, and which design parameters control scaling. -->

### 9.1. Programmable Scalability

<!--directive: Describe what is software-configurable or programmatically scalable. -->

{{programmable_scalability}}

### 9.2. Layout Scalability

<!--directive: Describe which design aspects are sensitive to physical scaling, floorplan growth, or implementation topology. -->

{{layout_scalability}}

#### 9.2.1. Min/Max Scalability

<!--directive: Record minimum and maximum supported scaling points for major architecture parameters. -->

{{min_max_scalability}}

#### 9.2.2. What Scales Well and What Does Not

<!--directive: State the architectural reasons why some blocks, policies, or interfaces scale poorly. -->

{{what_scales_well_and_what_does_not}}

### 9.3. Scalability by Parameters in the Design

<!--directive: List the design parameters that control scalability and the limits they impose. -->

| Parameter | Meaning | Scaling Effect | Min | Max | Source of Truth |
| --- | --- | --- | --- | --- | --- |
| {{scaling_param_name_0}} | {{scaling_param_meaning_0}} | {{scaling_param_effect_0}} | {{scaling_param_min_0}} | {{scaling_param_max_0}} | {{scaling_param_source_0}} |

#### 9.3.1. Parameterized RAMs/FIFOs

<!--directive: Describe parameterized memories, FIFOs, queues, or arrays whose size or shape changes with design settings. -->

{{parameterized_rams_fifos}}

## 10. Programming Guidelines

<!--directive: Describe software-visible usage rules, recommended programming patterns, and restrictions needed for correct or efficient behavior. -->

- {{programming_guideline_1}}
- {{programming_guideline_2}}
- {{programming_guideline_3}}

## 11. Reference and VerifyArch Mapping

<!--directive: Put all traceability material here so AI agents can match the spec against code, models, and outputs. -->

### 11.1. Interfaces

<!--directive: Summarize all architecturally relevant interfaces in one place with stable IDs and source anchors. -->

| Interface ID | Name | Producer | Consumer | Key Contract | Source Anchor |
| --- | --- | --- | --- | --- | --- |
| {{interface_id_0}} | {{interface_name_0}} | {{interface_producer_0}} | {{interface_consumer_0}} | {{interface_contract_0}} | {{interface_source_anchor_0}} |

### 11.2. Register Specification

<!--directive: Record architecturally visible registers, fields, access behavior, reset values, and side effects. -->

| Register | Address / Index | Access | Reset | Fields | Side Effects |
| --- | --- | --- | --- | --- | --- |
| {{register_name_0}} | {{register_addr_0}} | {{register_access_0}} | {{register_reset_0}} | {{register_fields_0}} | {{register_side_effects_0}} |

### 11.3. Parameter Traceability Matrix

<!--directive: List every key parameter that must match between spec and code, including legal values, source-of-truth location, and code evidence location. -->

| Parameter ID | Parameter Name | Category | Expected Value / Range | Source of Truth | Code Anchor | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| {{parameter_id_0}} | {{parameter_name_0}} | {{parameter_category_0}} | {{parameter_expected_0}} | {{parameter_source_0}} | {{parameter_code_anchor_0}} | {{parameter_notes_0}} |
| {{parameter_id_1}} | {{parameter_name_1}} | {{parameter_category_1}} | {{parameter_expected_1}} | {{parameter_source_1}} | {{parameter_code_anchor_1}} | {{parameter_notes_1}} |

### 11.4. Algorithm and Arbitration Traceability Matrix

<!--directive: Record the architecture-defined algorithms or arbitration policies that must match code behavior. -->

| Policy ID | Block | Policy Type | Required Rule | Allowed Abstraction | Code Anchor | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| {{policy_id_0}} | {{policy_block_0}} | {{policy_type_0}} | {{policy_rule_0}} | {{policy_allowed_abstraction_0}} | {{policy_code_anchor_0}} | {{policy_notes_0}} |

### 11.5. Model Mapping Matrix

<!--directive: Map which parts of this module spec are directly represented, abstracted, or intentionally omitted in each model. -->

| Spec Section | Functional Model | Performance Model | Energy Model | Allowed Abstraction Notes |
| --- | --- | --- | --- | --- |
| {{mapping_section_0}} | {{mapping_functional_0}} | {{mapping_performance_0}} | {{mapping_energy_0}} | {{mapping_notes_0}} |

### 11.6. VerifyArch Check Manifest

<!--directive: Define stable, objective checks that an AI agent can execute against the spec and code. -->

| Check ID | Category | Spec Section | Inputs | Expected Condition | Failure Severity | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| {{verifyarch_check_id_0}} | {{verifyarch_category_0}} | {{verifyarch_spec_section_0}} | {{verifyarch_inputs_0}} | {{verifyarch_expected_condition_0}} | {{verifyarch_failure_severity_0}} | {{verifyarch_notes_0}} |

### 11.7. Evidence Quality Rules

<!--directive: State how evidence should be weighted during review. -->

- Prefer direct source definitions over README-style summaries.
- Prefer explicit parameter declarations over inferred values.
- Use generated outputs as supporting evidence, not sole evidence.
- Mark missing evidence as `WARNING` unless this spec requires a direct anchor.

## 12. Short Authoring Rules

<!--directive: Keep these rules visible so completed module specs stay concise, structured, and verifiable. -->

1. Keep all section numbering stable after publication.
2. Use `{{double_brace}}` placeholders only in template files.
3. Use `<!--directive: ... -->` comments for author guidance.
4. Keep the main narrative model-neutral unless a subsection explicitly asks for model mapping.
5. Put every key parameter, arbitration policy, and interface contract into the traceability sections.
6. Prefer tables for structured facts and prose for rationale.
7. Ensure all stable IDs are unique within the document.
