@echo off
call .\venv\Scripts\activate.bat
rc2po -i ..\langs\en-US.rc -o en-US.pot -S -P --charset=utf-16 -l LANG_ENGLISH --sublang=SUBLANG_ENGLISH_US
pause
