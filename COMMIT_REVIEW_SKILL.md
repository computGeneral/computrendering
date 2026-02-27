# CG1 GPU Simulator — Commit & Review Skill

A repeatable workflow for reviewing code changes, verifying correctness, and committing to the CG1 GPU Simulator repository.

---

## 1. Pre-Commit Checklist

### 1.1 Build Verification

Build on **every platform you changed code for** before committing.

**Windows (MSVC, Debug, x86):**
```powershell
# Configure (one-time or after CMake changes)
cmake -S . -B _BUILD_ -DCMAKE_BUILD_TYPE=Debug -DMSVC=1 -A Win32

# Build
cmake --build _BUILD_ --config Debug --target CG1SIM
```

**Linux (GCC, Release):**
```bash
bash tools/script/build.sh
# or manually:
cmake -S . -B _BUILD_ -DCMAKE_BUILD_TYPE=Release
cd _BUILD_ && make -j$(nproc)
```

**Verify:** Binary exists at `_BUILD_/arch/CG1SIM` (Linux) or `_BUILD_/arch/Debug/CG1SIM.exe` (Windows).

### 1.2 Rendering Verification

Run the simulator against known test traces and compare output images against reference.

#### Functional Model (FM)
```bash
# From the test trace directory (e.g., tests/ogl/trace/glxgears/)
<path_to>/CG1SIM --fm --param <path_to>/arch/common/params/CG1.params.csv \
    --arch 1.0 --trace glxgears.trace --frames 1
```
- Output: `frame0000.cm.ppm`
- Compare against reference PPM (e.g., `glxgears.ppm`)

#### Behavior Model (BM)
```bash
<path_to>/CG1SIM --bm --param <path_to>/arch/common/params/CG1.params.csv \
    --arch 1.0 --trace glxgears.trace --frames 1
```
- Output: `frame0000.bm.png`
- Compare against the same reference

#### Comparison Methods (pick one or more)

| Method | Command | Pass Criteria |
|--------|---------|---------------|
| **MD5 hash** | `md5sum frame0000.cm.ppm` | Identical to reference hash |
| **PSNR (icmp_diff)** | `icmp_diff -i1 ref.ppm -i2 test.ppm -f "%p"` | PSNR ≥ tolerance (e.g., 40 dB) or `0` (identical) |
| **Pixel diff (Python)** | See script below | 0 differing pixels |

**Quick Python pixel comparison:**
```python
from PIL import Image
ref = Image.open("reference.ppm")
out = Image.open("frame0000.cm.ppm")
ref_px, out_px = list(ref.getdata()), list(out.getdata())
diffs = sum(1 for r, o in zip(ref_px, out_px) if r != o)
print(f"{diffs} differing pixels out of {len(ref_px)}")
```

### 1.3 Regression Suite

Run the full regression suite to catch regressions beyond your targeted test:

**Windows:**
```powershell
& .\tools\script\regression\regression.ps1
```

**Linux:**
```bash
bash tools/script/regression/regression.sh
```

The regression list uses CSV format (`tools/script/regression/regression_list`):
```
test_dir, arch_version, trace_file, frames, start_frame, tolerance
```

### 1.4 Code Review Checklist

Review every changed file against the following criteria:

#### Correctness
- [ ] **Uninitialized memory**: Any `new T[n]` allocation of `U08`/`U16`/`U32`/`F32` arrays **must** be followed by `memset(ptr, 0, size)`. MSVC Debug fills `new` with `0xCD`; GCC may zero-fill, masking bugs.
- [ ] **Platform types**: Use project types (`F32`, `U32`, `S32`, `U8`, `BIT`, etc.) from `GPUType.h`, not raw C++ types.
- [ ] **Register bank init**: Shader register banks (`constantBank`, `temporaryBank`, `predicateBank`, `outputBank`, `addressBank`) must be zero-initialized after allocation.

