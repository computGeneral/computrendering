# computrendering
===============

## Introduction
-------------

The computrendering project has arch named CG1.
CG1 is a GPU simulator with OpenGL and D3D9 API implementations that can be used
to simulate PC Windows game traces.

Referenced from project "https://attila.ac.upc.edu"

Some of the techniques and algorithms implemented in the simulator may be covered
by patents (for example DXTC/S3TC compression or the Z compression algorithm).

## Requirements
------------------------
CG1 depends on thirdparty tools and libraries. The following are required:

**Linux:**

    cmake    (>= 3.13.1)
    flex
    bison
    libpng-dev
    g++ / clang++

**Windows:**

    cmake    (>= 3.13.1)
    flex
    bison
    mingw    (for POSIX headers)
    Visual Studio 2022 or higher


#Contents of the package
------------------------
    README.txt                      This file.
    USAGE.txt                       Basic instructions to capture and simulate a trace.
    LICENCE                         MIT/FreeBSD License
    _BUILD_/bin/                      Directory where the compiled binaries are placed.
    _BUILD_/lib/                      Directory where the compiled libraries are placed.
    
    arch/                           Source code for the CG1 arch modeling, API framework and tools.
        Makefile                    Local makefile
        CG1Gpu/                     Simulator and behaviorModel binary related classes and source code.
        common/                     Common support classes.
            params/                     Configuration files for the simulator (must be renamed to CG1GPU.ini before use).
        behvmodel/                       behavior modeling classes.
        funcmodel/                       functional modeling classes.
            base/                   Base simulation classes (Box, Signal, Statistics).
            Cache/                  Cache source code.
            Clipper/                Clipper stage related simulated classes.
            CommandProcessor/       Command Processor stage  source code.
            DisplayController/      Display Controller stage source code.
            FragmentOperations/     ROP (Z and Color) stage source code.
            MemoryController/       Memory Controller stage source code.  Implements a simple memory model.
            MemoryControllerV2/     Memory Controller stage source code.  Implements an accurate GDDR model.
            PrimitiveAssembly/      Primitive Assembly stage source code.
            Rasterizer/             Rasterizer (Triangle Setup, Fragment Generation, Hierarchical Z and Fragment FIFO) stage source code.
            Streamer/               Streamer (Vertex Fetch and Vertex Post-shading Cache) stage source code.
            Shader/                 Shader stage source code.
            VectorShader/           Shader stage source code.  New implementation.
        archmodel/                  Architecture (cycle approximate full system) modeling classes.
        perfmodel/                  Performance (cycle accurate gpu ip) modeling classes.
        engymodel                   power(energy) consumption modeling classes.

        utils/
            CG1ASM/                  CG1 shader ISA tools.
            TestCreator/             Framework used to create simple non-API tests for the CG1 simulator.
            CGAPI/      CG1 Low Level API (computGeneral Language).  An experimental API to create tests for the simulator.
    
    driver/                          API related source code.

        (metaStream)traceExtractor/         Extracts a region of an MetaStream trace (experimental).
        (metaStream)traceTranslator/        Runs the CG1 API implementation, without simulation or emulation for a trace and generates an MetaStream trace (experimental).

        (hal)HAL/                    CG1 GPU driver.

        (gal/GAL)/GAL                The CG1 Common Graphics Abstraction Layer(GAL) definition and implementation.
        (gal/GALx)/GALX              Extensions for the GAL.
        (gal/test)/GALTest           Test tools for the GAL 

        (ogl/inc)                    OpenGL headers.
        (ogl)OGL14/                  OpenGL API implementation (old version not based on the GAL, TODO remove).
        (ogl)OGL2/                   OpenGL API implementation based on the GAL.
        (ogl/trace)GLInstrument/     OpenGL trace instrumentation.
        (ogl/trace)GLInstrumentTool/ OpenGL trace instrumentation.
        (ogl/trace)GLInterceptor/    OpenGL trace capturer.
        (ogl/trace)GLTracePlayer/    OpenGL trace player.


        (d3d/inc)                    D3D9 headers.
        (d3d)D3D9/                   D3D9 API implementation based on the GAL.
        (d3d/trace)D3DTracePlayer/   D3D9 PIX trace player, main func (playback the trace file before run on model)
        (d3d/trace)D3DTraceCore/     D3D9 PIX trace player, core lib  (caller: D3DTracePlayer, TraceDriverD3D ).
        (d3d/trace)D3DTraceStat/     D3D9 PIX trace statistics and instrumentation tool (plugin for the D3DTraceCore).

        utils/                       Miscellaneous classes used by the API source.
            D3DApiCodeGen/           Automatized code generation for the D3D9 PIX trace tools.
            OGLApiCodeGen/           Automatized code generation for the OpenGL trace tools.
    
     tests/                          CG1 simulator mini-regression/simulator(mdl) tests.
        config/                      Configuration files used for the CG1 simulator mini-regression test.
        d3d/                         D3D9 traces for the CG1 simulator mini-regression test.
        ogl/                         OpenGL traces for the CG1 simulator mini-regression test.
        script/                      Scripts for the CG1 simulator mini-regression test.
        mdl/                         Tools for testing different parts of the CG1 simulator.

    tools/                           Miscellaneous tools.
        PixParser/                   PIXRun file parser.
        PPMConverter/                Coloring tools for PPMs.
        SnapshotExplainer/           Tools for CG1 simulator state snapshots.
        TraceViewer/                 Signal Trace Visualizer
        DXInterceptor/               An independently developed D3D9 trace capturer and player

