# PPM Tools

This directory contains utilities for manipulating and inspecting PPM (Portable Pixel Map) image files.

## Tools

### 1. PPMConverter

`PPMConverter` is a tool for applying color transformations to PPM files. It supports linear and group remapping of color values.

**Usage:**
```bash
./PPMConverter [FUNCTION] [COLORS_FILE] [PPMFILE]
```

*   **[FUNCTION]**: The mapping function to apply. Supported values:
    *   `linear`: Linearly remaps colors based on the input colors file.
    *   `group`: Remaps colors based on grouping.
*   **[COLORS_FILE]**: A file containing a list of RGB values, or a special string specifying a color and tonality count (e.g., `red20`, `green10`, `blue5`).
    *   **File Format**: Each line should contain three integers representing R, G, and B values (0-255).
    *   **Special Format**: `colorN` where `color` is `red`, `green`, or `blue`, and `N` is the number of tonalities (2-255).
*   **[PPMFILE]**: The path to the input PPM image file.

**Examples:**
```bash
# Apply linear remapping using colors defined in colors.dat
./PPMConverter linear colors.dat input.ppm

# Apply group remapping generating 20 shades of blue
./PPMConverter group blue20 input.ppm
```

The output file will be named `output_[FUNCTION]_[PPMFILE]`.

### 2. PPMInspect

`PPMInspect` is a tool for inspecting PPM files and extracting statistics like image size, unique color counts, and min/max color values.

**Usage:**
```bash
./PPMInspect inputFile.ppm [Query] [Query_params]
```

*   **inputFile.ppm**: The path to the PPM file to inspect.
*   **[Query]** (Optional): The specific information to retrieve. If omitted, a general inspection is performed.
    *   `diffs`: Count the number of different RGB values.
    *   `size`: Show image width, height, and data size.
    *   `min`: Show the minimum RGB value(s).
    *   `max`: Show the maximum RGB value(s).
*   **[Query_params]** (Optional):
    *   For `min` and `max`: Specify the number of results to show (default is 1).

**Examples:**
```bash
# General inspection (size, diffs, min, max)
./PPMInspect image.ppm

# Get image dimensions and size
./PPMInspect image.ppm size

# Count unique colors
./PPMInspect image.ppm diffs

# Get the top 5 maximum color values
./PPMInspect image.ppm max 5
```

## Building

This project uses CMake. To build the tools:

1.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```

2.  Configure the project:
    ```bash
    cmake ..
    ```

3.  Build:
    ```bash
    make
    ```

The executables `PPMConverter` and `PPMInspect` will be created in the `build` directory.
