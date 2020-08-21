@echo off
REM server/tools/armageddon_WIPE_EVERYTHING_DOCKER.bat
REM
REM See Linux equivalent.

setlocal


REM ===========================================================================
REM Confirmation
REM ===========================================================================

choice /C YN /M "Wipe EVERY aspect of Docker data on this computer"
REM https://www.computerhope.com/choicehl.htm
REM - ERRORLEVEL is set to 1 for the first choice, 2 for the second, etc.
REM - "IF ERRORLEVEL" checks "greater than or equal to"
if ERRORLEVEL 2 exit /b 1
choice /C YN /M "This includes data volumes. Are you sure"
if ERRORLEVEL 2 exit /b 1
choice /C YN /M "Last chance -- you may lose databases. Really sure"
if ERRORLEVEL 2 exit /b 1


REM ===========================================================================
REM Destroy everything
REM ===========================================================================
REM https://stackoverflow.com/questions/48813286/stop-all-docker-containers-at-once-on-windows

echo - Proceeding to Docker armageddon.
echo - Stopping all Docker containers...
for /f "tokens=*" %%i in ('docker ps -aq') DO docker stop %%i
echo - Deleting all Docker containers...
for /f "tokens=*" %%i in ('docker ps -aq') DO docker rm %%i
echo - Pruning all networks...
docker network prune -f
echo - Deleting all dangling images...
for /f "tokens=*" %%i in ('docker images --filter "dangling=true" -qa') DO docker rmi -f %%i
echo - Deleting all volumes...
for /f "tokens=*" %%i in ('docker volume ls --filter "dangling=true" -q') DO docker volume rm %%i
echo - Deleting all images...
for /f "tokens=*" %%i in ('docker images -qa') DO docker rmi -f %%i
echo - Done.
