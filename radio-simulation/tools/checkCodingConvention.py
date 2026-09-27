#!/usr/bin/env python3
"""Check the machine-enforceable parts of codingConvention.md.

The checker intentionally works on changed files in CI.  This lets a team
introduce the convention without making unrelated legacy code block every MR.
Run without --changed-only to audit the complete repository.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


CPP_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
IGNORED_PARTS = {".git", "build", "Proto/Generated"}
IGNORED_FILES = {"Antenna/Header/matplotlibcpp.h"}
ALLOWED_SOURCE_FILES = {"main"}
PROTECTED_BRANCHES = {"main", "master", "develop"}
BRANCH_PATTERN = re.compile(
    r"^(?:feature|fix|docs|chore|refactor)/[a-z0-9]+(?:-[a-z0-9]+)*$"
)
MR_PATTERN = re.compile(
    r"^(?:feature|fix|docs|chore|refactor): (?:[a-z0-9]|\.[A-Za-z])[A-Za-z0-9 ._/'\"()&:+-]*$"
)
PASCAL_PATTERN = re.compile(r"^[A-Z][A-Za-z0-9]*$")
CAMEL_PATTERN = re.compile(r"^[a-z][A-Za-z0-9]*$")


@dataclass(frozen=True)
class Finding:
    path: str
    line: int
    rule: str
    message: str

    def __str__(self) -> str:
        location = f"{self.path}:{self.line}" if self.line else self.path
        return f"{location}: [{self.rule}] {self.message}"


class GitCommandError(RuntimeError):
    """A git command failed; an empty result is not equivalent to success."""


def run_git(root: Path, *args: str) -> list[str]:
    result = subprocess.run(
        ["git", "-C", str(root), *args],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or "git command failed"
        raise GitCommandError(f"git {' '.join(args)}: {detail}")
    return [line for line in result.stdout.splitlines() if line]


def changed_paths(root: Path, base_ref: str | None) -> list[Path]:
    """Return paths changed relative to a merge target or the previous commit."""
    if base_ref:
        names = run_git(root, "diff", "--name-only", "--diff-filter=ACMR", f"{base_ref}...HEAD")
    else:
        try:
            run_git(root, "rev-parse", "--verify", "HEAD^")
        except GitCommandError:
            return []
        names = run_git(root, "diff", "--name-only", "--diff-filter=ACMR", "HEAD^", "HEAD")

    return [root / name for name in names]


def is_ignored(path: Path, root: Path) -> bool:
    relative = path.relative_to(root).as_posix()
    if relative in IGNORED_FILES:
        return True
    parts = relative.split("/")
    return any(
        relative == ignored or relative.startswith(f"{ignored}/") or ignored in parts
        for ignored in IGNORED_PARTS
    )


def source_paths(root: Path, paths: Iterable[Path]) -> list[Path]:
    result = []
    for path in paths:
        if path.is_dir():
            result.extend(source_paths(root, path.rglob("*")))
            continue
        if not path.is_file() or is_ignored(path, root):
            continue
        if path.suffix in CPP_SUFFIXES or path.name == "Makefile.am":
            result.append(path)
    return sorted(set(result))


def strip_comments(text: str) -> str:
    """Mask comments and literals while preserving every source line."""
    out: list[str] = []
    i = 0
    state = "code"
    raw_end = ""
    while i < len(text):
        char = text[i]
        following = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if char == "/" and following == "/":
                out.extend("  ")
                i += 2
                state = "line-comment"
            elif char == "/" and following == "*":
                out.extend("  ")
                i += 2
                state = "block-comment"
            elif char == "R" and following == '"':
                delimiter_start = i + 2
                delimiter_end = text.find("(", delimiter_start, delimiter_start + 17)
                if delimiter_end >= 0:
                    delimiter = text[delimiter_start:delimiter_end]
                    raw_end = ")" + delimiter + '"'
                    out.extend(" " * (delimiter_end - i + 1))
                    i = delimiter_end + 1
                    state = "raw-string"
                else:
                    out.append(char)
                    i += 1
            elif char in {'"', "'"}:
                state = "string" if char == '"' else "char"
                out.append(" ")
                i += 1
            else:
                out.append(char)
                i += 1
        elif state == "line-comment":
            if char == "\n":
                out.append(char)
                state = "code"
            else:
                out.append(" ")
            i += 1
        elif state == "block-comment":
            if char == "*" and following == "/":
                out.extend("  ")
                i += 2
                state = "code"
            else:
                out.append("\n" if char == "\n" else " ")
                i += 1
        elif state == "raw-string":
            if text.startswith(raw_end, i):
                out.extend(" " * len(raw_end))
                i += len(raw_end)
                state = "code"
            else:
                out.append("\n" if char == "\n" else " ")
                i += 1
        else:
            if char == "\\" and i + 1 < len(text):
                out.extend("  " if text[i + 1] != "\n" else "\\\n")
                i += 2
            elif (state == "string" and char == '"') or (state == "char" and char == "'"):
                out.append(" ")
                i += 1
                state = "code"
            else:
                out.append("\n" if char == "\n" else " ")
                i += 1
    return "".join(out)


def add(findings: list[Finding], path: Path, line: int, rule: str, message: str, root: Path) -> None:
    findings.append(Finding(path.relative_to(root).as_posix(), line, rule, message))


def check_braced_control_statements(
    code: str, path: Path, root: Path, findings: list[Finding]
) -> None:
    """Require a body enclosed in braces for if, while, and for statements."""
    control = re.compile(r"\b(if|while|for)\s*\(")
    for match in control.finditer(code):
        depth = 1
        position = match.end()
        while position < len(code) and depth:
            if code[position] == "(":
                depth += 1
            elif code[position] == ")":
                depth -= 1
            position += 1
        if depth:
            continue

        body_start = position
        while body_start < len(code) and code[body_start].isspace():
            body_start += 1
        if body_start >= len(code) or code[body_start] != "{":
            line = code.count("\n", 0, match.start()) + 1
            keyword = match.group(1)
            add(
                findings,
                path,
                line,
                "control-braces",
                f"{keyword} statements must use braces around their body",
                root,
            )

    for match in re.finditer(r"\belse\b(?!\s+if\b)", code):
        body_start = match.end()
        while body_start < len(code) and code[body_start].isspace():
            body_start += 1
        if body_start >= len(code) or code[body_start] != "{":
            line = code.count("\n", 0, match.start()) + 1
            add(
                findings,
                path,
                line,
                "control-braces",
                "else statements must use braces around their body",
                root,
            )


def check_comment_and_brace_layout(
    raw_lines: list[str], code_lines: list[str], path: Path, root: Path, findings: list[Finding]
) -> None:
    """Check the layout rules that do not require a C++ type parser."""
    for index, (raw_line, code_line) in enumerate(zip(raw_lines, code_lines), start=1):
        for marker in ("//", "/*"):
            marker_at = raw_line.find(marker)
            quote_count = len(re.findall(r'(?<!\\)"', raw_line[:marker_at])) if marker_at >= 0 else 0
            if marker_at >= 0 and quote_count % 2 == 0 and code_line[:marker_at].strip():
                add(findings, path, index, "comment-placement", "comments must be above the code they describe", root)
                break

        close_at = code_line.find("}")
        if close_at < 0:
            continue
        remainder = code_line[close_at + 1:].strip()
        before = code_line[:close_at].strip()
        # `};`, `} else`, `} catch`, and do/while's `} while` are valid K&R forms.
        allowed_suffix = (not before and not remainder) or remainder.startswith((";", ",", "else", "catch", "while"))
        if before and not allowed_suffix:
            add(findings, path, index, "brace-style", "closing braces must be on their own line", root)


def check_path_names(path: Path, root: Path, findings: list[Finding]) -> None:
    relative = path.relative_to(root)
    if path.name == "Makefile.am":
        return

    if path.suffix in CPP_SUFFIXES:
        stem = path.stem
        if stem not in ALLOWED_SOURCE_FILES and not PASCAL_PATTERN.fullmatch(stem):
            add(findings, path, 1, "file-name", "source/Header file names must use PascalCase", root)

        for parent in relative.parents:
            if str(parent) == ".":
                continue
            name = parent.name
            if name in {"build", "docs", "proto"}:
                continue
            if not PASCAL_PATTERN.fullmatch(name):
                add(findings, path, 1, "folder-name", f"folder '{name}' must use PascalCase", root)


def check_cpp(path: Path, root: Path, findings: list[Finding]) -> None:
    raw = path.read_text(encoding="utf-8", errors="replace")
    code = strip_comments(raw)
    lines = code.splitlines()
    raw_lines = raw.splitlines()

    check_braced_control_statements(code, path, root, findings)
    check_comment_and_brace_layout(raw_lines, lines, path, root, findings)

    member_names: set[str] = set()
    class_body_depth: int | None = None
    class_name: str | None = None
    brace_depth = 0

    for index, line in enumerate(lines, start=1):
        raw_line = raw_lines[index - 1] if index <= len(raw_lines) else line
        stripped = line.strip()
        if len(raw_line) > 120:
            add(findings, path, index, "line-length", "line must not exceed 120 characters", root)
        if not stripped:
            brace_depth += line.count("{") - line.count("}")
            continue

        class_match = re.search(r"\b(?:class|struct)\s+([A-Za-z_]\w*)", stripped)
        if class_match:
            class_name = class_match.group(1)
            if not PASCAL_PATTERN.fullmatch(class_name):
                add(findings, path, index, "class-name", f"class/struct '{class_name}' must use PascalCase", root)
            if "{" in stripped:
                class_body_depth = brace_depth + 1

        if class_body_depth is not None and brace_depth == class_body_depth:
            # A declaration may contain an initializer, but a function body is
            # deeper than the class body and therefore cannot be mistaken for a field.
            field_match = re.search(
                r"\b([A-Za-z_]\w*)\s*(?:[=;]|\{[^}]*\}\s*;)$", stripped
            ) if "(" not in stripped and ";" in stripped else None
            if field_match:
                field_name = field_match.group(1)
                if not field_name.startswith("m_") and not re.fullmatch(r"[A-Z][A-Z0-9_]*", field_name):
                    add(findings, path, index, "member-name", f"class member '{field_name}' must use the m_ prefix", root)

        for member_match in re.finditer(r"\bm_[A-Za-z0-9_]+\b", stripped):
            member_names.add(member_match.group(0))

        if re.search(r"\bNULL\b", stripped):
            add(findings, path, index, "nullptr", "use nullptr instead of NULL", root)
        if re.search(r"\benum\s+(?!class\b|struct\b)", stripped):
            add(findings, path, index, "enum-class", "use enum class instead of a plain enum", root)
        if re.search(r"\bnew\s+", stripped) or re.search(r"\bdelete\s+(?!\=)", stripped):
            add(findings, path, index, "raii", "avoid raw owning new/delete; use RAII or smart pointers", root)

        if re.match(r"\s*#\s*define\s+([A-Za-z_]\w*)", stripped):
            macro_name = re.match(r"\s*#\s*define\s+([A-Za-z_]\w*)", stripped).group(1)
            if not re.fullmatch(r"_?[A-Z][A-Z0-9_]*", macro_name):
                add(findings, path, index, "constant-name", f"macro '{macro_name}' must use ALL_CAPS", root)

        const_match = re.search(
            r"\b(?:const|constexpr)\s+(?:[A-Za-z_]\w*(?:::\w+)*(?:\s*[<>,]\s*[A-Za-z_]\w*)*\s+)+([A-Za-z_]\w*)\s*=",
            stripped,
        )
        if const_match and not re.fullmatch(r"[A-Z][A-Z0-9_]*", const_match.group(1)):
            add(
                findings,
                path,
                index,
                "constant-name",
                f"constant '{const_match.group(1)}' must use ALL_CAPS",
                root,
            )

        global_match = re.match(
            r"\s*(?:static\s+)?(?:const\s+)?(?:[A-Za-z_]\w*(?:::\w+)*(?:\s*[<>,]\s*[A-Za-z_]\w*)*\s*[*&]?\s+)([A-Za-z_]\w*)\s*(?:=|;)",
            stripped,
        ) if brace_depth == 0 and "(" not in stripped else None
        if global_match and not re.fullmatch(r"[A-Z][A-Z0-9_]*", global_match.group(1)):
            add(findings, path, index, "constant-name", f"global variable '{global_match.group(1)}' must use ALL_CAPS", root)

        if re.search(r"\*\s*[A-Za-z_]\w*\s*=\s*0\b", stripped):
            add(findings, path, index, "nullptr", "use nullptr instead of literal 0 for pointers", root)

        alias_match = re.match(r"\s*using\s+([A-Za-z_]\w*)\s*=", stripped)
        if alias_match and not CAMEL_PATTERN.fullmatch(alias_match.group(1)):
            add(findings, path, index, "type-alias", "type aliases must use camelCase", root)

        if re.search(r"\bfor\s*\([^)]*;[^)]*\.size\s*\(", stripped):
            add(findings, path, index, "range-for", "prefer a range-based for loop for container iteration", root)
        if re.search(r"\bfor\s*\(\s*(?:std::)?(?:size_t|unsigned|int)\s+([A-Za-z_]\w*)\s*=\s*0\s*;\s*\1\s*<\s*([A-Za-z_]\w*)(?:\.size\s*\(\)|\[[^]]+\])", stripped):
            add(findings, path, index, "range-for", "prefer a range-based for loop for container iteration", root)
        if stripped.count(";") > 1 and not stripped.startswith("for ") and "for (" not in stripped:
            add(findings, path, index, "one-statement", "put one statement on each line", root)

        if stripped and not stripped.startswith(("#", "//")) and "{" not in stripped:
            next_line = lines[index] if index < len(lines) else ""
            if next_line.strip().startswith("{"):
                add(findings, path, index + 1, "brace-style", "opening braces must be on the declaration line", root)

        function_match = re.search(r"\b([A-Za-z_]\w*)\s*\(([^()]*)\)\s*(?:const\b|noexcept\b|override\b|\{|;|$)", stripped)
        function_prefix = stripped[: function_match.start()].strip() if function_match else ""
        # Calls have no return type/qualifier before the name.  Constructors
        # are the one exception and are recognized by the enclosing class.
        function_name = function_match.group(1) if function_match else ""
        prefix_has_type = bool(re.search(r"(?:^|\s)(?:void|bool|char|short|int|long|float|double|auto|[A-Za-z_]\w*(?:::\w+)*)\s*[*&]?$", function_prefix))
        looks_like_declaration = bool(function_match and (prefix_has_type or function_name == class_name) and not function_prefix.startswith(("return ", "if ", "for ", "while ", "switch ")))
        if function_match and looks_like_declaration:
            if function_name not in {"TEST_F", "TEST_P"} and not CAMEL_PATTERN.fullmatch(function_name):
                add(findings, path, index, "function-name", f"function '{function_name}' must use camelCase", root)
            parameters = function_match.group(2)
            if parameters.strip() and parameters.strip() != "void":
                for parameter in parameters.split(","):
                    parameter = parameter.split("=")[0].strip()
                    name_match = re.search(r"\b([A-Za-z_]\w*)\s*$", parameter)
                    if name_match and name_match.group(1) not in {"const", "volatile"}:
                        parameter_name = name_match.group(1)
                        # `int` and other type-only declarations have no parameter name.
                        if parameter_name in {"void", "bool", "char", "short", "int", "long", "float", "double", "auto"}:
                            continue
                        if not parameter_name.startswith("p_") and parameter_name not in {"argc", "argv"}:
                            add(
                                findings,
                                path,
                                index,
                                "parameter-name",
                                f"function parameter '{parameter_name}' must use the p_ prefix",
                                root,
                            )

            if function_name == class_name and len([p for p in parameters.split(",") if p.strip()]) == 1:
                if not re.search(r"\bexplicit\s+[^;{}]*\b" + re.escape(function_name) + r"\s*\(", stripped):
                    add(findings, path, index, "explicit-constructor", "single-argument constructors must be explicit", root)

        uninitialized = re.match(
            r"\s*(?:const\s+)?(?:bool|char|short|int|long|float|double)\s+([A-Za-z_]\w*)\s*;\s*$",
            stripped,
        ) if brace_depth > 0 else None
        if uninitialized:
            add(findings, path, index, "initialization", f"variable '{uninitialized.group(1)}' must be initialized", root)

        auto_match = re.search(r"\bauto\s+([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*(?:::\w+)*\s*\([^;]*\))", stripped)
        if auto_match:
            expression = auto_match.group(2)
            obvious = bool(re.search(r"(?:make_unique|make_shared|NewStub|\.find\s*\(|\.begin\s*\(|\.end\s*\()", expression))
            if not obvious:
                add(findings, path, index, "auto", f"the type of auto variable '{auto_match.group(1)}' is not obvious", root)

        brace_depth += line.count("{") - line.count("}")
        if class_body_depth is not None and brace_depth < class_body_depth - 1:
            class_body_depth = None
            class_name = None

    if member_names:
        for index, line in enumerate(lines, start=1):
            if re.search(r"\b(?:class|struct)\b", line):
                continue
            for member_name in member_names:
                for match in re.finditer(rf"\b{re.escape(member_name)}\b", line):
                    before = line[: match.start()]
                    in_initializer = re.search(rf":\s*(?:\w+\s*\([^)]*\),\s*)?{re.escape(member_name)}\s*\(", line)
                    declaration = (
                        not line.strip().startswith(("return ", "if ", "for ", "while ", "switch "))
                        and re.search(rf"\b(?:[A-Za-z_]\w*(?:::\w+)*\s+)+{re.escape(member_name)}\s*(?:[=;{{])", line)
                    )
                    if before.endswith("this->") or in_initializer or declaration:
                        continue
                    add(
                        findings,
                        path,
                        index,
                        "member-access",
                        f"access class member '{member_name}' through this->{member_name}",
                        root,
                    )

    # Handle declarations whose parameter list is wrapped over several lines.
    multiline_function = re.compile(
        r"\b(?:void|bool|char|short|int|long|float|double|auto|[A-Za-z_]\w*(?:::\w+)*)\s+"
        r"([A-Za-z_]\w*)\s*\((.*?)\)\s*(?:const\b|noexcept\b|override\b|\{|;)", re.S
    )
    for match in multiline_function.finditer(code):
        header = match.group(0)
        if "\n" not in header:
            continue
        line = code.count("\n", 0, match.start()) + 1
        function_name = match.group(1)
        if function_name not in {"TEST_F", "TEST_P"} and not CAMEL_PATTERN.fullmatch(function_name):
            add(findings, path, line, "function-name", f"function '{function_name}' must use camelCase", root)
        for parameter in match.group(2).split(","):
            parameter = parameter.split("=")[0].strip()
            name_match = re.search(r"\b([A-Za-z_]\w*)\s*$", parameter)
            if not name_match:
                continue
            parameter_name = name_match.group(1)
            if parameter_name in {"void", "bool", "char", "short", "int", "long", "float", "double", "auto", "const", "volatile"}:
                continue
            if not parameter_name.startswith("p_") and parameter_name not in {"argc", "argv"}:
                add(findings, path, line, "parameter-name", f"function parameter '{parameter_name}' must use the p_ prefix", root)


def check_makefile(path: Path, root: Path, findings: list[Finding]) -> None:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    program_positions = [index for index, line in enumerate(lines) if re.match(r"\s*\w+_PROGRAMS\s*[:+]?=", line)]
    library_positions = [index for index, line in enumerate(lines) if re.match(r"\s*(?:lib_LTLIBRARIES|noinst_LTLIBRARIES|lib_LIBRARIES|noinst_LIBRARIES)\s*[:+]?=", line)]
    if program_positions and library_positions:
        for position in program_positions:
            if any(library < position for library in library_positions):
                add(findings, path, position + 1, "makefile-order", "declare executable PROGRAMS before library targets", root)

    assignment = re.compile(r"^\s*([A-Za-z_]\w*)\s*[:+]?=\s*(.*)$")
    for index, line in enumerate(lines):
        match = assignment.match(line)
        if not match or match.group(1) != "SUBDIRS":
            continue
        value = match.group(2).strip()
        continuation = value.endswith("\\")
        items = value[:-1].split() if continuation else value.split()
        end = index
        while end + 1 < len(lines) and lines[end].rstrip().endswith("\\"):
            end += 1
            part = lines[end].strip()
            items.extend(part[:-1].split() if part.endswith("\\") else part.split())
        if len(items) > 1 and (not continuation or value[:-1].strip()):
            add(findings, path, index + 1, "makefile-layout", "multi-item SUBDIRS assignments must put the first item on the next line", root)
        if len(items) > 1 and not any(item.endswith("\\") for item in lines[index + 1:end + 1]):
            add(findings, path, end + 1, "makefile-layout", "continued SUBDIRS assignments must use line continuations", root)

    # Apply the same assignment layout rule to the common Automake list
    # variables.  Scalar assignments and compiler flags are intentionally not
    # interpreted as directory lists.
    list_assignment = re.compile(r"^\s*([A-Za-z_]\w*(?:_SOURCES|_CPPFLAGS|_CXXFLAGS|_LDADD|_PROGRAMS|_TESTS))\s*[:+]?=\s*(.*)$")
    for index, line in enumerate(lines):
        match = list_assignment.match(line)
        if not match:
            continue
        value = match.group(2).strip()
        if value and not value.endswith("\\") and len(value.split()) > 1:
            add(findings, path, index + 1, "makefile-layout", f"multi-item {match.group(1)} assignments must use line continuations", root)
        if value.endswith("\\") and value[:-1].strip():
            add(findings, path, index + 1, "makefile-layout", f"multi-item {match.group(1)} assignments must put the first item on the next line", root)

    # SUBDIRS order can be significant: Automake may need to build a
    # dependency before the module that links against it.  The checker
    # cannot infer that dependency graph from a Makefile reliably, so it
    # must not reject a valid dependency order as non-alphabetical.


def check_metadata(findings: list[Finding], root: Path, branch: str | None, mr_title: str | None) -> None:
    if branch:
        branch = branch.removeprefix("origin/")
        if branch not in PROTECTED_BRANCHES and not BRANCH_PATTERN.fullmatch(branch):
            findings.append(Finding("<jenkins>", 0, "branch-name", f"branch '{branch}' must use category/kebab-case"))
    if mr_title and not MR_PATTERN.fullmatch(mr_title):
        findings.append(Finding("<jenkins>", 0, "mr-name", "MR title must be '<type>: <short description>' in lowercase"))


def run_format_check(paths: list[Path], findings: list[Finding], root: Path) -> None:
    if not paths:
        return
    if not shutil_which("clang-format"):
        findings.append(Finding("<jenkins>", 0, "clang-format", "clang-format is required for --format-check"))
        return
    for path in paths:
        result = subprocess.run(
            ["clang-format", "--dry-run", "--Werror", str(path)],
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            output = result.stderr.strip().splitlines()
            detail = output[0] if output else "file is not formatted"
            findings.append(Finding(path.relative_to(root).as_posix(), 1, "clang-format", detail))


def shutil_which(command: str) -> str | None:
    for directory in os.environ.get("PATH", "").split(os.pathsep):
        candidate = Path(directory) / command
        if candidate.is_file() and os.access(candidate, os.X_OK):
            return str(candidate)
    return None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", type=Path, help="files/directories to check")
    parser.add_argument("--changed-only", action="store_true", help="check files changed from the merge target or HEAD^")
    parser.add_argument("--base-ref", help="git ref used as the merge target")
    parser.add_argument("--branch", help="branch name to validate")
    parser.add_argument("--mr-title", help="merge request title to validate")
    parser.add_argument("--format-check", action="store_true", help="also run clang-format --dry-run --Werror")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path(__file__).resolve().parents[1]
    convention_file = root / "codingConvention.md"
    if not convention_file.is_file():
        print(f"Convention file not found: {convention_file}", file=sys.stderr)
        return 2

    if args.paths:
        candidates = [(root / path).resolve() if not path.is_absolute() else path.resolve() for path in args.paths]
        missing = [str(path) for path in candidates if not path.exists()]
        outside = [str(path) for path in candidates if path != root and root not in path.parents]
        if missing:
            print(f"Input path not found: {', '.join(missing)}", file=sys.stderr)
            return 2
        if outside:
            print(f"Input path is outside the repository: {', '.join(outside)}", file=sys.stderr)
            return 2
    elif args.changed_only:
        base_ref = args.base_ref or os.environ.get("CHANGE_TARGET")
        if base_ref and not base_ref.startswith(("origin/", "refs/")):
            remote_ref = f"origin/{base_ref}"
            try:
                run_git(root, "rev-parse", "--verify", remote_ref)
                base_ref = remote_ref
            except GitCommandError:
                try:
                    run_git(root, "rev-parse", "--verify", base_ref)
                except GitCommandError:
                    print(f"Base ref not found: {base_ref}", file=sys.stderr)
                    return 2
        elif base_ref:
            try:
                run_git(root, "rev-parse", "--verify", base_ref)
            except GitCommandError:
                print(f"Base ref not found: {base_ref}", file=sys.stderr)
                return 2
        try:
            candidates = changed_paths(root, base_ref)
        except GitCommandError as error:
            print(f"Unable to determine changed files: {error}", file=sys.stderr)
            return 2
    else:
        candidates = [path for path in root.rglob("*") if path.is_file()]

    paths = source_paths(root, candidates)
    findings: list[Finding] = []
    for path in paths:
        check_path_names(path, root, findings)
        if path.suffix in CPP_SUFFIXES:
            check_cpp(path, root, findings)
        elif path.name == "Makefile.am":
            check_makefile(path, root, findings)

    branch = args.branch or os.environ.get("CHANGE_BRANCH") or os.environ.get("BRANCH_NAME")
    mr_title = args.mr_title or os.environ.get("CHANGE_TITLE") or os.environ.get("GITLAB_MERGE_REQUEST_TITLE")
    check_metadata(findings, root, branch, mr_title)
    if args.format_check:
        run_format_check([path for path in paths if path.suffix in CPP_SUFFIXES], findings, root)

    if findings:
        for finding in findings:
            print(f"ERROR: {finding}")
        print(f"\nCoding convention check failed: {len(findings)} finding(s).", file=sys.stderr)
        return 1

    scope = "changed files" if args.changed_only else "repository"
    print(f"Coding convention check passed for {scope} ({len(paths)} file(s)).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
