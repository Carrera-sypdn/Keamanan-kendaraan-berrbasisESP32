@echo off
:: ============================================================
:: start_all.bat
:: 1. Menjalankan Apache & MySQL (XAMPP)
:: 2. Membuka CMD baru sebagai Administrator untuk SSH tunnel
::    ke localhost.run (ssh -R 80:localhost:80 nokey@localhost.run)
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
echo === Menjalankan Apache ===
if exist "%XAMPP_DIR%\apache_start.bat" (
    start "Apache" /min cmd /c ""%XAMPP_DIR%\apache_start.bat""
) else (
    echo [PERINGATAN] Tidak ditemukan %XAMPP_DIR%\apache_start.bat
)

echo === Menjalankan MySQL ===
if exist "%XAMPP_DIR%\mysql_start.bat" (
    start "MySQL" /min cmd /c ""%XAMPP_DIR%\mysql_start.bat""
) else (
    echo [PERINGATAN] Tidak ditemukan %XAMPP_DIR%\mysql_start.bat
)

:: --- Beri jeda agar Apache & MySQL sempat naik ---
timeout /t 4 /nobreak >nul

:: --- Buka CMD baru untuk SSH tunnel (mewarisi hak admin dari script ini) ---
echo.
echo === Membuka SSH tunnel ke localhost.run ===
start "SSH Tunnel" cmd /k ssh -R 80:localhost:80 nokey@localhost.run

echo.
echo Selesai. Apache dan MySQL berjalan di background,
echo jendela CMD terpisah sedang membuka tunnel SSH.
echo (Jendela ini boleh ditutup)
pause
