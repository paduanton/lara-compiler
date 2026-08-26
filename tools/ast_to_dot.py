#!/usr/bin/env python3
"""Convert lara's textual AST dump (stdin) into a Graphviz .dot file (stdout).

Usage: ./lara < program.lc | python3 tools/ast_to_dot.py > ast.dot
       dot -Tsvg ast.dot -o ast.svg   # optional, if graphviz is installed
"""
import re
import sys

START_MARKER = "=== Árvore de Sintaxe Abstrata (AST) ==="
END_MARKER_PREFIX = "===="

NODE_LINE = re.compile(r"^\[([\w?]+)\](.*?)\(linha (\d+)\)\s*$")
CHILD_LABEL_LINE = re.compile(r"^filho\[\d+\]:\s*$")


def extract_ast_block(lines):
    in_block = False
    block = []
    for line in lines:
        stripped = line.rstrip("\n")
        if stripped == START_MARKER:
            in_block = True
            continue
        if in_block and stripped.startswith(END_MARKER_PREFIX):
            break
        if in_block:
            block.append(stripped)
    return block


def parse_node_line(line):
    indent = len(line) - len(line.lstrip(" "))
    content = line.strip()
    match = NODE_LINE.match(content)
    if not match:
        return None
    node_type, middle, lineno = match.groups()
    middle = middle.strip()
    value = None
    if middle.startswith('"') and middle.endswith('"') and len(middle) >= 2:
        value = middle[1:-1]
    return indent // 2, node_type, value, lineno


def build_tree(block):
    nodes = []   # (id, type, value, lineno)
    edges = []   # (parent_id, child_id)
    stack = []   # list of (indent_level, node_id)
    next_id = 0

    for raw_line in block:
        if not raw_line.strip():
            continue
        if CHILD_LABEL_LINE.match(raw_line.strip()):
            continue

        parsed = parse_node_line(raw_line)
        if parsed is None:
            continue
        indent, node_type, value, lineno = parsed

        node_id = f"n{next_id}"
        next_id += 1
        nodes.append((node_id, node_type, value, lineno))

        while stack and stack[-1][0] >= indent:
            stack.pop()
        if stack:
            edges.append((stack[-1][1], node_id))
        stack.append((indent, node_id))

    return nodes, edges


def escape(text):
    return text.replace("\\", "\\\\").replace('"', '\\"')


def render_dot(nodes, edges):
    out = ["digraph AST {", '  node [shape=box, fontname="monospace"];']
    for node_id, node_type, value, lineno in nodes:
        label = node_type if value is None else f"{node_type}\\n{escape(value)}"
        label += f"\\n(line {lineno})"
        out.append(f'  {node_id} [label="{label}"];')
    for parent_id, child_id in edges:
        out.append(f"  {parent_id} -> {child_id};")
    out.append("}")
    return "\n".join(out) + "\n"


def main():
    lines = sys.stdin.readlines()
    block = extract_ast_block(lines)
    if not block:
        sys.stderr.write("ast_to_dot: no AST block found in input\n")
        sys.exit(1)
    nodes, edges = build_tree(block)
    sys.stdout.write(render_dot(nodes, edges))


if __name__ == "__main__":
    main()
