# exkutor-lang Architecture Plan

## 1. Overview
`exkutor-lang` is a hybrid system designed to automate shell commands in Windows Subsystem for Linux (WSL) and PowerShell on Windows 11. It leverages Python for parsing and C++ for robust, low-level process execution.

## 2. Components

### 2.1 Python Parser (The Brain)
- **Role**: Parses the custom DSL (`.exk` files).
- **Location**: `src/parser/`
- **Responsibility**:
  - Read input script.
  - Parse lines using Regex (e.g., `RUN_WSL >> "cmd"`).
  - Encode the command string into **Base64** to avoid shell escaping issues.
  - Invoke the C++ Execution Engine via a subprocess call, passing arguments via CLI flags.
  - Aggregate and display the JSON output from the engine.

### 2.2 C++ Execution Engine (The Heart)
- **Role**: Executes the actual commands and manages processes.
- **Location**: `src/core/`
- **Responsibility**:
  - Accept arguments: `--env [WSL|PS]` and `--cmd [Base64_String]`.
  - Decode the Base64 command.
  - execute the command in the target environment.
  - Capture `stdout`, `stderr`, and `exit_code`.
  - Output a standardized JSON object.
- **Performance**: Targeted <10ms overhead.

## 3. Communication Protocol
The Python parser communicates with the C++ engine using CLI flags:
```bash
./exkutor-engine --env <ENV_TYPE> --cmd <BASE64_ENCODED_CMD>
```
- **ENV_TYPE**: `WSL` or `PS`.
- **BASE64_ENCODED_CMD**: The raw command string encoded in Base64.

## 4. Cross-Platform Strategy (Preprocessors)
To facilitate testing on Linux while targeting Windows, the C++ engine will use conditional compilation (`#ifdef _WIN32`).

### 4.1 Windows Build (`#ifdef _WIN32`)
- **API**: `CreateProcess`, pipes for I/O.
- **WSL Execution**: Spawns `wsl.exe -e <command>`.
- **PowerShell Execution**: Spawns `powershell.exe -Command <command>`.

### 4.2 Linux Build (`#else`) - For Testing/Verification
- **API**: `posix_spawn` or `popen`.
- **Simulation**:
  - `WSL` requests will be executed as local shell commands (`/bin/sh -c`).
  - `PS` requests will be executed as local shell commands (simulating the behavior).
- **Goal**: Verify the parser-engine pipeline, JSON formatting, and error handling logic in a CI/CD-friendly way.

## 5. Error Handling & JSON Schema
All outputs (success or failure) will be JSON.

```json
{
  "status": "success|error",
  "type": "SyntaxError|ExecutionError|None",
  "environment": "WSL|PS",
  "message": "Output or Error description",
  "exit_code": 0
}
```

## 6. Directory Structure
```
/
├── AGENT.md
├── ARCHITECTURE.md
├── src/
│   ├── core/
│   │   ├── engine.cpp       # C++ Source
│   │   ├── base64.h         # Helper
│   │   └── base64.cpp       # Helper
│   └── parser/
│       └── exkutor.py       # Python Parser
├── tests/
│   └── test_exkutor.py      # Integration tests
└── DOCS.md
```
