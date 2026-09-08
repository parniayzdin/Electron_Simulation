@echo off
wsl -d Ubuntu --cd "%~dp0." -- bash -lc "cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel 4 && ./build/Electron_Simulation"
if errorlevel 1 (
  echo.
  echo Could not start. Check the Ubuntu / WSL dependencies listed in README.md.
  pause
)
