import sys
import pathlib
import re

def extract_root_vars(css):
    vars = {}

    root_match = re.search(r":root\s*\{([^}]*)\}", css, re.S)
    if not root_match:
        return vars, css

    body = root_match.group(1)
    for line in body.split(";"):
        if "--" in line and ":" in line:
            name, value = line.split(":", 1)
            vars[name.strip()] = value.strip()

    css = css.replace(root_match.group(0), "")
    return vars, css


def replace_vars(css, vars):
    def repl(match):
        name = match.group(1)
        return vars.get(name, match.group(0))  # keep unresolved

    return re.sub(r"var\((--[^)]+)\)", repl, css)


def process_file(src, dst):
    css = src.read_text(encoding="utf-8")
    vars, css = extract_root_vars(css)
    css = replace_vars(css, vars)

    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text(css, encoding="utf-8")


def main(src_root, dst_root):
    src_root = pathlib.Path(src_root)
    dst_root = pathlib.Path(dst_root)

    for path in src_root.rglob("*"):
        out = dst_root / path.relative_to(src_root)

        if path.suffix == ".rcss":
            process_file(path, out)
        else:
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(path.read_bytes())


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
