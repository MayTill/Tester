@echo off
windres ICONS.rc -O coff -o ICONS_x64.res
windres --target=pe-i386 ICONS.rc -O coff -o ICONS_x86.res 
g++ TESTER.cpp ICONS_x64.res -o TESTER_x64 -std=c++17
g++ TESTER.cpp ICONS_x86.res -m32 -o TESTER_x86 -std=c++17
