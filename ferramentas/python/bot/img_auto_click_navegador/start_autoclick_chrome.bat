@echo off
REM ===== Config =====
set "CHROME_PATH=C:\Users\walli\AppData\Local\Google\Chrome SxS\Application\chrome.exe"
set "DEBUG_PORT=9222"
REM Use um perfil DEDICADO para automação (não use o seu 'User Data' normal)
set "CHROME_USER_DATA=%~dp0chrome_automation_profile"

REM ===== Verificações =====
REM Garante pasta do perfil
if not exist "%CHROME_USER_DATA%" (
  mkdir "%CHROME_USER_DATA%"
)

REM Verifica se a porta DevTools já está em uso
for /f "tokens=1,2,3,4,5" %%a in ('netstat -ano ^| findstr /R /C:":%DEBUG_PORT% .*LISTENING"') do (
  echo [WARN] Porta %DEBUG_PORT% já em uso pelo PID %%e.
  echo Feche a outra instância ou mude DEBUG_PORT.
  pause
  exit /b 1
)

REM Não bloqueie só porque existe outro chrome.exe do usuário:
REM a gente quer isolar por PERFIL e PORTA, então não precisa impedir se houver outro Chrome rodando.
REM (Se ainda quiser bloquear, mantenha o bloco abaixo descomentado.)
REM tasklist /FI "IMAGENAME eq chrome.exe" | find /I "chrome.exe" > nul
REM if %errorlevel%==0 (
REM   echo [WARN] Ja existe um Chrome rodando. OK, vamos abrir em perfil isolado mesmo assim.
REM )

REM ===== Inicializa Chrome de automação =====
start "" "%CHROME_PATH%" ^
  --remote-debugging-port=%DEBUG_PORT% ^
  --remote-allow-origins=* ^
  --user-data-dir="%CHROME_USER_DATA%" ^
  --profile-directory="Default" ^
  --new-window ^
  --no-first-run ^
  --no-default-browser-check ^
  --disable-features=CalculateNativeWinOcclusion ^
  --disable-backgrounding-occluded-windows ^
  --disable-renderer-backgrounding ^
  --disable-background-timer-throttling

REM ===== Captura o PID que está escutando a porta (para passar ao Python) =====
REM Dá um tempinho para o Chrome subir e abrir a porta
timeout /t 2 > nul

set "PID_DEVTOOLS="
for /f "tokens=5" %%p in ('netstat -ano ^| findstr /R /C:":%DEBUG_PORT% .*LISTENING"') do (
  set PID_DEVTOOLS=%%p
)

if "%PID_DEVTOOLS%"=="" (
  echo [ERROR] Nao consegui descobrir o PID via netstat. Tente aumentar o timeout.
  pause
  exit /b 2
)

echo [INFO] Chrome de automacao PID: %PID_DEVTOOLS%
echo Agora voce pode rodar:
echo   python autoclick_browser.py --pid %PID_DEVTOOLS%
