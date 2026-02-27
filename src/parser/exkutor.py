import argparse
import base64
import json
import os
import re
import subprocess
import sys

# Assume the C++ engine binary is located at src/core/engine
# In a real installation, this would be relative to the installation path.
DEFAULT_ENGINE_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../src/core/engine")

if os.name == 'nt' and not DEFAULT_ENGINE_PATH.lower().endswith(".exe"):
    DEFAULT_ENGINE_PATH += ".exe"

class ExkutorParser:
    def __init__(self, engine_path=DEFAULT_ENGINE_PATH):
        self.engine_path = engine_path
        if not os.path.exists(self.engine_path):
            raise FileNotFoundError(f"Engine binary not found at {self.engine_path}")

    def run_script(self, script_path):
        if not os.path.exists(script_path):
            print(json.dumps([self._error_json("SyntaxError", "Unknown", f"File not found: {script_path}")]))
            return

        try:
            with open(script_path, 'r') as f:
                lines = f.readlines()
        except Exception as e:
            print(json.dumps([self._error_json("SyntaxError", "Unknown", str(e))]))
            return

        results = []
        for line in lines:
            line = line.strip()
            if not line or line.startswith("#"):
                continue

            # Regex to match RUN_WSL >> "command" or RUN_PS >> "command"
            # We assume commands are double-quoted.
            match = re.match(r'^(RUN_WSL|RUN_PS)\s*>>\s*"(.*)"$', line)

            if not match:
                results.append(self._error_json("SyntaxError", "Unknown", f"Invalid syntax: {line}"))
                continue

            env_key = match.group(1)
            raw_cmd = match.group(2)

            env = "WSL" if env_key == "RUN_WSL" else "PS"

            # Base64 Encode
            encoded_cmd = base64.b64encode(raw_cmd.encode('utf-8')).decode('utf-8')

            # Call Engine
            result = self._call_engine(env, encoded_cmd)
            results.append(result)

        print(json.dumps(results, indent=2))

    def _call_engine(self, env, encoded_cmd):
        cmd = [self.engine_path, "--env", env, "--cmd", encoded_cmd]

        try:
            # Run the process and capture stdout/stderr
            # In Python 3.7+, capture_output=True is available.
            # Using subprocess.run for better control.
            proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

            output = proc.stdout.strip()

            if not output:
                 # If stdout is empty, maybe stderr has something?
                 if proc.stderr:
                     return self._error_json("ExecutionError", env, f"Engine Error: {proc.stderr.strip()}")
                 return self._error_json("ExecutionError", env, "Engine produced no output")

            try:
                return json.loads(output)
            except json.JSONDecodeError:
                return self._error_json("ExecutionError", env, f"Invalid JSON from engine: {output}")

        except Exception as e:
            return self._error_json("ExecutionError", env, str(e))

    def _error_json(self, type_, env, msg):
        return {
            "status": "error",
            "type": type_,
            "environment": env,
            "message": msg,
            "exit_code": 1
        }

def main():
    parser = argparse.ArgumentParser(description="Exkutor Language Parser")
    parser.add_argument("script", help="Path to the .exk script file")
    args = parser.parse_args()

    parser_obj = ExkutorParser()
    parser_obj.run_script(args.script)

if __name__ == "__main__":
    main()
