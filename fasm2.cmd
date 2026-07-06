@echo off
setlocal
set include=%~dp0include;%include%
"%~dp0fasmg" -e100 -iInclude('fasm2.inc') %*
endlocal