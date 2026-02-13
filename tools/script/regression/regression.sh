#!/bin/bash
#
# CG1 GPU Simulator — Regression Test Script
#
# Parses tools/script/regression/regression_list and runs CG1SIM (funcmodel)
# for each OGL test trace. Compares output PPMs against reference/ using
# icmp_diff (PSNR tolerance) and execution cycles via helper scripts.
# Results are written to tools/script/regression/regression.out.
#
# Usage (from anywhere):
#   bash tools/script/regression/regression.sh
#

# ---- Resolve project root from script location ----
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"

SIMULATOR="$PROJECT_ROOT/_BUILD_/arch/CG1SIM"
PARAM_CSV="$PROJECT_ROOT/arch/common/params/CG1GPU.csv"
TRACE_BASE="$PROJECT_ROOT/tests"
REG_DIR="$SCRIPT_DIR"
REG_LIST="$REG_DIR/regression_list"
REG_OUT="$REG_DIR/regression.out"

ICMP_SRC="$REG_DIR/icmp_diff.c"
ICMP_BIN="$REG_DIR/icmp_diff"
EXTRACT_TOOL="$REG_DIR/extract-exec-time.py"
COMPARE_TOOL="$REG_DIR/compare-exec-time.py"

# ---- Verify prerequisites ----
if [ ! -x "$SIMULATOR" ]; then
    echo "ERROR: Simulator binary not found at $SIMULATOR"
    echo "       Run tools/script/build.sh from the project root first."
    exit 1
fi
if [ ! -f "$REG_LIST" ]; then
    echo "ERROR: Regression list not found at $REG_LIST"
    exit 1
fi
if [ ! -f "$PARAM_CSV" ]; then
    echo "ERROR: Parameter CSV not found at $PARAM_CSV"
    exit 1
fi

# ---- Read available ARCH_VERSION columns from CSV header ----
ARCH_COLUMNS="$(head -1 "$PARAM_CSV")"

# ---- Compile icmp_diff comparison tool if needed ----
if [ -f "$ICMP_SRC" ]; then
    if [ ! -f "$ICMP_BIN" ] || [ "$ICMP_SRC" -nt "$ICMP_BIN" ]; then
        echo "Compiling comparison tool (icmp_diff)..."
        gcc -O "$ICMP_SRC" -o "$ICMP_BIN" -lm
        if [ $? -ne 0 ] || [ ! -f "$ICMP_BIN" ]; then
            echo "WARNING: Failed to compile icmp_diff, will use cmp for comparison"
            ICMP_BIN=""
        else
            echo "Done"
        fi
    fi
fi

# ---- Map regression_list test_dir to actual filesystem path ----
# regression_list uses:  ogl/triangle  → actual: tests/ogl/trace/triangle
# regression_list uses:  d3d/triangle  → actual: tests/d3d/trace/triangle
resolve_test_path() {
    local list_dir="$1"    # e.g. "ogl/triangle" or "ogl/micropolygon/0_5_size..."
    local api="${list_dir%%/*}"             # "ogl" or "d3d"
    local rest="${list_dir#*/}"             # "triangle" or "micropolygon/0_5_size..."

    # Try: tests/<api>/trace/<rest>  (current layout)
    local candidate="$TRACE_BASE/$api/trace/$rest"
    if [ -d "$candidate" ]; then
        echo "$candidate"
        return
    fi
    # Fallback: tests/<api>/<rest>  (old layout assumed by regression.pl)
    candidate="$TRACE_BASE/$api/$rest"
    if [ -d "$candidate" ]; then
        echo "$candidate"
        return
    fi
    # Not found
    echo ""
}

# ---- Initialize results ----
PASS=0
FAIL=0
SKIP=0
TOTAL=0

> "$REG_OUT"

echo "========================================"
echo " CG1 GPU Regression Test"
echo " List: $REG_LIST"
echo "========================================"
echo ""