## How to compile on GNU/Linux (CMake)
----------------------------

### Quick Build

Use the provided `build.sh` script under `tools/script` from the project root:

```bash
bash tools/script/build.sh
```

This will:
1. Create (or reuse) the `_BUILD_` directory.
2. Run CMake configuration.
3. Build all targets with `make`.

The compiled binaries are placed in `_BUILD_/arch/` (CG1SIM simulator) and
`_BUILD_/driver/` (trace tools).

### Manual CMake Build

```bash
mkdir -p _BUILD_ && cd _BUILD_
cmake ..
make -j$(nproc)
```

### Legacy Makefile Build (deprecated)

    Usage: make [clean | simclean]
           make [what options]

    clean      - Delete all OBJs and binary files.
    simclean   - Delete only simulator related OBJs, and binary files.

    what:
           all                  - Build CG1 simulator and tools
           CG1GPU               - Build CG1 simulator and behaviorModel
           traceTranslator      - Build traceTranslator
           traceExtractor   - Build traceExtractor

    options:
            CONFIG={ debug  | profiling | optimized | verbose }
                debug     - Compile with debug information
                profiling - Compile with profiling information
                optimized - Maximum optimization (default)

            CPU=<cpu>
                <cpu>   - Target cpu for compilation. This lets apply some optimizations.
                          Possible values: x86, x86_64, pentium4, athlon, core2

            VERBOSE={ yes | no }
                yes     - Activate debug messages
                no      - Deactive debug messages (default)

            PLATFORM=<platf>
                <platf> - Target platform for compilation.
                          Possible values: linux (default), cygwin



#How to compile on Windows
--------------------------

A) CG1 simulator and behaviorModel binaries

    -> Open the CG1 solution file in "CG1.sln" with Visual Studio 2022 or higher
    -> Select the target architecture
        - Win32 for the 32-bit build
        - x64 for 64-bit build
    -> Select the configuration
        - Optimized : full optimization build
        - Debug     : debug build
        - Profile   : profiling build
    -> Build the selected project
        - CG1SIM  : CG1 simulator
    -> The compiled binary can be found on the "BUILD/bin/" directory

B) OpenGL tools

    -> Open the CG1 solution file "CG1.sln" with Visual Studio 2022 or higher
    -> Set the target architecture to Win32
    -> Select the configuration
        - Optimized ; full optimization build
        - Debug     : debug build
        - Profile   : profiling build
    -> Build the selected project
        - GLInterceptor : OpenGL trace capturer, creates opengl32.dll
        - GLTracePlayer      : OpenGL trace player
    -> The compiled library or binaries can be found on the "BUILD/bin/" directory

