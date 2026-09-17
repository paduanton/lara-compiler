"""Byte-exact TAC checks and frontend regressions; Python standard library only."""
import difflib
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
COMPILER = (ROOT / "lara").resolve()


def compile_source(source, *args):
    return subprocess.run([str(COMPILER), *args], input=source, capture_output=True, timeout=5)


class CompilerTests(unittest.TestCase):
    def test_empty_program(self):
        result = compile_source(b"")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b"")

    def test_ast_keeps_initializers_and_statistics(self):
        result = compile_source(b"fun void main() { let x := 1 + 2; print x; }", "--ast")
        self.assertEqual(result.returncode, 0, result.stderr)
        text = result.stdout.decode()
        for marker in ['[VAR_DECL] "x"', '[EXPR_BINARY] "+"', 'Nós totais  : 10',
                       'Folhas      : 4', 'Profund. max: 5']:
            self.assertIn(marker, text)
        self.assertNotIn("beginFunc", text)
        dot = subprocess.run([sys.executable, str(ROOT / "tools/ast_to_dot.py")],
                             input=result.stdout, capture_output=True, timeout=5)
        self.assertEqual(dot.returncode, 0, dot.stderr)
        self.assertIn(b"digraph AST", dot.stdout)
        self.assertIn(b"EXPR_BINARY", dot.stdout)

    def test_comment_line_numbers(self):
        source = b"/* first\nsecond\n*/\nfun void main() {\n print @;\n}\n"
        result = compile_source(source)
        self.assertEqual(result.returncode, 1)
        self.assertIn(b"linha 5:", result.stderr)
        self.assertEqual(result.stdout, b"")

    def test_unclosed_comments(self):
        for source in [b"/*", b"fun void main() { /* unfinished", b"int a; /* tail"]:
            for args in [(), ("--ast",)]:
                with self.subTest(source=source, args=args):
                    result = compile_source(source, *args)
                    self.assertEqual(result.returncode, 1, result.stderr)
                    self.assertIn("comentário de bloco não fechado".encode(), result.stderr)
                    self.assertEqual(result.stdout, b"")

    def test_whitespace_and_scientific_literals(self):
        result = compile_source(b"fun\tvoid main()\r\n{ print 2.E-4; print .5e+2; }\r\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, b"    beginFunc main\n    print 2.E-4\n    print .5e+2\n    endFunc main\n")

    def test_unknown_option(self):
        result = compile_source(b"", "--unknown")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, b"")
        self.assertIn(b"Uso:", result.stderr)


def fixture_test(path, valid, frontend):
    def run(self):
        result = compile_source(path.read_bytes(), *(["--ast"] if frontend else []))
        self.assertEqual(result.returncode, 0 if valid else 1, result.stderr)
        if not valid:
            self.assertEqual(result.stdout, b"")
            self.assertTrue(result.stderr)
        elif frontend:
            self.assertIn(b"[PROGRAM]", result.stdout)
        else:
            expected = path.with_suffix(".expected")
            self.assertTrue(expected.is_file(), f"Missing expected output: {expected.name}")
            expected_bytes = expected.read_bytes()
            diff = "".join(difflib.unified_diff(
                expected_bytes.decode().splitlines(True), result.stdout.decode().splitlines(True),
                fromfile=expected.name, tofile="actual"))
            self.assertEqual(result.stdout, expected_bytes, diff)
            self.assertEqual(result.stderr, b"")
    return run


def add_fixtures():
    for folder, frontend in [(ROOT / "tests", False), (ROOT / "tests/frontend", True)]:
        for category in ["valid", "invalid"]:
            paths = sorted((folder / category).glob("*.lc"))
            if not paths:
                raise RuntimeError(f"Missing test inputs: {folder / category}")
            for path in paths:
                name = f"test_{'frontend' if frontend else 'tac'}_{category}_{path.stem}"
                setattr(CompilerTests, name, fixture_test(path, category == "valid", frontend))


if __name__ == "__main__":
    if len(sys.argv) > 1:
        COMPILER = Path(sys.argv.pop(1)).resolve()
    add_fixtures()
    unittest.main(verbosity=2)
