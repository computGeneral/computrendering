# Regression Test Suite

This directory contains scripts and configuration for running regression tests on the CG1 GPU Simulator.

## regression_list Format

The `regression_list` file defines the test cases to run. Each line represents a single test case and uses a Comma-Separated Values (CSV) format with the following columns:

1.  **Test Directory (`test_dir`)**:
    The directory containing the test, relative to the `tests/` directory.
    *   Example: `ogl/triangle` (resolves to `tests/ogl/trace/triangle` or `tests/ogl/triangle`).
    *   The script attempts to find the directory in `tests/<api>/trace/<name>` first, then `tests/<api>/<name>`.

2.  **Architecture Version (`arch_version`)**:
    The architecture configuration column to use from `arch/common/params/CG1GPU.csv`.
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

### Example Line

```
ogl/glxgears, 1.0, glxgears.trace, 10, 0, 40
```
