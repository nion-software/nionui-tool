REM Download NionUILauncher-Windows.zip and unzip to directory NionUILauncher-Windows

move NionUILauncher-Windows %SCRIPTS%\NionUILauncher
if errorlevel 1 exit 1

%PYTHON% -m pip install --no-deps --ignore-installed .
if errorlevel 1 exit 1