#### Portability (Windows / Linux)
- [ ] **No `unistd.h` on MSVC**: Guard with `#ifndef _WIN32` or replace with `<io.h>` equivalents (`_access` for `access`, `F_OK` defined as `0`).
- [ ] **No `sed` in CMake**: Use `file(READ)` / `string(REPLACE)` / `file(WRITE)` instead of `execute_process(COMMAND sed ...)`. sed is unavailable on Windows without MSYS.
- [ ] **Linker flags**: Guard GCC-specific flags (e.g., `-Wl,-as-needed`) with `if(NOT MSVC)`.
- [ ] **Flex `.gen` files**: If regenerating flex output, ensure `#include <unistd.h>` is guarded with `#ifndef _WIN32`.

#### Cleanliness
- [ ] **No debug prints**: Remove all `fprintf(stderr, "DBG...")`, `printf("DEBUG...")`, `cout << "DBG"` statements before committing.
- [ ] **No cosmetic-only diffs**: Revert files where the only changes are whitespace, formatting, or comment tweaks that don't affect functionality.
- [ ] **Include guards**: Use `#ifndef __HEADER_NAME__` / `#define __HEADER_NAME__`, not `#pragma once`.

---

## 2. Staging & Committing

### 2.1 Review Staged Changes

```bash
# Check what's modified
git status

# Review each file's diff before staging
git diff <file>

# Stage only intentional changes
git add <file1> <file2> ...
```

**Never use `git add .` or `git add -A`** without first reviewing `git status`. Formatter artifacts, IDE files, and accidental edits are common.

### 2.2 Unstage Unintended Files

```bash
# Unstage a file but keep local changes
git restore --staged <file>

# Fully revert a file to HEAD (discard local changes)
git checkout HEAD -- <file>
```

### 2.3 Commit Message Format

Use a structured multi-line commit message. The first line is a concise summary (≤72 chars). The body groups changes by category.

**Template:**
```
<type>: <concise summary of the change>

<Category 1>:
- <specific change description>
- <specific change description>

<Category 2>:
- <specific change description>

<Why / context paragraph if needed>
```

**Types:** `fix`, `feat`, `build`, `refactor`, `test`, `docs`, `chore`

**Example (from the Windows porting milestone):**
```
fix: Windows MSVC build support and shader register initialization

Build system (CMake):
- Guard -Wl,-as-needed linker flag for non-MSVC compilers
- Replace sed with CMake file()/string() for .gen file patching
- Disable SNAPPY_HAVE_BMI2/SSSE3 for MSVC x86 (no 64-bit intrinsics)
- Add /D"_HAS_STD_BYTE=0" to MSVC compiler flags
- Add BUILD_D3D option to gate D3D-dependent targets

Portability:
- Replace unistd.h with io.h/_access/F_OK for MSVC in CG1SIM.cpp
- Add #ifndef _WIN32 guards for unistd.h in 4 flex .gen files
- Move 'using namespace std' inside include guard (Profiler.h)

Critical bug fix:
- Zero-initialize constantBank/temporaryBank/predicateBank after
  allocation in bmUnifiedShader.cpp (MSVC Debug fills new[] with
  0xCD, causing garbage shader constants and corrupted rendering)
```

### 2.4 Commit

```bash
git commit
# (opens editor for multi-line message)

# Or for simple single-line commits:
git commit -m "fix: zero-initialize shader register banks in bmUnifiedShader"
```

---

## 3. Post-Commit Verification

### 3.1 Verify the Commit

```bash
# Confirm the commit contents
git log --oneline -1
git show --stat HEAD

# Verify no unintended files
git diff HEAD~1 --name-only
```

### 3.2 Push & CI

```bash
git push origin <branch>
```

- CI runs automatically on push to `main` or on pull requests (`.github/workflows/ci.yml`).
- CI builds on Ubuntu, runs `regression.sh`, uploads `regression.out` as an artifact.
- Check the Actions tab on GitHub for pass/fail status.

### 3.3 If CI Fails

1. Download the `regression-results` artifact from the GitHub Actions run.
2. Open `regression.out` to see which test(s) failed and the PSNR values.
3. Reproduce locally against the failing test trace.
4. Fix, re-verify, amend or create a follow-up commit.

---

## 4. Adding a New Regression Test

When you fix a bug or add a rendering feature, add a regression test to prevent future regressions.