C) D3D9 PIX player

    -> Open the CG1 solution file "CG1.sln" with Visual Studio 2022 or higher.
    -> Set the target architecture to Win32
    -> Select the configuration
        - Optimized ; full optimization build
        - Debug     : debug build
        - Profile   : profiling build
    -> Build the D3DTracePlayer4Windows project
    -> The compiled binary can be found on the "BUILD/bin" directory

D) DXInterceptor

    1) Build the external libraries first
        -> Open the DXInterceptor external libraries solution file "src\extern-libs\DXInterceptorLibs\DXInterceptorLibs.sln"
           with Visual Studio 2022 or higher
        -> Select the configuration
        -> Build all
    2) Build the DXInterceptor tools
        -> Open the DXInterceptor solution file "src\trace\DXInterceptor\DXInterceptor.sln" with Visual Studio 2022 or higher
        -> Select the configuration
        -> Build all

E) Tools in the CG1 solution

    -> Open the CG1 solution file in "CG1.sln" with Visual Studio 2022 or higher
    -> Select the target architecture
        - Win32 for the 32-bit build
        - x64 for 64-bit build
    -> Select the configuration
        - Optimized : full optimization build
        - Debug     : debug build
        - Profile   : profiling build
    -> Build the selected project
        - CGAPI
        - traceExtractor
        - traceTranslator
        - MemoryControllerTest
        - ShaderProgramTest
        - TestCreator

F) Other tools

A GNU/Linux makefile or Visual Studio project may be on the tool directory.

## How to run the CG1 funcmodel and bhavmodel
---------------------------------------------

### Quick-start: Running after build

After a successful build with `tools/script/build.sh`, you can test the simulator immediately:

```bash
cd _BUILD_/arch

# Copy the configuration file (required at runtime)
cp ../../arch/common/params/CG1GPU.ini .

# Run with an OpenGL trace (simplefog example)
./CG1SIM ../../tests/ogl/trace/simplefog/tracefile.ogl.txt.gz
```

