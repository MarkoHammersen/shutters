Rem modify PATH in order to find make, gcc, g++ and gcovr
PATH = C:\msys64\usr\bin;C:\msys64\mingw64\bin;c:\users\mhammer3\.platformio\penv\scripts;%PATH%

Rem build executable for Windows
make all

Rem run executable 
tests.exe

Rem generate code coverage
gcovr -r .. --html --html-details -o coverage.html

pause
