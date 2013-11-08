pushd deploy
set majorver=1.3.1
set minorver=0
set binbeta=stable
set beta=Stable
set exe=..\Release\pbldump.exe
set ver=v%majorver%.%minorver%
set binarc=%ver%\pbldump-%majorver%%binbeta%.zip
set srcarc=%ver%\pbldump.src.zip
set sitefile=%ver%\%binbeta%
if not exist %ver% mkdir %ver%
if exist  %ver%\*.* del %ver%\*.* /q
pkzip25 -add -attr=all -nozip  %binarc% @bin.txt > nul
pkzip25 -add -attr=all -nozip -path %srcarc% @src.txt > nul

popd
