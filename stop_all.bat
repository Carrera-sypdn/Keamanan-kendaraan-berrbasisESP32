@echo off
:: ============================================================
:: stop_all.bat
:: 1. Menghentikan Apache & MySQL (XAMPP)
:: 2. Menutup jendela CMD "SSH Tunnel" beserta proses ssh-nya
:: ============================================================

:: --- Cek apakah script sudah berjalan sebagai Administrator ---
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Script belum berjalan sebagai Administrator.
    echo Meminta izin elevasi...
    powershell -NoProfile -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

:: --- Sesuaikan path instalasi XAMPP jika berbeda ---
set "XAMPP_DIR=C:\xampp"

echo.
echo === Menghentikan Apache ===
if exist "%XAMPP_DIR%\apache_stop.bat" (
    call "%XAMPP_DIR%\apache_stop.bat"
) else (
    echo [PERINGATAN] Tidak ditemukan %XAMPP_DIR%\apache_stop.bat
    echo Mencoba menghentikan proses httpd.exe secara langsung...
    taskkill /F /IM httpd.exe >nul 2>&1
)

echo === Menghentikan MySQL ===
if exist "%XAMPP_DIR%\mysql_stop.bat" (
    call "%XAMPP_DIR%\mysql_stop.bat"
) else (
    echo [PERINGATAN] Tidak ditemukan %XAMPP_DIR%\mysql_stop.bat
    echo Mencoba menghentikan proses mysqld.exe secara langsung...
    taskkill /F /IM mysqld.exe >nul 2>&1
)

echo.
echo === Menutup SSH Tunnel ===
:: Tutup jendela CMD berjudul "SSH Tunnel" (yang dibuka start_all.bat)
taskkill /F /FI "WINDOWTITLE eq SSH Tunnel*" >nul 2>&1

:: Jaga-jaga kalau ada proses ssh.exe yang masih menempel ke localhost.run
:: (hati-hati: ini akan mematikan SEMUA proses ssh.exe yang sedang berjalan)
for /f "tokens=2" %%P in ('tasklist /FI "IMAGENAME eq ssh.exe" /FO CSV /NH 2^>nul') do (
    taskkill /F /PID %%~P >nul 2>&1
)

echo.
echo === Membersihkan sisa proses cmd minimized (Apache/MySQL launcher) ===
taskkill /F /FI "WINDOWTITLE eq Apache*" >nul 2>&1
taskkill /F /FI "WINDOWTITLE eq MySQL*" >nul 2>&1

echo.
echo Selesai. Apache, MySQL, dan SSH tunnel sudah dihentikan.
pause
