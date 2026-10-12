@echo off
setlocal
if not defined IDF_PATH set "IDF_PATH=%USERPROFILE%\.codex\tools\esp-idf-v5.5.4"
call "%IDF_PATH%\export.bat"
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0..\firmware\watch"
python "%IDF_PATH%\tools\idf.py" %*
