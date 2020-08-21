@echo off
REM server/docker/windows/upgrade_db.bat
REM
REM Launches the "camcops_server upgrade_db" command within a new Docker
REM container running the CamCOPS server image.

setlocal

set THIS_DIR=%~dp0
set CAMCOPS_SERVER=%THIS_DIR%\camcops_server.bat
set DEFAULT_CONFIG_FILE=camcops.conf

if "%CAMCOPS_DOCKER_CONFIG_HOST_DIR%" == "" (
    echo "Must set CAMCOPS_DOCKER_CONFIG_HOST_DIR environment variable!"
    exit /b 1
)
if "%CAMCOPS_DOCKER_CONFIG_FILENAME%" == "" (
    REM Set to default (as per .env file).
    REM Don't set DEFAULT_CONFIG_FILE here. Obscure, but:
    REM https://stackoverflow.com/questions/9102422/windows-batch-set-inside-if-not-working
    REM ... you can also use "setlocal EnableDelayedExpansion"
    echo CAMCOPS_DOCKER_CONFIG_FILENAME not set; using default of %DEFAULT_CONFIG_FILE%
    set CAMCOPS_DOCKER_CONFIG_FILENAME=%DEFAULT_CONFIG_FILE%
)

set CFG_ON_HOST=%CAMCOPS_DOCKER_CONFIG_HOST_DIR%/%CAMCOPS_DOCKER_CONFIG_FILENAME%
REM ... CAMCOPS_DOCKER_CONFIG_HOST_DIR will use /host_mnt/c/... notation
set CFG_ON_DOCKER=/camcops/cfg/%CAMCOPS_DOCKER_CONFIG_FILENAME%

echo Upgrading CamCOPS database to current version.
echo - Config file on host: %CFG_ON_HOST%
echo - Config file as seen by Docker: %CFG_ON_DOCKER%

"%CAMCOPS_SERVER%" upgrade_db --config "%CFG_ON_DOCKER%"
