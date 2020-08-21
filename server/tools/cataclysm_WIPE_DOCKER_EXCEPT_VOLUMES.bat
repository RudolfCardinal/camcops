@echo off
REM server/tools/cataclysm_WIPE_DOCKER_EXCEPT_VOLUMES.bat
REM
REM See Linux equivalent.

setlocal


REM ===========================================================================
REM Confirmation
REM ===========================================================================

choice /C YN /M "Wipe Docker data (images, containers, networks) EXCEPT volumes"
if ERRORLEVEL 2 exit /b 1


REM ===========================================================================
REM Destroy everything except volumes
REM ===========================================================================

echo - Stopping all Docker containers...
for /f "tokens=*" %%i in ('docker ps -aq') DO docker stop %%i
echo - Deleting all Docker containers...
for /f "tokens=*" %%i in ('docker ps -aq') DO docker rm %%i
echo - Pruning all networks...
docker network prune -f
echo - Deleting all images...
for /f "tokens=*" %%i in ('docker images -qa') DO docker rmi -f %%i
echo - Done.
