import sys
import pathlib
import re

def extract_all_vars(css):
    """Extract all CSS variables from the entire CSS file, not just :root"""
    vars = {}

    # Find all variable definitions (anywhere in the CSS)
    # Pattern matches: --variable-name: value;
    var_pattern = r'--[a-zA-Z0-9-]+:\s*([^;]+);'

    for match in re.finditer(var_pattern, css):
        # Get the full variable declaration line
        start = match.start()
        # Find the start of the line to get the variable name
        line_start = css.rfind('\n', 0, start) + 1
        line_end = css.find('\n', start)
        if line_end == -1:
            line_end = len(css)

        line = css[line_start:line_end]
        if ':' in line:
            parts = line.split(':', 1)
            var_name = parts[0].strip()
            var_value = parts[1].strip().rstrip(';')
            vars[var_name] = var_value

    # Remove all :root blocks and any other blocks containing variable definitions
    # We need to be careful not to remove too much
    css_without_defs = re.sub(r':root\s*\{[^}]*\}', '', css, flags=re.S)

    return vars, css_without_defs

def replace_vars_recursive(css, vars, max_iterations=10):
    """Recursively replace variables until no more changes or max iterations reached"""
    changed = True
    iteration = 0

    while changed and iteration < max_iterations:
        changed = False
        iteration += 1

        # Replace var() function calls
        def replace_var(match):
            var_name = match.group(1)
            if var_name in vars:
                nonlocal changed
                changed = True
                return vars[var_name]
            return match.group(0)

        css = re.sub(r'var\((--[^)]+)\)', replace_var, css)

        # Also replace direct variable references (without var())
        for var_name, var_value in vars.items():
            # Only replace if the variable appears as a value, not in a definition
            # Use word boundaries to avoid partial matches
            pattern = r'(?<!:)--' + re.escape(var_name[2:]) + r'(?!-)'
            if re.search(pattern, css):
                changed = True
                css = re.sub(pattern, var_value, css)

    return css

def process_file(src, dst):
    try:
        css = src.read_text(encoding="utf-8")

        # Extract all variable definitions
        vars, css_without_defs = extract_all_vars(css)

        # Print found variables for debugging
        print(f"Found {len(vars)} variables in {src.name}:")
        for var_name, var_value in list(vars.items())[:5]:  # Show first 5
            print(f"  {var_name}: {var_value}")

        # Replace variables recursively
        processed_css = replace_vars_recursive(css_without_defs, vars)

        # Clean up any leftover var() calls that couldn't be resolved
        processed_css = re.sub(r'var\(--[^)]+\)', '#FF00FF', processed_css)  # Replace unresolved with magenta for debugging

        # Clean up empty lines
        processed_css = re.sub(r'\n\s*\n', '\n', processed_css)

        # Write the processed CSS
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_text(processed_css, encoding="utf-8")

        print(f"Processed: {src} -> {dst}")

    except Exception as e:
        print(f"Error processing {src}: {e}", file=sys.stderr)

def main(src_root, dst_root):
    src_root = pathlib.Path(src_root)
    dst_root = pathlib.Path(dst_root)

    # Process all .rcss files
    for path in src_root.rglob("*.rcss"):
        out = dst_root / path.relative_to(src_root)
        process_file(path, out)

    # Copy non-RCSS files (images, fonts, etc.)
    for path in src_root.rglob("*"):
        if path.suffix != ".rcss" and path.is_file():
            out = dst_root / path.relative_to(src_root)
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(path.read_bytes())
            print(f"Copied: {path} -> {out}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python rcss_preprocessor.py <source_dir> <dest_dir>", file=sys.stderr)
        sys.exit(1)

    main(sys.argv[1], sys.argv[2])