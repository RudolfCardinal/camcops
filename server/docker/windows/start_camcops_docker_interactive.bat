@echo off
REM server/docker/windows/start_camcops_docker_interactive.bat
REM
REM Launches the Docker Compose application in interactive mode.

set THIS_DIR=%~dp0
set DOCKER_COMPOSE_DIR=%THIS_DIR%\..\dockerfiles

REM We must change directory to pick up ".env" etc.

cd "%DOCKER_COMPOSE_DIR%"

REM Having done so, the default Docker Compose filenames include
REM docker-compose.yaml, so we don't need to specify that.

REM In interactive mode, it's also helpful if the containers abort on failure,
REM rather than restarting (though restarting needs to be the default to cope
REM with reboots in unattended mode). The "--abort-on-container-exit" option
REM does this.

docker-compose up --abort-on-container-exit
