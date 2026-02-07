## Command Line Options

| Option | Default Value | Description |
|:--|:-:|:--|
| `-d <index>` | 0 | Specify the index of the OpenCL device in the platform to execute on the sample on.
| `-p <index>` | 0 | Specify the index of the OpenCL platform to execute the sample on.
| `--file <string>` | `sample_kernel.cl` | Specify the name of the file with the OpenCL kernel source.
| `--name <string>` | `Test` | Specify the name of the OpenCL kernel in the source file.
| `--options <string>` | None | Specify optional program build options.
| `--gwx <number>` | 512 | Specify the global work size to execute.
| `--lwx <number>` | 512 | Specify the local work size grouping.
| `-a` | `false` | Show advanced options.
| `-c` | `false` | (advanced) Use `clCompileProgram` and `clLinkProgram` instead of `clBuildProgram`.
| `--compileoptions <string>` | None | (advanced) Specify optional program compile options.
| `--linkoptions <string>` | None | (advanced) Specify optional program link options.
