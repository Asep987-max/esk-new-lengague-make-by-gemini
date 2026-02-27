# Exkutor Language Documentation

## Overview
Exkutor is a domain-specific language (DSL) for automating shell commands in both Windows Subsystem for Linux (WSL) and PowerShell on Windows 11. It uses a Python parser for syntax analysis and a C++ execution engine for robust process management.

## Syntax

### Running Commands in WSL
To execute a command in the Linux subsystem:
```exk
RUN_WSL >> "your linux command here"
```

### Running Commands in PowerShell
To execute a command in Windows PowerShell:
```exk
RUN_PS >> "your powershell command here"
```

### Comments
Lines starting with `#` are treated as comments and ignored.
```exk
# This is a comment
```

## Usage

### Prerequisites
- Python 3.x
- C++ Compiler (GCC/Clang on Linux, MSVC on Windows)

### Compilation
The C++ engine must be compiled before running scripts.
```bash
# On Linux (Mock Mode)
g++ -o src/core/engine src/core/engine.cpp src/core/base64.cpp

# On Windows (Production)
cl /Fe:src/core/engine.exe src/core/engine.cpp src/core/base64.cpp
```

### Running a Script
Run the Python parser with the path to your `.exk` script:
```bash
python3 src/parser/exkutor.py path/to/script.exk
```

## JSON Output Schema
The output is always a JSON array of result objects.

```json
[
  {
    "status": "success",
    "type": "None",
    "environment": "WSL",
    "message": "Output of the command",
    "exit_code": 0
  },
  {
    "status": "error",
    "type": "ExecutionError",
    "environment": "PS",
    "message": "Error description",
    "exit_code": 1
  }
]
```

## Architecture
- **Parser**: Python (`src/parser/exkutor.py`)
  - Parses `.exk` files.
  - Base64 encodes commands.
  - Spawns the C++ engine.
- **Engine**: C++ (`src/core/engine.cpp`)
  - Receives `--env` and `--cmd` (Base64) flags.
  - Decodes command.
  - Executes via `wsl.exe` or `powershell.exe` (Windows) or `popen` (Linux Mock).
  - Returns JSON to stdout.
