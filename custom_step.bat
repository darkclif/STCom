echo off
set PROJECT_DIR=%1
set FLATBUF_EXE=%PROJECT_DIR%\libs\flatbuffer\bin\flatc.exe

if not exist %FLATBUF_EXE% (
    echo Flatbuffer executable doesn not exist inside expected folder.
    echo Path: %FLATBUF_EXE%
    exit /B 1
)

pushd %PROJECT_DIR%\src\schemas
%FLATBUF_EXE% --cpp chatpacket.fbs 2>&1

if %errorlevel% NEQ 0 (
    echo Flatbuffer schema compilation returned with non-zero exit code.
    exit /B 2
) else (
    echo Flatbuffer schema compiled successfuly.
)
popd