### General Usage

    1. Copy the simulator binary (CG1SIM), a configuration file (CG1GPU.ini) and the
       trace files into the same directory (the configuration file described in doc/CG1_Architecture_Specification.docx)
    2. Execute the simulator
        > [CG1SIM]
        > [CG1SIM] filename
        > [CG1SIM] filename n
        > [CG1SIM] filename n m

        When no parameters are specified the trace filename and all other parameters are obtained from the configuration file (which must be named 'CG1GPU.ini').

        When parameters are used:

            filename        OpenGL trace file (usually tracefile.txt or tracefile.txt.gz) 
                            (see "./driver/ogl/trace/README.md" for more details about OGL Trace Generation)
                            or
                            PIX trace file (PIXRun or PIXRunz) (Windows only)
                            (see "./driver/d3d/trace/README.md" for more details about D3D Trace Generation)

            n               Number of frames to render (if n < 10K)
                            or
                            Cycles to simulate (if n > 10K)
            m               Start simulation frame (when m > 0 the simulator will skip
                            m frames from the trace before starting the simulation)

    ##4. Get the simulation output
       The simulation has four outputs:
        a) standard output
        ```
            Standard output information:
                I)   Some of the simulation parameters obtained from the configuration file
                     and the command line
                II)  Miscelaneous messages (may be described in future documentation)
                III) Simulator state:

                        A '.' indicates that a number of cycles (10K by default) has passed

                        A 'B' indicates that a OpenGL draw call (or batch) has been fully
                        processed.

                        Memory usage messages from the driver after each frame finalizes.

                        Messages from the ColorWrite pipelines with the simulated cycle at
                        which the the last fragment in a frame was processed.  Used to
                        measure the per frame simulation time

                        Messages from the Display Controller unit with the cycle when the frame is dumped
                        to a file.  Used to measure the per frame simulation time

                        Misc messages and warnings from the simulator and OpenGL library
                        Misc panic and crash messages from the simulator and OpenGL library

                    If after many cycles (millions) no 'B' is ever printed it is very likely
                    that the simulator is in a deadlock (it uses to happen a lot when using
                    buggus configurations).

                IV)  End of simulation message with the last simulated cycle and dynamic
                     memory usage statistics
            Example:
                > ~/cg1gpu/bin/CG1SIM texComb256.txt 1
                Simulator Parameters.
                --------------------

                Input File = texComb256.txt
                Signal Trace File = signaltrace.txt
                Statistics File = stats.csv
                Statistics (Per Frame) File = stats.frame.csv
                Statistics (Per Batch) File = stats.batch.csv
                Simulation Cycles = 0
                Simulation Frames = 1
                Simulation Start Frame = 0
                Signal Trace Dump = disabled
                Signal Trace Start Cycle = 0
                Signal Trace Dump Cycles = 10000
                Statistics Generation = enabled
                Statistics (Per Frame) Generation = enabled
                Statistics (Per Batch) Generation = enabled
                Statistics Rate = 10000
                OptimizedDynamicMemory => FAST_NEW_DELETE enabled.  Ignoring third bucket!
                Using OpenGL Trace File as simulation input.
                Simulating 1 frames (1 dot : 10K cycles).


                Warning: clearZ implemented as clear z and stencil (optimization)
                ............B.Dumping frame 0
                HAL => Memory usage : GPU 263 blocks | System 0 blocks
                ColorWrite => End of swap.  Cycle 132570
                ColorWrite => End of swap.  Cycle 132570
                ColorWrite => End of swap.  Cycle 132570
                ColorWrite => End of swap.  Cycle 132570
                .DAC => Cycle 141952 Color Buffer Dumped.

                END Cycle 141954 ----------------------------
                Bucket 0: Size 65536 Last 731 Max 0 | Bucket 1: Size 16384 Last 0 Max 0 | Bucket 2: Size 65536 Last 0 Max 0


                End of simulation
        ```

        b) the rendered frames
            The rendered frames as PPM files
        c) 'latency' or per quad map frames
            Per pixel quad (2x2) map with some property for example overdraw or
            per pixel latency (will be described in future documentation)
        d) statistics files
            Simulator statistics file (will be described in future documentation)
        e) signal trace dump file
            Used for debugging purposes (will be described in future documentation)


#How to run the CG1 simulator or behaviorModel with MetaStream traces (experimental)
---------------------------------------------------------------------------
    1. Copy the simulator (CG1SIM), the OpenGL to 'MetaStream' trace translator tool (traceTranslator)
       a configuration file (CG1GPU.ini) and the OpenGL trace files into the same directory
    2. Execute the translator tool to generate the MetaStream trace file for the input OpenGL tracefile:
        > traceTranslator
        > traceTranslator filename
        > traceTranslator filename n
        > traceTranslator filename n m

        When no parameters are specified the trace filename (only for the OpenGL API call
        trace file, the memory regions and buffer descriptors files have fixed file names)
        and all other parameters are obtained from the configuration file (which must
        be named 'CG1GPU.ini')

        When parameters are used:

            filename        Name of the OpenGL API call trace file
            n               Number of frames to translate
            m               First frame to translate (when m > 0 the translator tool will skip
                            m frames from the trace before starting with the translation process)

       The output of the translator tool is a file named 'CG1.MetaStream.tracefile.gz' which stores all the
       MetaStream objects (the commands that control the rendering process in the CG1 GPU)
       that would be issued to the CG1 GPU to render the selected frames of the input OpenGL trace file.

       The first two steps can be skipped if the resulting MetaStream trace file is stored for later use.  The HDP
       trace file could be generated for all the frames in the input OpenGL trace and then reused for a
       faster simulation.  The name of the output MetaStream tracefile can changed as the simulator (CG1SIM)
       accepts the trace file name as a parameter.  The only limitation is that some of the parameters in the
       configuration file (CG1GPU.ini) used to generate the translated MetaStream trace file should be the same in the
       configuration file (CG1GPU.ini) used by the simulator (memory sizes, texture block dimensions, rasterization
       tile dimensions, bytes per pixel, double buffering and fetch rate).

    ##3. Execute the simulator
        > [CG1SIM]
        > [CG1SIM] filename
        > [CG1SIM] filename n
        > [CG1SIM] filename n m

        When no parameters are specified the trace filename (MetaStream trace file) and all other parameters
        are obtained from the configuration file (which must be named 'CG1GPU.ini')

        When parameters are used:
            filename        Name of the MetaStream trace file
            n               Number of frames to render (if n < 10K)
                            or
                            Cycles to simulate (if n > 10K)
            m               Start simulation frame (when m > 0 the simulator will skip
                            m frames from the trace before starting the simulation)

    ##4. Get the simulation output
        (Same as the simulator output, detailed in first section)



