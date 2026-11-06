"""Fail unless every executable application line and function was exercised."""
import argparse
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--gcov", default="gcov", help="gcov matching the build's GCC")
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1] / "src"
    expected = {path.resolve() for path in source.glob("*.c")}
    seen, lines, functions = set(), {}, {}
    objects = sorted((args.build.resolve() / "src").glob("*.gcno"))
    if not objects:
        parser.error("no .gcno files; configure a GCC --coverage build first")
    for obj in objects:
        result = subprocess.run([args.gcov, "--json-format", "--stdout", str(obj)],
                                capture_output=True, text=True, check=True)
        report = json.loads(result.stdout)
        for file in report["files"]:
            path = Path(file["file"])
            if not path.is_absolute():
                path = Path(report["current_working_directory"]) / path
            path = path.resolve()
            if path not in expected:
                continue
            seen.add(path)
            for line in file["lines"]:
                key = (path, line["line_number"])
                lines[key] = lines.get(key, 0) + line["count"]
            for function in file["functions"]:
                key = (path, function["name"])
                functions[key] = functions.get(key, 0) + function["execution_count"]
    missed = [f"{path.name}:{line}" for (path, line), count in lines.items() if count == 0]
    missed += [f"missing source: {path.name}" for path in sorted(expected - seen)]
    missed += [f"uncalled function: {path.name}:{name}"
               for (path, name), count in functions.items() if count == 0]
    print(f"Executable lines: {sum(count > 0 for count in lines.values())}/{len(lines)}")
    print(f"Functions: {sum(count > 0 for count in functions.values())}/{len(functions)}")
    if missed:
        print("Coverage gaps:\n" + "\n".join(missed))
        return 1
    print("PASS: 100% application line and function coverage")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
