# Repository Guidelines

## Project Structure & Module Organization

`wir` is a cross-platform C CLI for inspecting ports and processes. Source files live in `src/` and are split by responsibility: `main.c` orchestrates CLI flow, `args.c/h` parses options, `platform.c/h` contains macOS/Linux system calls, `output.c/h` formats normal, short, JSON, and tree output, and `utils.c/h` holds shared helpers. Documentation is in `README.md` and `GUIDE.md`; images and demos are in `assets/`. Build output is generated as `wir` and `obj/` in the repository root.

## Build, Test, and Development Commands

- `make`: builds the optimized `wir` binary with `gcc`, `-Wall -Wextra -Werror`, and C11.
- `make debug`: cleans and rebuilds with debug symbols and `DEBUG` defined.
- `make clean`: removes `obj/` and the `wir` binary.
- `./wir --help`: smoke-test the built CLI and review supported options.
- `./wir --pid $$`, `./wir --port 8080`, or `./wir --all --short`: manual checks for core modes.
- `sudo make install` / `sudo make uninstall`: install or remove `/usr/local/bin/wir`.

## Coding Style & Naming Conventions

Use C11 and keep changes compatible with both macOS and Linux. Follow existing 4-space indentation, K&R-style braces, `snake_case` function and variable names, and `_t` suffixes for project structs such as `cli_args_t`. Keep platform-specific behavior behind `platform.c/h`; callers should use the platform abstraction instead of conditional system code in higher-level modules. Check allocations and system-call return values, free owned memory on all paths, and preserve readable error messages.

## Testing Guidelines

There is no automated test suite in the current tree. Before submitting changes, run `make` and perform targeted CLI smoke tests for any affected mode. For output changes, verify normal and `--json` output where relevant. For platform changes, test on the affected operating system or document the untested platform clearly in the PR.

## Commit & Pull Request Guidelines

Recent history uses short imperative or conventional-style subjects such as `fix: v1.0.9`, `fix: --interactive now report the process on log`, and `docs: Update README...`. Prefer concise subjects with a type prefix (`fix:`, `docs:`, `feat:`) when practical. PRs should describe the behavior change, list commands run, mention platform coverage, link related issues, and include screenshots or terminal output when changing user-facing output.

## Security & Configuration Tips

Process and environment inspection can require elevated privileges. Avoid logging sensitive environment values in examples or tests. Keep install paths and privilege-requiring commands explicit so contributors do not run `sudo` unexpectedly.