# ---- Process each test in regression_list ----
while IFS= read -r line || [ -n "$line" ]; do
    # Skip blank lines
    trimmed="$(echo "$line" | tr -d '[:space:]')"
    [ -z "$trimmed" ] && continue

    # Parse CSV fields: test_dir, config, trace, frames, start_frame, tolerance
    IFS=',' read -r raw_dir arch_version trace_file frames start_frame tolerance <<< "$line"
    raw_dir="$(echo "$raw_dir" | xargs)"
    arch_version="$(echo "$arch_version" | xargs)"
    trace_file="$(echo "$trace_file" | xargs)"
    frames="$(echo "$frames" | xargs)"
    start_frame="$(echo "$start_frame" | xargs)"
    tolerance="$(echo "$tolerance" | xargs)"
    : "${tolerance:=0}"
    : "${start_frame:=0}"
    : "${frames:=1}"

    TOTAL=$((TOTAL + 1))

    # ---- Skip D3D tests on Linux ----
    case "$raw_dir" in
        d3d/*)
            echo "SKIP: $raw_dir (D3D not supported on Linux)"
            SKIP=$((SKIP + 1))
            continue
            ;;
    esac

    # ---- Resolve actual test directory ----
    full_test_path="$(resolve_test_path "$raw_dir")"
    if [ -z "$full_test_path" ] || [ ! -d "$full_test_path" ]; then
        echo "SKIP: $raw_dir (test directory not found)"
        SKIP=$((SKIP + 1))
        continue
    fi

    # ---- Check trace file ----
    if [ ! -f "$full_test_path/$trace_file" ]; then
        echo "SKIP: $raw_dir (trace $trace_file not found)"
        SKIP=$((SKIP + 1))
        continue
    fi

    # ---- Check reference image ----
    ref_ppm="${trace_file%.*}.ppm"
    if [ ! -f "$full_test_path/$ref_ppm" ]; then
        echo "SKIP: $raw_dir (reference $ref_ppm not found)"
        SKIP=$((SKIP + 1))
        continue
    fi

    # ---- Check arch column exists in CSV header ----
    if ! echo "$ARCH_COLUMNS" | grep -q "\\b${arch_version}\\b" 2>/dev/null; then
        # Fallback: simple comma-separated field match
        if ! echo ",$ARCH_COLUMNS," | grep -q ",${arch_version}," 2>/dev/null; then
            echo "SKIP: $raw_dir (arch '$arch_version' not found in $PARAM_CSV)"
            SKIP=$((SKIP + 1))
            continue
        fi
    fi

    echo -n "Executing $raw_dir ... "

    # ---- Clean old outputs ----
    cd "$full_test_path"
    # Do NOT delete *.ppm indiscriminately as it contains the reference image!
    rm -f frame*.cm.ppm frame*.sim.ppm output.txt stats*.*.* 2>/dev/null

    # ---- Build simulator command ----
    # --param points to the CSV param file, --arch selects the ARCH_VERSION column
    sim_cmd="$SIMULATOR --fm --param $PARAM_CSV --arch $arch_version --trace $trace_file --frames $frames"
    if [ "$start_frame" -gt 0 ] 2>/dev/null; then
        sim_cmd="$sim_cmd --start $start_frame"
    fi

    # ---- Run simulator ----
    eval "$sim_cmd" > output.txt 2>&1
    sim_exit=$?

    # ---- Rename output PPMs: frameNNNN.cm.ppm → frameNNNN.sim.ppm ----
    # This aligns with the reference naming convention used by regression.pl
    for f in frame*.cm.ppm; do
        [ -f "$f" ] || continue
        newname="${f%.cm.ppm}.sim.ppm"
        mv "$f" "$newname"
    done

    if [ $sim_exit -ne 0 ]; then
        # Check if simulation produced any output despite the crash
        any_ppm=$(ls *.sim.ppm 2>/dev/null | head -1)
        if [ -z "$any_ppm" ]; then
            echo "INTERRUPTED (exit $sim_exit)"
            echo "$raw_dir (ref tolerance $tolerance dB): INTERRUPTED (exit $sim_exit)" >> "$REG_OUT"
            FAIL=$((FAIL + 1))
            cd "$PROJECT_ROOT"
            continue
        fi
        # Simulator crashed but produced output — still compare
        echo "done (exit $sim_exit, output produced)."
    else
        echo "done."
    fi

    # ---- Compare output PPMs against reference/ ----
    echo "  Testing $raw_dir (tolerance $tolerance dB)..."
    echo -n "$raw_dir (ref tolerance $tolerance dB): " >> "$REG_OUT"

    test_passed=1

    # Collect reference PPMs and test PPMs
    ref_ppm="${trace_file%.*}.ppm"
    ref_file="$full_test_path/$ref_ppm"
    test_ppm="frame0000.sim.ppm"
    test_file="$full_test_path/$test_ppm"

    if [ ! -f "$test_file" ]; then
        test_passed=0
        echo "    FAILED — output $test_ppm not produced"
        echo "FAILED, missing output $test_ppm" >> "$REG_OUT"
    else
        if [ -n "$ICMP_BIN" ] && [ -x "$ICMP_BIN" ]; then
            # Use icmp_diff for PSNR-based comparison
            "$ICMP_BIN" -i1 "$ref_file" -i2 "$test_file" -silent -od "${test_file}_diff.ppm" > /dev/null 2>&1
            icmp_exit=$?
            psnr="$("$ICMP_BIN" -i1 "$ref_file" -i2 "$test_file" -f "%p" 2>/dev/null)"

            if [ $icmp_exit -ne 0 ]; then
                test_passed=0
                echo "    FAILED: $ref_ppm — icmp_diff error (exit $icmp_exit)"
                echo "FAILED, icmp_diff exit != 0" >> "$REG_OUT"
            elif [ "$psnr" = "0" ] || [ "$psnr" = "0.000000" ] || [ -z "$psnr" ]; then
                echo "    PASS:   $ref_ppm — identical"
                echo "PASS: $ref_ppm identical " >> "$REG_OUT"
                rm -f "${test_file}_diff.ppm"
            else
                # Compare PSNR against tolerance
                below=$(echo "$psnr $tolerance" | awk '{print ($1 < $2 && $1 != 0) ? 1 : 0}')
                if [ "$below" = "1" ]; then
                    test_passed=0
                    echo "    FAILED: $ref_ppm — PSNR ${psnr} dB < ${tolerance} dB"
                    echo "FAILED, $ref_ppm psnr $psnr dB (below tolerated)" >> "$REG_OUT"
                else
                    echo "    PASS:   $ref_ppm — PSNR ${psnr} dB >= ${tolerance} dB"
                    echo "PASS: $ref_ppm psnr $psnr dB (above tolerated) " >> "$REG_OUT"
                fi
            fi
        else
            # Fallback: byte-for-byte comparison
            if cmp -s "$ref_file" "$test_file"; then
                echo "    PASS:   $ref_ppm — identical (cmp)"
                echo "PASS: $ref_ppm identical " >> "$REG_OUT"
            else
                test_passed=0
                echo "    FAILED: $ref_ppm — differs (cmp)"
                echo "FAILED: $ref_ppm differs" >> "$REG_OUT"
            fi
        fi
    fi

    # ---- Compare execution cycles (only if images passed) ----
    if [ $test_passed -eq 1 ] && [ -f "$EXTRACT_TOOL" ] && [ -f "$COMPARE_TOOL" ]; then
        ref_stats="$full_test_path/reference/stats.frames.csv.gz"
        test_stats="$full_test_path/stats.frames.csv.gz"
        if [ -f "$ref_stats" ] && [ -f "$test_stats" ]; then
            python3 "$EXTRACT_TOOL" "$ref_stats" "$REG_DIR" > "$full_test_path/_ref_cycles" 2>/dev/null
            python3 "$EXTRACT_TOOL" "$test_stats" "$REG_DIR" > "$full_test_path/_test_cycles" 2>/dev/null
            cycle_result="$(python3 "$COMPARE_TOOL" "$full_test_path/_ref_cycles" "$full_test_path/_test_cycles" "$start_frame" 2>/dev/null)"
            rm -f "$full_test_path/_ref_cycles" "$full_test_path/_test_cycles"
            if [ -n "$cycle_result" ]; then
                echo "    Cycles: $cycle_result"
                echo ". [Frame] SpeedUp : $cycle_result" >> "$REG_OUT"
            fi
        fi
    fi

    echo "" >> "$REG_OUT"

    if [ $test_passed -eq 1 ]; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
    fi

    cd "$PROJECT_ROOT"
done < "$REG_LIST"

echo ""
echo "========================================"
printf " Results: %d passed, %d failed, %d skipped (of %d)\n" $PASS $FAIL $SKIP $TOTAL
echo "========================================"
echo " Details: $REG_OUT"

[ $FAIL -gt 0 ] && exit 1
exit 0
