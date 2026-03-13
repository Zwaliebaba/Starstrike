#!/usr/bin/env python3
"""Migrate traditional #ifndef/#define include guards to #pragma once."""

import re
import sys
from pathlib import Path


def migrate_file(filepath: Path) -> bool:
    """
    Migrate a single header file from traditional include guard to #pragma once.
    Returns True if the file was modified, False otherwise.
    """
    content = filepath.read_text(encoding='utf-8', errors='replace')
    lines = content.splitlines(keepends=True)

    # Find the include guard: scan through lines (skipping comments/blank lines)
    # looking for #ifndef GUARD followed immediately by #define GUARD
    ifndef_line_idx = None
    define_line_idx = None
    guard_name = None

    i = 0
    in_block_comment = False
    while i < len(lines):
        stripped = lines[i].strip()
        # Track block comments /* ... */
        if in_block_comment:
            if '*/' in lines[i]:
                in_block_comment = False
            i += 1
            continue
        if stripped.startswith('/*'):
            if '*/' not in lines[i]:
                in_block_comment = True
            i += 1
            continue
        # Skip blank lines and single-line // comments
        if not stripped or stripped.startswith('//'):
            i += 1
            continue
        # Check for #ifndef
        m = re.match(r'^#ifndef\s+(\w+)\s*$', stripped)
        if m:
            guard_name = m.group(1)
            ifndef_line_idx = i
            # Look for matching #define on the very next non-blank line
            j = i + 1
            while j < len(lines) and not lines[j].strip():
                j += 1
            if j < len(lines):
                m2 = re.match(r'^#define\s+(\w+)\s*$', lines[j].strip())
                if m2 and m2.group(1) == guard_name:
                    define_line_idx = j
                    break
        # If we hit a non-comment, non-blank line that isn't #ifndef, stop
        break

    if ifndef_line_idx is None or define_line_idx is None:
        return False  # No traditional guard found

    # Find the last #endif that closes this guard.
    # It's the very last #endif in the file (possibly with a comment referencing the guard name).
    endif_line_idx = None
    for k in range(len(lines) - 1, -1, -1):
        stripped = lines[k].strip()
        if re.match(r'^#endif', stripped):
            endif_line_idx = k
            break

    if endif_line_idx is None:
        return False

    # Build the new file:
    # - Replace the #ifndef line with '#pragma once'
    # - Remove the #define line
    # - Remove the final #endif line

    new_lines = []
    for idx, line in enumerate(lines):
        if idx == ifndef_line_idx:
            new_lines.append('#pragma once\n')
        elif idx == define_line_idx:
            pass  # skip
        elif idx == endif_line_idx:
            pass  # skip
        else:
            new_lines.append(line)

    # Remove a trailing blank line that may have been left before the removed #endif
    # (only if there's now a blank line at the very end before EOF)
    while new_lines and new_lines[-1].strip() == '':
        new_lines.pop()
    new_lines.append('\n')  # Keep a single trailing newline

    new_content = ''.join(new_lines)
    if new_content == content:
        return False

    filepath.write_text(new_content, encoding='utf-8')
    return True


def main():
    root = Path('/home/user/Starstrike')
    header_files = list(root.rglob('*.h'))

    modified = []
    skipped = []

    for filepath in sorted(header_files):
        try:
            if migrate_file(filepath):
                modified.append(filepath)
                print(f'  MIGRATED: {filepath.relative_to(root)}')
            else:
                skipped.append(filepath)
        except Exception as e:
            print(f'  ERROR: {filepath.relative_to(root)}: {e}', file=sys.stderr)

    print(f'\nDone: {len(modified)} migrated, {len(skipped)} skipped (no guard or already using #pragma once).')


if __name__ == '__main__':
    main()
