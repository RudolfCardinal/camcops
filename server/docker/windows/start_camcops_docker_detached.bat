@echo off
REM server/docker/windows/start_camcops_docker_detached.bat
REM
REM Launches the Docker Compose application in detached (daemon) mode.

set THIS_DIR=%~dp0
set DOCKER_COMPOSE_DIR=%THIS_DIR%\..\dockerfiles

REM We must change directory to pick up ".env" etc.

cd "%DOCKER_COMPOSE_DIR%"

REM Having done so, the default Docker Compose filenames include
REM docker-compose.yaml, so we don't need to specify that.

docker-compose up -d
