@echo off
set DIR=%~dp0
"%DIR%\java" --module-path "%DIR%..\mods" -Dheapstats.boot.mode=jlink -m heapstats.fx/jp.co.ntt.oss.heapstats.fx.HeapStatsFXAnalyzer %*
