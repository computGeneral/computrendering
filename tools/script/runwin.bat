mc alias set oss https://oss.mthreads.com arch-perf arch-perf123
@rem pause

set DRIVER=wddm_cc1dcd798_hw
set TRACE_PATH=D:\TOOLS\Apitrace_x64_Release_version5.0\bin
set REGEDIT_PATH=HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}\0001\UMD

for /f "tokens=1-3 delims=-/ " %%1 in ("%date%") do set ymd=%%1%%2%%3

set workdir=%~dp0\_%DRIVER%_%ymd%_
mkdir %workdir%
cd %workdir%

echo %ymd% %time%  > run.time
pause

for %%a in (0 1 2 3 4 5 6) do ( 
@rem for %%a in (1) do ( 
call:run "D:\TRACE\d3dtest_784tri.trace" AppName="d3dtest.exe" DumpFolder=%workdir% StartFrame=0x00000006 Group=%%a DavyGroup=%%a Interval=0x00000064
)
echo %ymd% %time% all run task finished >> run.time
@rem mc cp -r run.time oss/arch-perf/logging/%workdir%/
@rem mc cp -r panelSettings.txt oss/arch-perf/logging/%workdir%/
cd ..
pause
goto:eof

@rem =================================================
:run

set name=%~n1
mkdir %name%

call:panelConf %*

cd %name%
mkdir pipeline_dumps
mkdir perfLog
echo %ymd% %time% %name% start  >> ..\run.time
@rem %TRACE_PATH%\d3dretrace.exe -v --snapshot=frame %1
%TRACE_PATH%\d3dretrace.exe  %1
cd ..
@rem mc cp -r %name% oss/arch-perf/logging/%workdir%/
goto:eof

@rem =================================================
:panelConf
set subFolder=%~n1

set AppName="test.exe"
set DumpFolder=%~dp1
set CoreMask=0x000000ff
set StartFrame=0x00000001
set FrameCount=0x00000001
set Group=0x00000000
set DavyGroup=0x00000000
set Interval=0x00000014

shift
echo %1
echo %2
:userCfgLoop
if "%1" == "" goto userCfgEnd
if "%1" == "AppName" (
    set AppName=%2    
) else if "%1" == "DumpFolder" (
    set DumpFolder=%2\%subFolder%
) else if "%1" == "CoreMask" (
    set CoreMask=%2
) else if "%1" == "StartFrame" (
    set StartFrame=%2
) else if "%1" == "FrameCount" (
    set FrameCount=%2
) else if "%1" == "Group" (
    set Group=%2
) else if "%1" == "DavyGroup" (
    set DavyGroup=%2
) else if "%1" == "Interval" (
    set Interval=%2
)
shift
shift
goto:userCfgLoop
:userCfgEnd
@rem General Settings for per-draw analysis
reg delete %REGEDIT_PATH% /v WaitAfterSubmit /f
reg add    %REGEDIT_PATH% /v WaitAfterSubmit /d 0x00000001 /f
reg query  %REGEDIT_PATH% /v WaitAfterSubmit

reg delete %REGEDIT_PATH% /v SubmitOnDispatchCount /f
reg add    %REGEDIT_PATH% /v SubmitOnDispatchCount /d 0x00000001 /f  
reg query  %REGEDIT_PATH% /v SubmitOnDispatchCount

reg delete %REGEDIT_PATH% /v SubmitOnDrawCount /f
reg add    %REGEDIT_PATH% /v SubmitOnDrawCount /d 0x00000001 /f  
reg query  %REGEDIT_PATH% /v SubmitOnDrawCount

reg delete %REGEDIT_PATH% /v SubmitOnTransferCount /f
reg add    %REGEDIT_PATH% /v SubmitOnTransferCount /d 0x00000001 /f  
reg query  %REGEDIT_PATH% /v SubmitOnTransferCount

@rem pfm Settings for each run call
reg delete %REGEDIT_PATH% /v PfmDumpAppName /f
reg delete %REGEDIT_PATH% /v PfmDumpFolder /f
reg delete %REGEDIT_PATH% /v PfmDumpCoreMask /f
reg delete %REGEDIT_PATH% /v PfmDumpStartFrame /f
reg delete %REGEDIT_PATH% /v PfmDumpFrameCount /f
reg delete %REGEDIT_PATH% /v PfmDumpGroup /f
reg delete %REGEDIT_PATH% /v PfmDumpDavyGroup /f
reg delete %REGEDIT_PATH% /v PfmDumpInterval /f

reg add    %REGEDIT_PATH% /v PfmDumpAppName  /d %AppName% /f  
reg add    %REGEDIT_PATH% /v PfmDumpFolder  /d %DumpFolder% /f  
reg add    %REGEDIT_PATH% /v PfmDumpCoreMask /d %CoreMask% /f  
reg add    %REGEDIT_PATH% /v PfmDumpStartFrame  /d %StartFrame% /f  
reg add    %REGEDIT_PATH% /v PfmDumpFrameCount  /d %FrameCount% /f  
reg add    %REGEDIT_PATH% /v PfmDumpGroup  /d %Group% /f  
reg add    %REGEDIT_PATH% /v PfmDumpDavyGroup  /d %DavyGroup% /f  
reg add    %REGEDIT_PATH% /v PfmDumpInterval  /d %Interval% /f  
  
reg query  %REGEDIT_PATH% /v PfmDumpAppName
reg query  %REGEDIT_PATH% /v PfmDumpFolder
reg query  %REGEDIT_PATH% /v PfmDumpCoreMask
reg query  %REGEDIT_PATH% /v PfmDumpStartFrame
reg query  %REGEDIT_PATH% /v PfmDumpFrameCount
reg query  %REGEDIT_PATH% /v PfmDumpGroup 
reg query  %REGEDIT_PATH% /v PfmDumpDavyGroup 
reg query  %REGEDIT_PATH% /v PfmDumpInterval  

@rem compiler Dump enable
reg delete %REGEDIT_PATH% /v EnableCompilerDump /f
reg add    %REGEDIT_PATH% /v EnableCompilerDump /d TRUE /f
reg query  %REGEDIT_PATH% /v EnableCompilerDump

reg delete %REGEDIT_PATH% /v EnableDiskShaderCache /f
reg add    %REGEDIT_PATH% /v EnableDiskShaderCache /d FALSE /f
reg query  %REGEDIT_PATH% /v EnableDiskShaderCache

@rem Max gfx multiple kick Count (default 0xff) 
@rem reg delete %REGEDIT_PATH% /v MaxMultiKickCount /f
@rem reg add    %REGEDIT_PATH% /v MaxMultiKickCount /d 0x00000001 /f
@rem reg query  %REGEDIT_PATH% /v MaxMultiKickCount


goto:eof

pause
