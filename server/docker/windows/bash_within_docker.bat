@echo off
REM server/docker/windows/bash_within_docker.bat
REM
REM Starts a container with the CamCOPS server image and runs "bash" in it.

setlocal

set THIS_DIR=%~dp0
set WITHIN_DOCKER=%THIS_DIR%\within_docker.bat

"%WITHIN_DOCKER%" /bin/bash
