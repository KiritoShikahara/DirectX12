@echo off
setlocal

REM Starts Claude Code in this script's folder.
REM Example: start-claude-code.cmd --help
cd /d "%~dp0"

set "CLAUDE_EXE=C:\Users\com\.local\bin\claude.exe"

if not exist "%CLAUDE_EXE%" (
  echo Claude Code was not found:
  echo %CLAUDE_EXE%
  pause
  exit /b 1
)

"%CLAUDE_EXE%" --dangerously-skip-permissions %*
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
  echo.
  echo Claude Code exited with code %EXIT_CODE%.
  pause
)

exit /b %EXIT_CODE%
