for /f "tokens=*" %%F in ('dir /b /a:-d "%GITHUB_WORKSPACE%\dist\*.whl"') do call set WHEEL_FN=%%F
%PYTHON% -c "import sys; print(sys.executable, end='')"
REM conda info -- running conda info here breaks the shell commands somehow
%PYTHON% -m pip install --no-deps --ignore-installed %GITHUB_WORKSPACE%\dist\%WHEEL_FN%
