import time
import mss
import numpy as np
import pyautogui
import subprocess
import win32gui
import win32con
import os
from datetime import datetime

# === CONFIGURAÇÕES ===
TITULO_JANELA = "Ragnarok"
AHK_EXE = r"C:\Program Files\AutoHotkey\v2\AutoHotkey.exe"
SCRIPT_CURAR = "./AutoHotkey/curar.ahk"
SCRIPT_MOVER = "./AutoHotkey/mover_mouse.ahk"
SCRIPT_CLICAR = "./AutoHotkey/clicar.ahk"

# Coordenadas da barra de vida (ajuste conforme a HUD do seu jogo)
vida_x = 281
vida_y = 423
largura_barra = 64
altura_barra = 3

# Coordenada para clicar após cura
click_x = 155
click_y = 399
tolerancia = 5  # margem de erro para verificar posição do mouse

# === Funções auxiliares ===
def log(mensagem):
    print(f"[{datetime.now().strftime('%H:%M:%S')}] {mensagem}")

def calcular_percentual_vida():
    with mss.mss() as sct:
        monitor = {"top": vida_y, "left": vida_x, "width": largura_barra, "height": altura_barra}
        img = np.array(sct.grab(monitor))
        pixels = img[:, :, :3].reshape(-1, 3)
        verdes = np.sum((pixels[:, 1] > 100) & (pixels[:, 0] < 100) & (pixels[:, 2] < 100))
        total = pixels.shape[0]
        return (verdes / total) * 100

def mouse_esta_na_posicao():
    atual = pyautogui.position()
    return abs(atual.x - click_x) <= tolerancia and abs(atual.y - click_y) <= tolerancia

def executar_ahk(script):
    if not os.path.exists(script):
        log(f"[✘] Script não encontrado: {script}")
        return
    subprocess.call([AHK_EXE, script])

def encontrar_janela(titulo_parcial="Ragnarok"):
    def callback(hwnd, lista):
        if win32gui.IsWindowVisible(hwnd):
            titulo = win32gui.GetWindowText(hwnd)
            if titulo_parcial.lower() in titulo.lower():
                lista.append(hwnd)
    janelas = []
    win32gui.EnumWindows(callback, janelas)
    return janelas[0] if janelas else None

def ativar_janela(hwnd):
    try:
        win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
        win32gui.SetForegroundWindow(hwnd)
        return True
    except Exception as e:
        log(f"[✘] Erro ao ativar janela: {e}")
        return False

# === Início ===
if not os.path.exists(AHK_EXE):
    log(f"[✘] AutoHotkey.exe não encontrado em: {AHK_EXE}")
    exit()

log("🎮 Monitorando barra de vida... Pressione Ctrl+C para sair.")

try:
    while True:
        hwnd = encontrar_janela(TITULO_JANELA)
        if hwnd:
            ativar_janela(hwnd)
            vida = calcular_percentual_vida()
            log(f"❤️ Vida atual: {vida:.1f}%")

            if vida < 70:
                log("⚠️ Vida baixa! Iniciando sequência de cura...")

                executar_ahk(SCRIPT_CURAR)
                time.sleep(0.3)

                executar_ahk(SCRIPT_MOVER)
                time.sleep(0.5)

                timeout = 2
                while timeout > 0:
                    if True:
                        log("🖱 Mouse na posição. Clicando...")
                        executar_ahk(SCRIPT_CLICAR)
                        break
                    time.sleep(0.1)
                    timeout -= 0.1

                log("⏳ Esperando antes da próxima verificação...\n")
                time.sleep(2)

            time.sleep(1)
        else:
            log("[✘] Janela do jogo não encontrada.")
            time.sleep(3)

except KeyboardInterrupt:
    log("⏹ Monitoramento encerrado.")
except Exception as e:
    log(f"[✘] Erro inesperado: {e}")
