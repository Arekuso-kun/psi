@echo off
cd /d "%~dp0.."
python PC\controller.py
if errorlevel 1 pause
