"""Verify both exit status and Valgrind diagnostics, including parse failures."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def check(command, source, expected_status, label):
    result = subprocess.run(
        ["valgrind", "--quiet", "--leak-check=full",
         "--errors-for-leak-kinds=definite,indirect", "--error-exitcode=99", *command],
        input=source, capture_output=True, timeout=30)
    if result.returncode != expected_status:
        raise RuntimeError(f"{label}: exit {result.returncode}\n{result.stderr.decode()}")
    print(f"OK memory: {label}", flush=True)


def main():
    compiler = str(Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "lara").resolve())
    core = str(Path(sys.argv[2] if len(sys.argv) > 2 else ROOT / "tests/core_tests").resolve())
    for folder, args in [(ROOT / "tests", []), (ROOT / "tests/frontend", ["--ast"])]:
        for category, status in [("valid", 0), ("invalid", 1)]:
            paths = sorted((folder / category).glob("*.lc"))
            if not paths:
                raise RuntimeError(f"No fixtures in {folder / category}")
            for path in paths:
                check([compiler, *args], path.read_bytes(), status, str(path.relative_to(ROOT)))
    check([compiler], b"", 0, "empty program")
    check([compiler], b"/*", 1, "comment at EOF")
    check([compiler], b"int a; /* tail", 1, "comment after declaration")
    check([core], b"", 0, "core structures")


if __name__ == "__main__":
    main()
