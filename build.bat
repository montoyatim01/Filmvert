@echo off
setlocal EnableExtensions

set "BASH_EXE=C:\Program Files\Git\bin\bash.exe"
if not exist "%BASH_EXE%" set "BASH_EXE=C:\Program Files (x86)\Git\bin\bash.exe"
if not exist "%BASH_EXE%" (
  echo Git Bash not found. Expected "%BASH_EXE%".
  exit /b 1
)

REM Run build via Git Bash, forwarding any args to build.sh.
conan profile detect --name msvc --force
for /f "delims=" %%p in ('conan profile path msvc') do set "MSVC_PROFILE=%%p"
if not exist "%MSVC_PROFILE%" (
  echo Conan MSVC profile not found at "%MSVC_PROFILE%".
  exit /b 1
)
findstr /C:"compiler=msvc" "%MSVC_PROFILE%" >nul
if errorlevel 1 (
  echo Conan MSVC profile does not appear to target MSVC.
  exit /b 1
)
powershell -NoProfile -Command "(Get-Content \"%MSVC_PROFILE%\") -replace 'compiler.cppstd=14','compiler.cppstd=17' | Set-Content \"%MSVC_PROFILE%\""
copy /Y "%MSVC_PROFILE%" "%USERPROFILE%\.conan2\profiles\default" >nul
"%BASH_EXE%" -lc "./build.sh %*"
exit /b %ERRORLEVEL%
