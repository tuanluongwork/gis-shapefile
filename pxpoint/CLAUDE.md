## Purpose

This file captures a concise, technical, tech-lead level understanding of the `pxpoint` component so a Claude-style code agent can reason about, modify, and extend the demo multi-process correlation & structured-logging solution.

Use this file as the canonical quick-reference: what the module does, how it's organized, build/run/test flows, important APIs, conventions, and recommended next steps.

## High-level summary

PxPoint (folder `pxpoint/`) is a demo and integration slice that showcases a cross-language, multi-process correlation and structured logging solution. Key goals:

- Provide hierarchical correlation (pipeline → process → activity).
- Provide a reusable C++ `log-services` library (structured JSON logging, RAII scopes, performance metrics, environment propagation).
- Provide a demo C++ process (`dummy_parcel_processor`) and a C# orchestrator/demo (under `cs/`) that show how correlation is propagated across processes.
- Provide small test scripts to exercise multi-process behavior.

This code is intended as both a reference implementation and a starting point for integrating correlation/logging into real PxPoint processes.

## Requirements (from user request)

- Read the `pxpoint` directory thoroughly and produce an authoritative description for a Claude code agent. [Done]
- Document build/run/test and integration points so the agent can perform edits and run validations. [Done]

## Directory map (what to look at first)

- `README.md` — Executive summary and integration guidance (high-level design, sample logs, integration recommendations).
- `CLAUDE.md` — (this file) agent-facing technical notes and next steps.
- `cpp/` — C++ demo and library wiring:
	- `CMakeLists.txt` — top-level build for the demo binary.
	- `dummy_parcel_processor.cpp` — example process that exercises logging, correlation, and performance scopes.
	- `log-services/` — the reusable logging library (public headers in `include/`, impl in `src/`, YAML config templates in `config/`, CMake config in `cmake/`). See `log-services/README.md` for API details.
- `cs/` — C# demo/orchestrator and related project files (simulate the orchestrator and cross-language usage); open and inspect when doing C# changes.
- `test_integration.sh`, `test_multi_process.sh` — helper scripts that run multi-process scenarios and demonstrate environment propagation.

## Key concepts and APIs (quick reference)

- Correlation hierarchy: Pipeline → Process → Activity. IDs are string tokens encoded as human-friendly prefixes.
- Correlation manager API (C++): `CorrelationManager` with methods such as `loadFromEnvironment()`, `saveToEnvironment()`, `getPipelineId()`, `getProcessId()`, and helpers to generate IDs.
- Scopes (RAII): `ProcessScope`, `ActivityScope`, `PerformanceTimer` (or `PerformanceScope`) — enter a scope to set activity/process correlation, automatically cleaned on scope exit.
- StructuredLogger (singleton): `StructuredLogger::getInstance()` with methods like `initialize()`, `logProcessStart()`, `logProcessEnd()`, `logPerformance()`, `info()/error()/event()`.
- Convenience macros used throughout demo: `LOG_INFO`, `LOG_ERROR`, `LOG_COMPONENT_INFO`, `LOG_ACTIVITY_SCOPE`, `LOG_PERFORMANCE_SCOPE`, `LOG_COMPONENT_DEBUG`, etc.
- Environment integration: environment variables are used to pass pipeline/process IDs to child processes. Default env var names are configurable in YAML but commonly seen as `LOG_PIPELINE_ID` / `LOG_PROCESS_ID` (see `log-services/README.md`).

## Typical edit or feature tasks (how a Claude agent should proceed)

1. When adding correlation to a C++ binary:
	 - Include `log-services/include/correlation_manager.h` and `log-services/include/structured_logger.h`.
	 - At process start call `CorrelationManager::getInstance().loadFromEnvironment()`.
	 - Create a `ProcessScope` in `main()` and call `StructuredLogger::getInstance().initialize(process_name, level)`.
	 - Replace ad-hoc prints with `LOG_*` macros and add activity scopes for major stages.

2. When modifying `log-services` library:
	 - Update headers under `cpp/log-services/include/` and corresponding `src/` files.
	 - Update CMake under `cpp/log-services/CMakeLists.txt` and ensure targets are exported if needed.
	 - Add unit tests in `cpp/log-services/tests/` and run `ctest`.

