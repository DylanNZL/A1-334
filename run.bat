cls
@echo off

@echo Clean folder before making?
@echo Y/N?

set /p clean=
@echo %clean%

if %clean% == y (
  @echo del *.o
  del *.o
  @echo del *.exe
  del *.exe
) else if %clean% == Y (
  @echo del *.o
  del *.o
  @echo del *.exe
  del *.exe
)
@echo make
make
@echo server.exe 1155
server.exe 1155
pause