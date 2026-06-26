@echo off
g++ main.cpp -o NetworkSimulator.exe -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
echo.
echo Compilation Done!
pause