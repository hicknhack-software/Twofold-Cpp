#!/usr/bin/env python3
"""
Twofold Format Tool

A command-line tool to format .twofold files while preserving generated code content.
Uses clang-format for the host C++ code and preserves output directive content exactly.

Usage:
    twofold_format.py [OPTIONS] [FILES...]

Options:
    --target-column N    Target column for output directives (default: infer from first | or \\)
    --clang-format PATH  Path to clang-format executable (default: search PATH)
    --check              Exit with error if files need formatting (dry-run mode)
    --version            Show version information
"""

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional

VERSION = "1.0.0"

# Pattern to match output lines: | or \ (but not \# which is an escape)
OUTPUT_LINE_PATTERN = re.compile(r'^(\s*)([|\\])(.*)$')

# Pattern to match indent directive: = followed by code
INDENT_DIRECTIVE_PATTERN = re.compile(r'^(\s*)(=)(.*)$')

# Placeholder for preserved lines during clang-format processing
PRESERVED_LINE_PLACEHOLDER = 'TWOFOLD_PRESERVED_LINE'


class TwofoldFormatter:
    """Formatter for .twofold files."""

    def __init__(self, clang_format_path: Optional[str] = None, target_column: Optional[int] = None):
        self.clang_format_path = clang_format_path or self._find_clang_format()
        self.target_column = target_column
        self._inferred_column: Optional[int] = None
        self._preserved_lines: list[tuple[int, str]] = []  # (line_number, original_line)

    @staticmethod
    def _find_clang_format() -> str:
        """Find clang-format in PATH."""
        clang_format = shutil.which('clang-format')
        if clang_format:
            return clang_format
        raise RuntimeError(
            "clang-format not found in PATH. Please specify --clang-format option."
        )

    def _infer_target_column(self, lines: list[str]) -> Optional[int]:
        """Infer target column from the first | or \\ directive."""
        for line in lines:
            match = OUTPUT_LINE_PATTERN.match(line)
            if match:
                return len(match.group(1))
        return None

    def _classify_line(self, line: str) -> tuple[str, str, str, str]:
        """
        Classify a line and return (indent, directive, content, original_line).
        
        Returns:
            tuple: (indent, directive, content, original_line)
                   directive is one of: '|', '\\', '=', '' (empty for host code)
        """
        # Check for output directive (| or \)
        # Note: \#{ is an escape for literal #{, not an output directive
        match = OUTPUT_LINE_PATTERN.match(line)
        if match:
            indent, directive, content = match.groups()
            # Check if this is actually an escape sequence \#{ or \#
            if directive == '\\' and content.startswith('#'):
                # This is an escape sequence, treat as host code
                return '', '', '', line
            return indent, directive, content, line
        
        # Check for indent directive (=)
        match = INDENT_DIRECTIVE_PATTERN.match(line)
        if match:
            indent, directive, content = match.groups()
            return indent, directive, content, line
        
        # Host code line
        return '', '', '', line

    def _convert_to_cpp(self, lines: list[str]) -> list[str]:
        """
        Convert twofold file to formatable C++ code.
        Replace output lines and indent directives with placeholders.
        """
        converted = []
        self._preserved_lines = []

        for line_num, line in enumerate(lines):
            indent, directive, content, original_line = self._classify_line(line)

            if directive in ('|', '\\', '='):
                # Store the original line for later restoration
                line_num = len(converted)
                self._preserved_lines.append((line_num, original_line))
                # Replace with a unique placeholder (add semicolon to make valid C++)
                converted.append(f'TWOFOLD_LINE_{line_num};')
            else:
                # Keep as-is (host code)
                converted.append(line)

        return converted

    def _run_clang_format(self, content: str) -> str:
        """Run clang-format on the given content."""
        try:
            result = subprocess.run(
                [self.clang_format_path],
                input=content,
                capture_output=True,
                text=True,
                check=True,
            )
            return result.stdout
        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"clang-format failed: {e.stderr}") from e
        except FileNotFoundError as e:
            raise RuntimeError(
                f"clang-format not found at '{self.clang_format_path}'"
            ) from e

    def _restore_preserved_lines(self, lines: list[str]) -> list[str]:
        """
        Restore preserved lines (output directives and indent directives) from stored original lines.
        Uses unique markers to find the correct lines after clang-format may have reordered them.
        """
        result = list(lines)

        # Build a map of markers to original lines
        marker_to_original = {}
        for placeholder_idx, original_line in self._preserved_lines:
            marker = f'TWOFOLD_LINE_{placeholder_idx};'
            marker_to_original[marker] = original_line

        # Replace markers with original content
        for i, line in enumerate(result):
            for marker, original_line in marker_to_original.items():
                if marker in line:
                    result[i] = original_line
                    break

        return result

    def _realign_output_lines(self, lines: list[str], target_column: int) -> list[str]:
        """
        Realign output lines and indent directive lines to start at the target column.
        Adjusts indentation so |, \\, or = appears at the target column.
        """
        result = []
        
        for line in lines:
            # Check for output directive (| or \)
            match = OUTPUT_LINE_PATTERN.match(line)
            if match:
                indent, directive, content = match.groups()
                new_indent = ' ' * target_column
                result.append(f"{new_indent}{directive}{content}")
                continue
            
            # Check for indent directive (=)
            match = INDENT_DIRECTIVE_PATTERN.match(line)
            if match:
                indent, directive, content = match.groups()
                new_indent = ' ' * target_column
                result.append(f"{new_indent}{directive}{content}")
                continue
            
            result.append(line)
        
        return result

    def format_content(self, content: str) -> str:
        """
        Format twofold content.
        
        Args:
            content: The twofold file content
            
        Returns:
            The formatted content
        """
        lines = content.splitlines()
        
        # Determine target column
        effective_column = self.target_column
        if effective_column is None:
            effective_column = self._infer_target_column(lines) or 0
        
        self._inferred_column = effective_column
        
        # Step 1: Convert to formatable C++
        converted = self._convert_to_cpp(lines)
        
        # Step 2: Run clang-format
        cpp_content = '\n'.join(converted)
        formatted_cpp = self._run_clang_format(cpp_content)
        
        # Step 3: Restore preserved lines (output and indent directives)
        formatted_lines = formatted_cpp.splitlines()
        restored = self._restore_preserved_lines(formatted_lines)

        # Step 4: Realign output lines to target column
        restored = self._realign_output_lines(restored, effective_column)
        
        return '\n'.join(restored) + '\n'

    def format_file(self, filepath: Path, dry_run: bool = False) -> bool:
        """
        Format a twofold file.
        
        Args:
            filepath: Path to the twofold file
            dry_run: If True, don't write the file, just return whether it would change
            
        Returns:
            True if the file was (or would be) changed
        """
        content = filepath.read_text(encoding='utf-8')
        formatted = self.format_content(content)
        
        changed = content != formatted
        
        if not dry_run and changed:
            filepath.write_text(formatted, encoding='utf-8')
        
        return changed


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Format .twofold files while preserving generated code content.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s file.twofold                    Format a single file
  %(prog)s --check file.twofold            Check if file needs formatting
  %(prog)s --target-column 8 file.twofold  Align output at column 8
  %(prog)s *.twofold                       Format all twofold files
        """,
    )
    parser.add_argument(
        'files',
        nargs='+',
        type=Path,
        help='Twofold files to format',
    )
    parser.add_argument(
        '--target-column',
        type=int,
        default=None,
        metavar='N',
        help='Target column for output directives (default: infer from code)',
    )
    parser.add_argument(
        '--clang-format',
        type=str,
        default=None,
        metavar='PATH',
        help='Path to clang-format executable (default: search PATH)',
    )
    parser.add_argument(
        '--check',
        action='store_true',
        help='Check if files need formatting (exit 1 if changes needed)',
    )
    parser.add_argument(
        '--version',
        action='version',
        version=f'%(prog)s {VERSION}',
    )
    
    args = parser.parse_args()
    
    try:
        formatter = TwofoldFormatter(
            clang_format_path=args.clang_format,
            target_column=args.target_column,
        )
    except RuntimeError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    
    exit_code = 0
    for filepath in args.files:
        if not filepath.exists():
            print(f"Error: File not found: {filepath}", file=sys.stderr)
            exit_code = 1
            continue
        
        try:
            changed = formatter.format_file(filepath, dry_run=args.check)
            
            if args.check:
                if changed:
                    print(f"Would reformat: {filepath}")
                    exit_code = 1
                else:
                    print(f"OK: {filepath}")
            else:
                if changed:
                    print(f"Formatted: {filepath}")
                else:
                    print(f"Unchanged: {filepath}")
                    
        except Exception as e:
            print(f"Error formatting {filepath}: {e}", file=sys.stderr)
            exit_code = 1
    
    return exit_code


if __name__ == '__main__':
    sys.exit(main())