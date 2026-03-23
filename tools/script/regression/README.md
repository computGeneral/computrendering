# Regression Test Suite

This directory contains scripts and configuration for running regression tests on the computrender GPU Simulator.

## regression_list Format

The `regression_list` file defines the test cases to run. Each line represents a single test case and uses a Comma-Separated Values (CSV) format with the following columns:

1.  **Test Directory (`test_dir`)**:
    The directory containing the test, relative to the `tests/` directory.
    *   Example: `ogl/triangle` (resolves to `tests/ogl/trace/triangle` or `tests/ogl/triangle`).
    *   The script attempts to find the directory in `tests/<api>/trace/<name>` first, then `tests/<api>/<name>`.

2.  **Architecture Version (`arch_version`)**:
    The architecture configuration column to use from `arch/common/params/archParams.csv`.
    *   Example: `1.0`

3.  **Trace File (`trace_file`)**:
    The name of the trace file within the test directory.
    *   Example: `triangle.txt`, `glxgears.trace`

4.  **Frames (`frames`)**:
    The number of frames to simulate.
    *   Example: `1`

5.  **Start Frame (`start_frame`)**:
    The frame number to start the simulation from (0-based).
    *   Example: `0`

6.  **Tolerance (`tolerance`)**:
    The minimum Peak Signal-to-Noise Ratio (PSNR) in decibels (dB) required for the image comparison to pass.
    *   Example: `90` (effectively identical)
    *   Example: `40` (visually similar, allows for minor differences)

7.  **Mode (`mode`)**:
    Optional. Defines how the case is validated.
    *   `image` (default): requires a `<trace_name>.ppm` reference image and performs the normal image comparison.
    *   `smoke`: requires the simulator to complete successfully and emit the requested output frame, but does not require a reference image.

### Example Line

```
ogl/glxgears, 1.0, glxgears.trace, 10, 0, 40, image
```

## D3D9 Notes

- Public D3D9 traces are staged in `tests/d3d9/traces/`.
- `FruitNinja.trace` and `FruitNinja.ppm` are versioned there as the canonical image-based D3D9 regression asset.
- `download_d3d9_traces.sh` downloads the supplemental public smoke-test traces (`garfield`, `spv3-*`, and `tokitori*`) into the same directory.
- D3D9 entries can use `d3d9/traces` as the `test_dir`, with the specific trace selected by the `trace_file` column.
- The regression scripts probe the current simulator once and automatically skip D3D9 cases when the built binary reports `Unsupported API type 'd3d9'`, which is the current behavior for Linux builds without `BUILD_D3D`.