## Supported Trace Formats
---------------------------------------------------------------------------

CG1 supports multiple trace formats for different use cases:

| Format | Extension | Capture Tool | Status | Use Case |
|--------|-----------|--------------|--------|----------|
| **GLInterceptor** | `.txt`, `.txt.gz`, `.ogl.txt.gz` | GLInterceptor (Windows DLL) | ✅ Full support | Debugging, regression tests |
| **Apitrace** | `.trace` | apitrace (cross-platform) | 🚧 Experimental | Standard format, community traces |
| **D3D9 PIX** | `.PIXRun`, `.PIXRunz` | PIX / DXInterceptor | ✅ Windows only | D3D9 applications |
| **MetaStream** | `.tracefile.gz` | traceTranslator tool | ✅ Optimized | Pre-translated GPU commands |

### GLInterceptor Format (CG1 Native)

- **Capture**: Windows only (opengl32.dll interceptor)
- **Format**: Text-based, human-readable function calls
- **Auxiliary files**: `BufferDescriptors.dat`, `MemoryRegions.dat`
- **See**: `driver/ogl/trace/README.md`

### Apitrace Format (Experimental)

- **Capture**: Cross-platform (`apitrace trace --api gl <app>`)
- **Format**: Binary with Snappy compression
- **Integration status**: Parser and driver implemented, parameter conversion in progress
- **See**: `driver/ogl/trace/README_APITRACE.md`
- **Limitation**: Currently returns empty MetaStream (no simulation output yet)

### MetaStream Format (Optimized)

- **Generation**: `traceTranslator` converts GLInterceptor traces to binary MetaStream
- **Format**: Pre-compiled GPU command stream (bypasses API emulation)
- **Performance**: Fastest simulation (no API overhead)

## Testing & Result Verification
---------------------------------------------------------------------------

The project ships with mini-regression traces under `tests/`. Below is the
verified end-to-end testing flow on GNU/Linux.

### Available test traces (OpenGL)

| Trace            | Path                                                   |
|------------------|--------------------------------------------------------|
| simplefog        | `tests/ogl/trace/simplefog/tracefile.ogl.txt.gz`      |
| triangle         | `tests/ogl/trace/triangle/tracefile.ogl.txt.gz`       |
| torus            | `tests/ogl/trace/torus/tracefile.ogl.txt.gz`          |
| bunny            | `tests/ogl/trace/bunny/tracefile.ogl.txt.gz`          |
| blendedcubes     | `tests/ogl/trace/blendedcubes/tracefile.ogl.txt.gz`   |
| 8lights          | `tests/ogl/trace/8lights/tracefile.ogl.txt.gz`        |
| billboard        | `tests/ogl/trace/billboard/tracefile.ogl.txt.gz`      |
| copyteximage     | `tests/ogl/trace/copyteximage/tracefile.ogl.txt.gz`   |
| micropolygon     | `tests/ogl/trace/micropolygon/tracefile.ogl.txt.gz`   |
| spaceship        | `tests/ogl/trace/spaceship/tracefile.ogl.txt.gz`      |
| spheremap        | `tests/ogl/trace/spheremap/tracefile.ogl.txt.gz`      |

