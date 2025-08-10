# Repository Guidelines

## Project Structure & Module Organization
- Source: `src/` contains the C++ entry point `multiblend.cpp` and included modules (`image.cpp`, `pyramid.cpp`, `functions.cpp`, `threadpool.cpp`, `geotiff.cpp`, etc.).
- Binary: built as `src/multiblend`.
- Build notes: see `build.txt` for platform-specific flags; quick script in `src/bld`.
- Licensing: GPLv3 text in `gpl-3.0.txt` and notes in `licences.txt`.

## Build, Test, and Development Commands
- Bazel build (preferred): `bazel build --config=linux //:multiblend`
- Run binary: `bazel run //:multiblend -- --help`
- Tests: `bazel test --config=linux //tests:mapalloc_test`
- Extra tests: `bazel test //tests:mapalloc_edge_test` or `bazel test //...`
- Legacy build (Linux): from `src/`: `./bld` or the `g++` line in `build.txt`.
- macOS notes: see `build.txt` for Homebrew-installed `jpeg-turbo`, `libpng`, `libtiff` paths.
- Example run: `bazel run //:multiblend -- -o out.tif img1.tif 0,0 img2.tif 200,0`

## Coding Style & Naming Conventions
- Language: C++ (C++14 compatible on macOS path); single TU includes other `.cpp` files.
- Indentation: follow existing style (tabs present in sources); keep braces on same line for functions.
- Filenames: lowercase with `.cpp`/`.h` (e.g., `pyramid.cpp`, `threadpool.h`).
- Types/classes: PascalCase (e.g., `Threadpool`, `PyramidWithMasks`).
- Functions: CamelCase/MixedCase (e.g., `Output`, `SetTmpdir`).
- Variables: snake_case for locals/fields where present.

## Testing Guidelines
- No formal unit tests in-repo. Validate changes by running with small TIFF/PNG/JPEG inputs and checking:
  - `--help` output, expected options parse.
  - Deterministic dimensions/levels reported and file written.
  - Optional diagnostics: `--timing`, `--save-seams seams.png`, `--no-output` for seam workflows.
- Prefer adding minimal reproducible image fixtures when proposing behavior changes.

## Commit & Pull Request Guidelines
- Commits: concise, imperative mood; group related changes. Suggested pattern: `fix: ...`, `build: ...`, `perf: ...`, `refactor: ...`.
- PRs should include:
  - Summary of change, motivation, and scope (modules touched in `src/`).
  - Build/run proof (command used, output snippet), and if relevant before/after timings.
  - Any dependency or flag changes (e.g., `-msse4.1`, compression behavior).
- CI: GitHub Actions runs `bazel test --config=ci //...` on push/PR.

## Security & Configuration Tips
- Dependencies: requires `libpng`, `libtiff`, `libjpeg` (Linux: `sudo apt-get install libpng-dev libtiff-dev libjpeg-dev`).
- Large images may use significant RAM/disk; use `--tempdir <dir>` and consider `--bigtiff` for outputs >4GB.
- Be cautious with untrusted inputs; libraries are native—prefer latest security updates.
