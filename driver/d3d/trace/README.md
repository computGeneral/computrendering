------------------------------------------------------------------------------------------------
PIX Traces
-----------

Use the PIX tool in the DirectX SDK to capture single frame or whole capture PIXRun traces from
a game or an application.

Microsoft provides an API to game developers to disable PIX trace capturing capabilities so you may
not be able to capture traces from some/many games without a bit of hacking.  We have used our
independently developed DXInterceptor tools to capture D3D9 game traces and then capture them with
PIX.


PIX Player
-----------

D3DTracePlayer

    1.  Place D3DTracePlayer.exe, d3d9pixrunplayer.ini and PIXLog.cfg in a directory

    2.  Set the PIX player parameters defined in "d3d9pixrunplayer.ini" and "PIXLog.cfg" (for logging)

    3.  From a command line window:
        > D3DTracePlayer filename

        Where filename is the path to a PIXRun trace file.  The player supports PIXRun traces compiled
        with gzip (renamed to PIXRunz).

        While reproducing the trace this keys can be used :

            ESC     End the reproduction
            ENTER   Start/Stop trace reproduction
            B       Render the next draw call
            F       Render the next frame
            A       Enable/Disable frame autocapture (PNG files per draw call or frame)
            C       Capture frame (PNG file)
            D       Dump color, depth and stencil buffers (may not work for depth and stencil)

------------------------------------------------------------------------------------------------