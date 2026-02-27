import unittest
import json
import os
import sys

# Ensure src/parser is in python path
sys.path.append(os.path.join(os.path.dirname(__file__), '../src/parser'))
from exkutor import ExkutorParser

class TestExkutor(unittest.TestCase):
    def setUp(self):
        self.parser = ExkutorParser()
        self.test_script = "tests/test_script.exk"

    def test_run_script(self):
        # We need to capture stdout since run_script prints to it
        from io import StringIO
        captured_output = StringIO()
        sys.stdout = captured_output

        self.parser.run_script(self.test_script)
        sys.stdout = sys.__stdout__

        output = captured_output.getvalue()
        try:
            results = json.loads(output)
        except json.JSONDecodeError:
            self.fail(f"Parser output is not valid JSON: {output}")

        self.assertEqual(len(results), 2)

        # Verify first command (WSL ls -la)
        self.assertEqual(results[0]["environment"], "WSL")
        self.assertEqual(results[0]["status"], "success")
        self.assertEqual(results[0]["type"], "None")

        # Verify second command (PS echo)
        self.assertEqual(results[1]["environment"], "PS")
        self.assertEqual(results[1]["status"], "success")
        self.assertEqual(results[1]["message"], "Hello from PowerShell")

if __name__ == '__main__':
    unittest.main()
