set driver_Version=wddm_cc1dcd798_hw
set REGEDIT_PATH=HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}\0000\UMD
set TRACE_PATH=D:\TOOLS\Apitrace_x64_Release_version5.0\bin
set CURRENT_PATH=%~dp0

@REM mtpanel: 
@REM mc cp oss/madmax/zx/Tools/mtpanel.exe .

set today=%date:~0,4%-%date:~5,2%-%date:~8,2%
echo %today%

@REM copy /Y %driver_Version%\fre_win10_amd64\mtdxum64.dll %TRACE_PATH%

call driver_check.bat D:\DRIVER\%driver_Version% C:\Windows\System32\
pause

pushd %TRACE_PATH%

echo %date% %time%  > run.time


@REM -----------RUN PART-------------
@REM call:run "D:\syros\trace\3DMark11_DeepSea.trace" cb  gfx_start=0x000000cb comp_start=0x00000009
@REM call:run "D:\syros\trace\3DMark11_DeepSea.trace" frame  frame_start=0x00000004
call:run "D:\gene\trace\ludashi.trace"


echo %date% %time%  >> run.time
pause
popd

goto:eof

:run
set trace=%1
set name=%~n1
set cur_dir=%CURRENT_PATH%\%driver_Version%_%name%
echo %cur_dir%


call:set_range %*

if exist %cur_dir% (
    del /f /s /q %cur_dir%
) else (
    mkdir %cur_dir%
)

if exist pipeline_dumps (
    del /f /s /q pipeline_dumps
) else (
    mkdir pipeline_dumps
)

if exist compiler_dumps (
    del /f /s /q compiler_dumps
) else (
    mkdir compiler_dumps
)

if exist perfLog (
    del /f /s /q perfLog 
) else (
    mkdir perfLog
)
echo %date% %time% %name% %gfx_start% %gfx_end% %comp_start% %comp_end% >> run.time
echo %date% %time% %name% %gfx_start% %gfx_end% %comp_start% %comp_end% start >> %cur_dir%/run.time
d3dretrace.exe --snapshot=frame %1
echo %date% %time% %name% %gfx_start% %gfx_end% %comp_start% %comp_end% >> run.time
echo %date% %time% %name% %gfx_start% %gfx_end% %comp_start% %comp_end% end >> %cur_dir%/run.time
move *.log %cur_dir%
move *.png %cur_dir%
move *.json %cur_dir%
move compiler_dumps %cur_dir%
move pipeline_dumps %cur_dir%
move perfLog %cur_dir%
@REM mc cp -r %cur_dir% %OSS_UPLOAD_PATH%/%today%/
goto:eof


:set_range

set gfx_start=0x00000001
set gfx_end=0xffffffff

set comp_start=0x00000001
set comp_end=0xffffffff


set frame_start=0x00000000
set frame_end=0xffffffff

shift
set flag=%1
shift

:set_range_start
if "%1" == "" goto end
if "%1" == "gfx_start" (
    set gfx_start=%2
) else if "%1" == "gfx_end" (
    set gfx_end=%2
) else if "%1" == "comp_start" (
    set comp_start=%2
) else if "%1" == "comp_end" (
    set comp_end=%2
) else if "%1" == "frame_start" (
    set frame_start=%2
) else if "%1" == "frame_end" (
    set frame_end=%2
)
shift
shift
goto set_range_start
:end

if "%flag%" == "cb" (

reg delete  %REGEDIT_PATH% /v PerfCounterStartFrameId  /f
reg delete  %REGEDIT_PATH% /v PerfCounterEndFrameId  /f

reg add  %REGEDIT_PATH% /v PerfCounterStartComputeSubmissionNum  /d %comp_start% /f
reg add  %REGEDIT_PATH% /v PerfCounterStopComputeSubmissionNum   /d %comp_end% /f
reg add  %REGEDIT_PATH% /v PerfCounterStartGfxSubmissionNum  /d %gfx_start% /f
reg add  %REGEDIT_PATH% /v PerfCounterStopGfxSubmissionNum   /d %gfx_end% /f


reg query  %REGEDIT_PATH% /v PerfCounterStartComputeSubmissionNum
reg query  %REGEDIT_PATH% /v PerfCounterStartGfxSubmissionNum
reg query  %REGEDIT_PATH% /v PerfCounterStopComputeSubmissionNum
reg query  %REGEDIT_PATH% /v PerfCounterStopGfxSubmissionNum


    
) else if "%flag%" == "frame" (

reg delete %REGEDIT_PATH% /v PerfCounterStartComputeSubmissionNum  /f
reg delete %REGEDIT_PATH% /v PerfCounterStopComputeSubmissionNum  /f
reg delete %REGEDIT_PATH% /v PerfCounterStartGfxSubmissionNum  /f
reg delete %REGEDIT_PATH% /v PerfCounterStopGfxSubmissionNum  /f


reg add  %REGEDIT_PATH% /v PerfCounterStartFrameId   /d %frame_start% /f    
reg add  %REGEDIT_PATH% /v PerfCounterEndFrameId  /d %frame_end% /f


reg query  %REGEDIT_PATH% /v PerfCounterStartFrameId
reg query  %REGEDIT_PATH% /v PerfCounterEndFrameId
)

pause
goto:eof
