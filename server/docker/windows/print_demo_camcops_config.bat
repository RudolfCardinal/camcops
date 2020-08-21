@echo off
REM server/docker/windows/print_demo_camcops_config.bat
REM
REM Prints a demo CamCOPS config file.
REM This is slightly tricky because we want to get rid of some stderr (which
REM otherwise blends into stdout once passed through Docker).

setlocal

set THIS_DIR=%~dp0
set CAMCOPS_SERVER=%THIS_DIR%\camcops_server.bat

"%CAMCOPS_SERVER%" --no_log demo_camcops_config --docker
