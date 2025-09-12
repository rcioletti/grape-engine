@echo off
setlocal enabledelayedexpansion

echo Compiling shaders...

set SHADER_DIR=..\resources\shaders
set COMPILED_COUNT=0
set SKIPPED_COUNT=0

:: Create output directory if it doesn't exist
if not exist "%SHADER_DIR%" (
    echo Error: Shader directory %SHADER_DIR% not found!
    exit /b 1
)

:: Function to check if shader needs recompilation
for %%f in ("%SHADER_DIR%\*.vert" "%SHADER_DIR%\*.frag" "%SHADER_DIR%\*.comp" "%SHADER_DIR%\*.geom") do (
    if exist "%%f" (
        set "SOURCE_FILE=%%f"
        set "OUTPUT_FILE=%%f.spv"
        
        :: Check if output exists and is newer than source
        set NEEDS_COMPILE=1
        if exist "!OUTPUT_FILE!" (
            for %%A in ("!SOURCE_FILE!") do set SOURCE_TIME=%%~tA
            for %%B in ("!OUTPUT_FILE!") do set OUTPUT_TIME=%%~tB
            
            :: Simple time comparison (this is basic, but works for most cases)
            if "!OUTPUT_TIME!" GEQ "!SOURCE_TIME!" (
                set NEEDS_COMPILE=0
            )
        )
        
        if !NEEDS_COMPILE!==1 (
            echo Compiling: %%~nxf
            glslc "%%f" -o "%%f.spv"
            if !ERRORLEVEL! neq 0 (
                echo Error compiling %%~nxf
                exit /b 1
            )
            set /a COMPILED_COUNT+=1
        ) else (
            echo Skipping: %%~nxf ^(up to date^)
            set /a SKIPPED_COUNT+=1
        )
    )
)

echo.
echo Shader compilation complete!
echo Compiled: %COMPILED_COUNT% shaders
echo Skipped: %SKIPPED_COUNT% shaders
echo.