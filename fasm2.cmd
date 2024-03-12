@echo off
setlocal
set include=%~dp0include;%include%
"%~dp0fasmg" -iInclude('fasm2.inc') %*
endlocal