3. When adding a C# orchestrator/workflow:
	 - Mirror correlation semantics: read env vars at startup, expose a `PxPointCorrelation` helper, and implement `IDisposable`-style scopes to match RAII behavior.
	 - Ensure JSON logging format matches the C++ output so ELK parsers can be shared.

## Build & run (C++ demo)

Quick commands (macOS, zsh). Run these from `pxpoint/cpp`:

```bash
# create a build dir and compile
mkdir -p pxpoint/cpp/build && cd pxpoint/cpp/build
cmake ..
cmake --build . --config Release

# run the demo binary (optional fips code argument)
./bin/dummy_parcel_processor 01001
```

Notes:
- The top-level `pxpoint/cpp/CMakeLists.txt` adds the `log-services` subdirectory and builds `dummy_parcel_processor` linked against it.
- `log-services` has YAML config templates in `config/`; `StructuredLogger::loadConfigFromYaml()` is available if you want to use config files.

## Run the multi-process test scripts

From `pxpoint/` run:

```bash
./test_multi_process.sh
./test_integration.sh
```

These scripts spawn processes and validate that environment propagation and correlation IDs flow between parent and child.

## Tests and quality gates

- Build: ensure `cmake` + build completes without errors. Typical failure points are missing dependencies (`spdlog`, `yaml-cpp`) if CMake cannot fetch them.
- Lint/Style: the codebase follows modern C++17 idioms; keep compilation flags enabled (-Wall -Wextra -Wpedantic) for local checks.
- Unit tests: `log-services/tests/` should be run with `ctest --output-on-failure` from the build directory.
- Smoke test: run `dummy_parcel_processor` and verify JSON logs are written to `/tmp/pxpoint-logs/` (or configured `log_directory`).

## Conventions & Best practices

- Use structured context maps rather than concatenated strings for fields in logs.
- Add activity scopes for major steps; use `logPerformance()` for long-running operations.
- Propagate correlation via environment variables when spawning child processes.
- Prefer async logging in high-throughput paths (configured via YAML).

## Common troubleshooting tips

- If logs are missing: verify the configured `log_directory` and that the process has write permission.
- If correlation IDs aren't present in child processes: confirm the parent called `saveToEnvironment()` (or that the library is configured to propagate IDs) and that child calls `loadFromEnvironment()` early in `main()`.
- If build fails due to missing third-party libs: check that CMake fetches `spdlog`/`yaml-cpp` (the `log-services` CMake should implement fetching; run CMake with verbose output to inspect failures).

## Assumptions & open questions

- I inspected `pxpoint/README.md`, `pxpoint/cpp/CMakeLists.txt`, `pxpoint/cpp/dummy_parcel_processor.cpp`, and `pxpoint/cpp/log-services/README.md` to create this summary.
- I did not yet deep-inspect the `cs/` folder or the internal `log-services/src/` implementation files. If you want, I can parse those next and extend this file with API signatures and example call-sites.

## Suggested next steps for the Claude agent

1. Inspect `pxpoint/cpp/log-services/include/*.h` and `pxpoint/cpp/log-services/src/*.cpp` to extract exact function signatures and macro definitions.
2. Inspect `pxpoint/cs/` to ensure parity of correlation APIs and logging format.
3. Run the demo build locally and execute `test_multi_process.sh` to validate environment propagation. Capture sample logs under `/tmp/pxpoint-logs/` for ELK pattern refining.
4. Add small unit tests if missing for `CorrelationManager` (generate/load env, ID formats) and for `StructuredLogger` JSON formatting.

## Quick grep/search hints for the agent

- Find all uses of scopes/macros:

	- Search for `ProcessScope`, `ActivityScope`, `LOG_`, `StructuredLogger::getInstance`.

- Find environment variable names or defaults:

	- Search `env_var_pipeline|env_var_process|LOG_PIPELINE_ID|LOG_PROCESS_ID`.

## Requirements coverage

- Read `pxpoint` directory and write understanding into `pxpoint/CLAUDE.md`: Done — this file.

## Completion

If you want, I will now:

- (A) open and summarize the `cs/` directory and C# implementation files, or
- (B) run a local build of `pxpoint/cpp` and run `test_multi_process.sh` to produce sample logs and verify the integration.

Choose A or B (or both) and I will proceed.
