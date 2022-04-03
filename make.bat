@echo off
echo Run this with MSVC developer command prompt, or it may not work.
echo If you get cl.exe not found or library path not found, above is your problem.

cl *.c /link /out:revsocks.exe