### Step 1: Build

```bash
bash tools/script/build.sh
```

### Step 2: Run the simulator

```bash
cd _BUILD_/arch

# Ensure the config file is present
cp ../../arch/common/params/CG1GPU.ini .

# Run a test trace (e.g. simplefog)
./CG1SIM ../../tests/ogl/trace/simplefog/tracefile.ogl.txt.gz
```

The simulator produces the following outputs in the current working directory:

| Output file                  | Description                                                    |
|------------------------------|----------------------------------------------------------------|
| `frame0000.cm.ppm`          | Rendered frame (funcmodel, cycle-accurate simulation)          |
| `frame0000.bm.png`          | Rendered frame (bhavmodel, fast emulation)                     |
| `stats.frames.csv.gz`       | Per-frame statistics                                           |
| `stats.batches.csv.gz`      | Per-batch (draw call) statistics                               |
| `stats.general.csv.gz`      | Accumulated statistics at a configurable cycle rate            |

### Step 3: Verify rendered output

Each test trace directory contains a reference PPM under `tests/ogl/trace/<name>/`.
Compare the simulator-generated PPM against the reference:

```bash
# Byte-for-byte comparison of funcmodel output
diff _BUILD_/arch/frame0000.cm.ppm tests/ogl/trace/simplefog/frame0000.cm.ppm

# If using cmp (silent on match):
cmp _BUILD_/arch/frame0000.cm.ppm tests/ogl/trace/simplefog/frame0000.cm.ppm && echo "PASS" || echo "FAIL"
```

A **byte-for-byte identical** PPM file indicates the funcmodel rendering is correct.

### Step 4: Verify statistics (optional)

Statistics files may show minor numerical variations across different compilers or
platforms due to floating-point precision and cycle-counting differences. To inspect:

```bash
# Decompress and diff
zdiff _BUILD_/arch/stats.frames.csv.gz tests/ogl/trace/simplefog/stats.frames.csv.gz
```

Small differences in cycle counts (< 0.1%) are generally acceptable when using
different compilers, optimization levels, or platforms.

### Step 5: Verify bhavmodel PNG output

The bhavmodel produces a PNG image (`frame0000.bm.png`). Verify it is non-empty and valid:

```bash
file _BUILD_/arch/frame0000.bm.png
# Expected: PNG image data, 400 x 400 (or the trace's viewport size)
```

### Automated regression example

A simple script to run all traces and verify output:

```bash
#!/bin/bash
PASS=0; FAIL=0
for trace_dir in tests/ogl/trace/*/; do
    name=$(basename "$trace_dir")
    trace_file="$trace_dir/tracefile.ogl.txt.gz"
    ref_ppm="$trace_dir/frame0000.cm.ppm"

    [ ! -f "$trace_file" ] && continue
    [ ! -f "$ref_ppm" ]    && continue

    # Create isolated run directory
    run_dir="_BUILD_/test_$name"
    mkdir -p "$run_dir"
    cp arch/common/params/CG1GPU.ini "$run_dir/"

    # Run simulator
    (cd "$run_dir" && ../_BUILD_/arch/CG1SIM "../../$trace_file" > /dev/null 2>&1)

    # Compare
    if cmp -s "$run_dir/frame0000.cm.ppm" "$ref_ppm"; then
        echo "PASS: $name"
        PASS=$((PASS+1))
    else
        echo "FAIL: $name"
        FAIL=$((FAIL+1))
    fi
done
echo "Results: $PASS passed, $FAIL failed"
```

### Known platform differences

| Item                     | Notes                                                         |
|--------------------------|---------------------------------------------------------------|
| D3D / PIX traces         | Only supported on Windows (requires `TraceDriverD3D`)         |
| PNG output (bhavmodel)   | Requires `libpng-dev` on Linux; uses GDI+ on Windows         |
| Statistics CSV delimiter | May use `,` or `;` depending on platform/locale settings      |
| Cycle counts             | Minor variations (< 0.1%) across compilers are expected       |

