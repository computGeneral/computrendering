# GLInterceptor
--------------

  1.  Modify the GLIconfig.ini file with the name you want to use for the
      captured trace files

        outputFile=[tracefile.txt]          Sets the name for the OpenGL API call
                                            trace file.  GLInterceptor may generate
                                            two additional files with fixed file names
                                            if the logBuffers option is enabled:
                                            MemoryRegions.dat and BufferDescriptors.dat

  2.  Set the capturing parameters

        format=[text|hex]                   Sets how float point numbers are dumped in the
                                            trace file:
                                                hex => as 32-bit or 64-bit hexadecimal values
                                                text => decimal format (may produce precision
                                                        problems).
        logBuffers=[0|1]                    Enables (1) or disables (0) dump buffers and
                                            textures provided to the OpenGL library (required
                                            if you want to play or simulate the file).
        dumpMode=[VERTICAL|HORIZONTAL]      Sets how statistics are generated (to avoid the
                                            Office limitation of 255 columns ...)
        frameStats=[filename]               Per frame statistics file name.  Set to 'null' to
                                            disable statistics generation
        batchStats=[filename]               Per batch statistics file name.  Set to 'null' to
                                            disable statistics generation
        traceReturn=[0|1]                   Sets if the return values from OpenGL API calls are
                                            dumped in the trace (simulator or player don't
                                            require the return values)
        firstFrame=[frame]                  Sets the start frame for dumping the OpenGL API
                                            call trace.  If set different from 1 the trace
                                            shouldn't be reproducible with GLTracePlayer or simulable
                                            but you can always try ...
        lastFrame=[frame]                   Sets the last frame for which the OpenGL API call
                                            trace is generated.  Used to limit the number
                                            of captured frames and output files sizes

  3.  Place the GLIConfig.ini and opengl32.dll in the same directory that the
      main binary of the game or application for which you want to capture
      the OpenGL API call traces (may be different than the directory where
      the game/application executable is found, for example in UT2004 it must
      be placed in the /system directory).  Obviously it doesnt' work with
      Direct3D based games or games configured to use the Direct3D rendering
      path

      The drive must have enough space for the trace (in the order of 100's of MBs
      per a hundred of frames for the tested OpenGL games) or the capturing process
      may experience 'problems'.  The capturing process may consume a significant
      amount of system memory so try to avoid using it on systems with less than
      512 MBs.

  4.  Start the game or application for which to capture the OpenGL API call trace.
      If you have plenty of luck the game or application won't crash, the trace
      will be correctly captured and will be reproducible with GLTracePlayer.  Even then
      it's very likely that the simulator won't be able to use work with the trace.
      Keep in mind that only a minimum set of OpenGL API has been implemented (and most
      of that minimum set has not been fully tested).  Keep in mind that capturing
      the same game/application and timedemo/map/scenary in different GPUs (NVidia,
      ATI, ...) may result in different API call traces and some of all of those
      captured traces may not work because of unimplemented specific OpenGL extensions
      or features

      We can capture, reproduce and simulate traces for the following games:
      UT2004, Doom3, Quake4, Chronicles of the Riddick, Prey and
      Enemy Territories: Quake Wars.

            UT2004
                => you must set the OpenGL renderer (can't be done through the game
                   menu), set the rendered to use vertex buffer objects and disable
                   vertex shaders (as the CG1 rei OpenGL library only supports
                   either fixed function T&L or ARB vertex programs).  We have tested
                   UT2004 traces captured with NVidia GPUs so traces captured with ATI
                   GPUs may not work or show render errors.

            DOOM3/QUAKE4/PREY/ETQW
                => same engine so same configuration should work.  Disable
                   two sided stencil (not supported in the simulator), set
                   the arb2 render path (or whatever other path that only
                    uses ARB vertex and fragment programs).  Enable vertex
                    buffers, disable index buffers.

            Chronicles of the Riddick
                => use arb programs, use vertex objects

------------------------------------------------------------------------------------------------

# GLTracePlayer
---------

    1. Place GLTracePlayer.exe and its configuration file GLPconfig.ini wherever you want
       on your drive

    2. Modify the GLPconfig.ini file to point to the trace files captured with GLInterceptor
       that you want to reproduce

        inputFile=tracefile.txt             OpenGL API call trace file name
        bufferFile=BufferDescriptors.dat    OpenGL API call buffer descriptors file name
        memFile=MemoryRegions.dat           OpenGL API call memory regions file name

    2. Set the player parameters

        freezeFrame=0                       Sets if the player reproduces the trace in sequence
                                            (0) or stops after each frame (0).  When freezeMode
                                            is enabled use the 'f' key to move to the next frame
        fps=100                             Maximum framerate for the reproduction (freezeMode
                                            disabled
        resTrace=1                          Use (1) the resolution specified in the trace as the
                                            reproduction resolution/window size
        resViewport=1                       Use (1) the resolution specified by the first
                                            glViewport() call
        defaultRes=800x600                  Sets the default resolution if the other two methods
                                            are disabled or fail to obtain a resolution
        startFrame=0                        Sets the reproduction start frame.  If set different
                                            from 0 GLTracePlayer will skip and avoid rendering the
                                            first n frames of the trace
        autoCapture=0                       If set to 1 captures and dumps as bmps all the
                                            rendered frames.  Manual capturing is performed using
                                            the 'C' key.
        bufferCache=100                     (ask Carlos)
        allowUndefinedBuffers=0             (ask Carlos)

    3. Execute GLTracePlayer

       While reproducing the trace this keys can be used :

        ESC     End the reproduction
        C       Capture the current frame, dumps a PNG file (only works in freeze mode)
        D       Dump the color, depth and stencil buffers (only works in freeze mode)
        F       Render the next frame (only works in freeze mode)
        B       Render the next draw call (only work in freeze mode)
        R       Benchmark the current frame (experimental, only works win freeze mode)
