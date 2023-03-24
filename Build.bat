@echo off
xmake config -p windows -a x64 -m %1
xmake build
@echo on