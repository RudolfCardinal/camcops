@echo off
REM server/docker/windows/camcops_server.bat
REM
REM Launches the "camcops_server" command within one of the Docker containers
REM running the CamCOPS server image.

setlocal

set THIS_DIR=%~dp0
set WITHIN_DOCKER=%THIS_DIR%\within_docker.bat

"${WITHIN_DOCKER}" /camcops/venv/bin/camcops_server %*
REM ^              ^                                ^
REM |              |                                |
REM |              |                                |
REM |              |                                +- user arguments
REM |              +- the camcops_server command
REM +- execute within our Docker container...
