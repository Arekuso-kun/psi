@echo off
cd /d "%~dp0.."
python PC\controller.py --demo
if errorlevel 1 pause
