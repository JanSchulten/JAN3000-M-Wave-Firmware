# M-Vave-OP1

Custom firmware for the M-VAVE FM-1, a fork of Felucca (GPL-3.0-only). For any firmware work, load the
`fm1-firmware` skill (`.claude/skills/fm1-firmware/SKILL.md`) first: it holds the budgets, the test loop and the
safety rules for installing on a real FM-1.

## graphify

This project can have a knowledge graph of its code at `graphify-out/` (graphify, `.claude/skills/graphify/`): the
functions, the files that hold them and who calls whom, with community structure and god nodes. It is built
locally from the code alone (tree-sitter, no LLM, no API key) and is not in git (`.gitignore`): rebuild it in a
fresh checkout or container, ~10 s:

```sh
uv tool install graphifyy==0.9.97          # or pipx / pip; setup_env.sh does this
graphify extract . --code-only             # graphify-out/graph.json
graphify cluster-only . --no-label         # GRAPH_REPORT.md, graph.html
```

Rules:
- When graphify-out/graph.json exists, run `graphify query "<question>"` before your first grep, search or
  source-file read for a codebase question, and put this rule in every subagent prompt that explores code. Read
  raw files only for lines the graph or an earlier read already located. Use `graphify path "<A>" "<B>"` for
  relationships, `graphify explain "<concept>"` for focused concepts and `graphify affected "<X>"` for what a change
  to X reaches. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface
  enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
- What the graph cannot see here: the firmware is a unity build (`firmware/src/felucca.c` includes the other `.c`
  files), so most calls between files are matched by name and marked INFERRED; a few macro-heavy files parse only in
  part (graphify names them when it builds). Calls through function pointers (engine tables, page hooks) and what the
  audio ISR reaches are not in it. For those, and for anything about budgets, timing or stored formats, the code and
  the `fm1-firmware` references are the authority.