### 4.1 Create or Obtain a Trace

Place the trace files in `tests/ogl/trace/<test_name>/`:
- `<test_name>.trace` — the API trace file
- `<test_name>.ppm` — reference output image (rendered from a known-good build)
- Optional: `BufferDescriptors.dat`, `MemoryRegions.dat`

### 4.2 Generate the Reference Image

```bash
cd tests/ogl/trace/<test_name>/
<path_to>/CG1SIM --fm --param <path_to>/arch/common/params/CG1.params.csv \
    --arch 1.0 --trace <test_name>.trace --frames 1
cp frame0000.cm.ppm <test_name>.ppm
```

Visually inspect the PPM to confirm it looks correct before using it as reference.

### 4.3 Add to Regression List

Edit `tools/script/regression/regression_list`:
```csv
ogl/<test_name>, 1.0, <test_name>.trace, 1, 0, <tolerance_dB>
```

Fields:
| Field | Description |
|-------|-------------|
| `test_dir` | Path relative to `tests/` (e.g., `ogl/glxgears`) |
| `arch_version` | Architecture config column (e.g., `1.0`) |
| `trace_file` | Trace filename within the test directory |
| `frames` | Number of frames to simulate |
| `start_frame` | Frame offset to start from (0-based) |
| `tolerance` | Minimum PSNR (dB) to pass; `0` = byte-identical required |

**Tolerance guidelines:**
- `0` — Exact match required (deterministic tests)
- `40` — Minor floating-point differences acceptable
- `90` — Very loose (use sparingly, for tests with known platform variance)

### 4.4 Verify

```bash
bash tools/script/regression/regression.sh
```

Confirm the new test shows `PASS` in the output.

---

## 5. Common Pitfalls

| Pitfall | Symptom | Fix |
|---------|---------|-----|
| Uninitialized `new T[]` arrays | Correct on GCC, garbage on MSVC Debug | Add `memset(ptr, 0, bytes)` after every `new U08[n]` / `new F32[n]` |
| `#include <unistd.h>` | MSVC compile error: `cannot open include file` | Guard with `#ifndef _WIN32` or use `<io.h>` |
| `sed` in CMakeLists.txt | Windows build fails (sed not found) | Use `file(READ)` / `string(REPLACE)` / `file(WRITE)` |
| Debug prints left in code | Noisy console output, possible perf regression | `grep -rn "fprintf(stderr" arch/` before committing |
| Forgetting `--config Debug` on Windows | Links wrong config or can't find binary | Always specify `--config Debug` (or Release) for multi-config generators |
| Staging formatter changes | Diff noise, unrelated changes in commit | Review `git diff` per-file; `git restore --staged` unwanted files |
| Reference PPM overwritten | `rm -f *.ppm` deletes reference | Never glob-delete PPMs in test dirs; delete only `frame*.cm.ppm` / `frame*.sim.ppm` |

---

## 6. Quick Reference Commands

```bash
# ---- Build ----
cmake --build _BUILD_ --config Debug --target CG1SIM          # Windows
cd _BUILD_ && make -j$(nproc)                                  # Linux

# ---- Run FM test ----
./CG1SIM --fm --param ../arch/common/params/CG1.params.csv --arch 1.0 \
    --trace test.trace --frames 1

# ---- Run BM test ----
./CG1SIM --bm --param ../arch/common/params/CG1.params.csv --arch 1.0 \
    --trace test.trace --frames 1

# ---- Compare images ----
md5sum frame0000.cm.ppm reference.ppm                          # Quick hash check
icmp_diff -i1 ref.ppm -i2 out.ppm -f "%p"                     # PSNR value

# ---- Regression ----
bash tools/script/regression/regression.sh                     # Linux
# & .\tools\script\regression\regression.ps1                  # Windows

# ---- Git workflow ----
git status
git diff <file>
git add <file>
git commit                          # Opens editor for multi-line message
git log --oneline -3
git show --stat HEAD
```

---

*This skill document codifies the review and commit workflow for the CG1 GPU Simulator project. Keep it updated as the project evolves.*
