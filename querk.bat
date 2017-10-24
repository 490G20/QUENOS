@echo off
cx6812 -vl crts.s qmain.c qinit.c qcore.c qrequest.c queue.c quser.c
if errorlevel 1 goto bad
:clink
echo.
echo Linking ...
clnk -o querk.h12 querk.lnk
if errorlevel 1 goto bad
:chexa
echo.
echo Converting ...
chex -o querk.s19 querk.h12
if errorlevel 1 goto bad
:cllabs
echo.
echo Generating absolute listing ...
clabs querk.h12
if errorlevel 1 goto bad
echo.
echo.
echo        Compilation successful.
goto sortie
:bad
echo.
echo.
echo        Compilation FAILED.
:sortie
echo on
