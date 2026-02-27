#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <algorithm>
#include "base64.h"

// Define specific headers for Windows vs Linux
#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/wait.h>
  #include <cstdio>
#endif

// -----------------------------------------------------------------------------
// 1. JSON & Output Helpers
// -----------------------------------------------------------------------------

// Helper to escape JSON strings manually
std::string json_escape(const std::string &s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    o << "\\u" << std::hex << (int)c;
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

void print_json(const std::string &status,
                const std::string &type,
                const std::string &env,
                const std::string &msg,
                int exit_code) {
    std::cout << "{"
              << "\"status\": \"" << status << "\","
              << "\"type\": \"" << type << "\","
              << "\"environment\": \"" << env << "\","
              << "\"message\": \"" << json_escape(msg) << "\","
              << "\"exit_code\": " << exit_code
              << "}" << std::endl;
}

// -----------------------------------------------------------------------------
// 2. Execution Logic
// -----------------------------------------------------------------------------

void execute_command(const std::string &env, const std::string &cmd) {
#ifdef _WIN32
    // --- Windows Implementation ---
    std::string full_cmd;
    if (env == "WSL") {
        // Run via wsl.exe
        full_cmd = "wsl.exe " + cmd;
    } else if (env == "PS") {
        // Run via powershell
        full_cmd = "powershell.exe -NoProfile -Command \"" + cmd + "\"";
    } else {
        print_json("error", "ExecutionError", env, "Unknown environment: " + env, 1);
        return;
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE hOutputRead, hOutputWrite;
    if (!CreatePipe(&hOutputRead, &hOutputWrite, &sa, 0)) {
        print_json("error", "ExecutionError", env, "CreatePipe failed", 1);
        return;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hOutputWrite;
    si.hStdError  = hOutputWrite; // Merge stdout/stderr

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    // CreateProcess requires a non-const char* buffer
    std::vector<char> cmd_vec(full_cmd.begin(), full_cmd.end());
    cmd_vec.push_back(0);

    if (!CreateProcessA(NULL, cmd_vec.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        print_json("error", "ExecutionError", env, "CreateProcess failed", 1);
        CloseHandle(hOutputRead);
        CloseHandle(hOutputWrite);
        return;
    }

    // Close the write end of the pipe in the parent so we don't hang reading
    CloseHandle(hOutputWrite);

    // Read output
    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hOutputRead, buffer, sizeof(buffer)-1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = 0;
        output += buffer;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD dwExitCode = 0;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOutputRead);

    // If exit code is non-zero, it's an error in our JSON schema
    std::string status = (dwExitCode == 0) ? "success" : "error";
    std::string type   = (dwExitCode == 0) ? "None" : "ExecutionError";

    print_json(status, type, env, output, (int)dwExitCode);

#else
    // --- Linux Mock Implementation ---
    // In this mock, we just run the command using popen to simulate execution.
    // We treat WSL and PS as just "run this command in local shell".

    std::string full_cmd = cmd + " 2>&1"; // Capture stderr into stdout

    FILE* pipe = popen(full_cmd.c_str(), "r");
    if (!pipe) {
        print_json("error", "ExecutionError", env, "popen failed", 1);
        return;
    }

    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }

    int status_code = pclose(pipe);
    int exit_code = 0;

    if (WIFEXITED(status_code)) {
        exit_code = WEXITSTATUS(status_code);
    } else {
        exit_code = -1; // Abnormal termination
    }

    // Remove trailing newline if present, common in shell output
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    std::string status = (exit_code == 0) ? "success" : "error";
    std::string type   = (exit_code == 0) ? "None" : "ExecutionError";

    print_json(status, type, env, result, exit_code);
#endif
}

// -----------------------------------------------------------------------------
// 3. Main Entry
// -----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    // Expected usage: ./engine --env <WSL|PS> --cmd <BASE64_STRING>
    if (argc < 5) {
        print_json("error", "SyntaxError", "Unknown", "Usage: ./engine --env <ENV> --cmd <BASE64_CMD>", 1);
        return 1;
    }

    std::string env_arg;
    std::string cmd_arg;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--env" && i + 1 < argc) {
            env_arg = argv[++i];
        } else if (arg == "--cmd" && i + 1 < argc) {
            cmd_arg = argv[++i];
        }
    }

    if (env_arg.empty() || cmd_arg.empty()) {
        print_json("error", "SyntaxError", "Unknown", "Missing arguments", 1);
        return 1;
    }

    // Decode Base64
    std::string decoded_cmd = base64_decode(cmd_arg);

    // Execute
    execute_command(env_arg, decoded_cmd);

    return 0;
}
