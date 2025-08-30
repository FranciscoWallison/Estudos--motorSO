# bot_canvas.py
# Fast Unity WebGL canvas matcher/clicker via Chrome DevTools Protocol (CDP)
# Now with continuous watch mode (--watch) to keep scanning until you stop (Ctrl+C).
#
# Requisitos:
#   pip install playwright opencv-python-headless numpy psutil
#   playwright install
#
# Exemplo:
#   chrome.exe --remote-debugging-port=9222
#   python bot_canvas.py --pid 52016 --img-root "C:\\PROJETOS\\PASTA\\img" --threshold 0.92 --cooldown 0.10 --watch
#
import argparse, time, os, ctypes, sys, math
from collections import deque
from ctypes import wintypes
import numpy as np
import cv2
import psutil
from playwright.sync_api import sync_playwright

# ---------------- CLI ----------------
def parse_args():
    p = argparse.ArgumentParser(description="Canvas matcher + clicker para Unity WebGL via CDP (com PID do Windows).")
    p.add_argument("--pid", type=int, required=True, help="PID informado do Chrome (do Gerenciador de Tarefas).")
    p.add_argument("--port", type=int, default=9222, help="Porta do DevTools (default: 9222).")
    p.add_argument("--img-root", default=r"C:\PROJETOS\PASTA\img", help="Pasta raiz contendo subpastas 1..5 com templates.")
    p.add_argument("--threshold", type=float, default=0.90, help="Acurácia mínima para clicar (default: 0.90).")
    p.add_argument("--max-iters", type=int, default=100, help="Máximo de iterações de clique (quando --watch NÃO está ligado).")
    p.add_argument("--cooldown", type=float, default=0.15, help="Pausa entre cliques, em segundos (default: 0.15s).")
    p.add_argument("--selector", default="#unity-canvas", help="CSS selector do canvas (default: #unity-canvas).")
    p.add_argument("--once", action="store_true", help="Executa apenas uma varredura (sem loop). (Ignorado se --watch)")
    # Continuous scanning
    p.add_argument("--watch", action="store_true", help="Mantém procurando infinitamente até você parar (Ctrl+C).")
    p.add_argument("--idle", type=float, default=0.08, help="Pausa (s) quando NÃO há match >= threshold (default: 0.08s).")
    # Repeat/suppression
    p.add_argument("--repeat", action="store_true", help="Permite clicar repetidamente o MESMO alvo enquanto existir (desabilita supressão).")
    p.add_argument("--suppress-ttl", type=float, default=0.6, help="Tempo (s) para suprimir reclick no mesmo alvo (default: 0.6s).")
    p.add_argument("--suppress-radius", type=float, default=20.0, help="Raio (device px) para considerar mesmo alvo (default: 20px).")
    p.add_argument("--strict", action="store_true", help="Se não achar janela para o PID, falha ao invés de auto-resolver.")
    return p.parse_args()

# ---------------- Win32 helpers ----------------
user32 = ctypes.windll.user32
EnumWindows = user32.EnumWindows
EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)
IsWindowVisible = user32.IsWindowVisible
GetWindowThreadProcessId = user32.GetWindowThreadProcessId
GetWindowRect = user32.GetWindowRect
GetWindowTextW = user32.GetWindowTextW
GetWindowTextLengthW = user32.GetWindowTextLengthW
GetForegroundWindow = user32.GetForegroundWindow

def _get_window_text(hwnd):
    length = GetWindowTextLengthW(hwnd)
    buf = ctypes.create_unicode_buffer(length + 1)
    GetWindowTextW(hwnd, buf, length + 1)
    return buf.value

def _rect(hwnd):
    rect = wintypes.RECT()
    if not GetWindowRect(hwnd, ctypes.byref(rect)):
        return None
    return (rect.left, rect.top, rect.right, rect.bottom)

def rect_area(r):
    l, t, rgt, bot = r
    return max(0, rgt - l) * max(0, bot - t)

def overlap_area(a, b):
    l1, t1, r1, b1 = a
    l2, t2, r2, b2 = b
    l = max(l1, l2)
    t = max(t1, t2)
    r = min(r1, r2)
    b = min(b1, b2)
    if r <= l or b <= t:
        return 0
    return (r - l) * (b - t)

def rect_from_screen(x, y, w, h):
    return (int(x), int(y), int(x + w), int(y + h))

def windows_for_pid(pid):
    """Lista de (hwnd, rect, title) para janelas visíveis do PID."""
    out = []
    def callback(hwnd, lParam):
        if not IsWindowVisible(hwnd):
            return True
        lpdwProcessId = wintypes.DWORD()
        GetWindowThreadProcessId(hwnd, ctypes.byref(lpdwProcessId))
        if lpdwProcessId.value == pid:
            r = _rect(hwnd)
            if r:
                out.append((hwnd, r, _get_window_text(hwnd)))
        return True
    EnumWindows(EnumWindowsProc(callback), 0)
    return out

def resolve_ui_pid_from_any(pid):
    """
    Chrome abre vários processos. O PID informado pode ser renderer/GPU (sem janela).
    Subimos a cadeia de pais até achar um PID com janela. Senão, tentamos qualquer chrome/msedge com janela.
    Por fim, usamos a janela do Chrome em foco.
    """
    tried = set()
    try:
        proc = psutil.Process(pid)
    except Exception:
        proc = None
    curr = proc
    while curr and curr.pid not in tried:
        tried.add(curr.pid)
        wins = windows_for_pid(curr.pid)
        if wins:
            return curr.pid, wins
        curr = curr.parent()
    for p in psutil.process_iter(['pid','name']):
        name = (p.info.get('name') or '').lower()
        if name in ('chrome.exe','msedge.exe','chrome','msedge'):
            wins = windows_for_pid(p.info['pid'])
            if wins:
                return p.info['pid'], wins
    fg = GetForegroundWindow()
    if fg:
        lpdw = wintypes.DWORD()
        GetWindowThreadProcessId(fg, ctypes.byref(lpdw))
        fpid = int(lpdw.value)
        try:
            name = psutil.Process(fpid).name().lower()
        except Exception:
            name = ""
        if 'chrome' in name or 'msedge' in name:
            wins = windows_for_pid(fpid)
            if wins:
                return fpid, wins
    return None, []

# ---------------- Templates ----------------
EXTS = (".png",".jpg",".jpeg",".bmp",".webp")

def load_templates(root):
    paths = []
    for sub in ["1","2","3","4","5"]:
        psub = os.path.join(root, sub)
        if not os.path.isdir(psub):
            continue
        for name in os.listdir(psub):
            if os.path.splitext(name)[1].lower() in EXTS:
                paths.append(os.path.join(psub, name))
    templates = []
    for p in paths:
        img = cv2.imread(p, cv2.IMREAD_COLOR)
        if img is None:
            continue
        gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        h, w = gray.shape[:2]
        templates.append((p, gray, (w, h)))
    if not templates:
        raise RuntimeError("Nenhum template encontrado em subpastas 1..5.")
    return templates

def best_match(scene_gray, templates):
    best = (None, -1.0, (0,0), (0,0))
    for path, tgray, (w, h) in templates:
        H, W = scene_gray.shape[:2]
        if H < h or W < w:
            continue
        res = cv2.matchTemplate(scene_gray, tgray, cv2.TM_CCOEFF_NORMED)
        _, maxV, _, maxL = cv2.minMaxLoc(res)
        if maxV > best[1]:
            best = (path, maxV, maxL, (w, h))
    return best  # (path, score, (x,y), (w,h))

# ---------------- Pick page by PID ----------------
def pick_page_by_pid(browser, pid):
    ui_pid, wins = resolve_ui_pid_from_any(pid)
    if not wins:
        raise RuntimeError("Não foi possível mapear o PID informado a nenhuma janela de Chrome.")
    wins = [w for w in wins if rect_area(w[1]) > 0]
    wins.sort(key=lambda x: rect_area(x[1]), reverse=True)
    target_rects = [w[1] for w in wins]

    candidate = None
    best_score = -1
    for ctx in browser.contexts:
        for pg in ctx.pages:
            try:
                info = pg.evaluate("""({ 
                    x: window.screenX, y: window.screenY, 
                    w: window.outerWidth, h: window.outerHeight, 
                    vis: document.visibilityState 
                })""")
            except Exception:
                continue
            if not info:
                continue
            page_rect = rect_from_screen(info["x"], info["y"], info["w"], info["h"])
            ov = max(overlap_area(page_rect, tr) for tr in target_rects)
            bonus = 1.5 if info.get("vis") == "visible" else 1.0
            score = ov * bonus
            if score > best_score:
                best_score = score
                candidate = pg
    if not candidate or best_score <= 0:
        # fallback: escolhe a página visível com maior área
        fallback = None
        fb_score = -1
        for ctx in browser.contexts:
            for pg in ctx.pages:
                try:
                    info = pg.evaluate("""({x: window.screenX, y: window.screenY, w: window.outerWidth, h: window.outerHeight, vis: document.visibilityState })""")
                except Exception:
                    continue
                if not info:
                    continue
                area = info["w"] * info["h"]
                area *= 1.5 if info.get("vis") == "visible" else 1.0
                if area > fb_score:
                    fb_score = area
                    fallback = pg
        candidate = fallback
    if not candidate:
        raise RuntimeError("Nenhuma page disponível no CDP.")
    return candidate

# ---------------- Main ----------------
def main():
    args = parse_args()

    # Carrega templates
    templates = load_templates(args.img_root)
    print(f"[i] {len(templates)} templates carregados de {args.img_root}")

    # Verifica PID existe
    try:
        _ = psutil.Process(args.pid).name()
    except Exception:
        if args.strict:
            raise SystemExit(f"PID {args.pid} inválido. Use um PID de um processo chrome/msedge com janela.")
        else:
            print(f"[!] PID {args.pid} não acessível; tentando auto-resolver...")

    with sync_playwright() as p:
        browser = p.chromium.connect_over_cdp(f"http://127.0.0.1:{args.port}")

        try:
            page = pick_page_by_pid(browser, args.pid)
        except Exception as e:
            if args.strict:
                raise
            print(f"[!] Aviso: {e}\n    Fallback: escolhendo maior página visível do CDP.")
            # pega a maior visível
            page = None
            best = -1
            for ctx in browser.contexts:
                for pg in ctx.pages:
                    try:
                        info = pg.evaluate("""({x: window.screenX, y: window.screenY, w: window.outerWidth, h: window.outerHeight, vis: document.visibilityState})""")
                    except Exception:
                        continue
                    if not info:
                        continue
                    area = info["w"] * info["h"]
                    area *= 1.5 if info.get("vis") == "visible" else 1.0
                    if area > best:
                        best = area
                        page = pg
            if not page:
                raise RuntimeError("Não consegui selecionar nenhuma página.")

        try:
            page.bring_to_front()
        except Exception:
            pass

        print(f"[i] Page: title={page.title()!r} | url={page.url!r}")

        # canvas e overlay
        page.wait_for_selector(args.selector, state="attached", timeout=30_000)
        canvas = page.locator(args.selector)
        page.evaluate("""
          document.getElementById('unity-loading-bar')?.style.setProperty('pointer-events','none');
          document.getElementById('unity-logo')?.style.setProperty('pointer-events','none');
        """)

        bbox = canvas.bounding_box()  # CSS px
        if not bbox:
            raise RuntimeError("Não consegui medir o #unity-canvas (sem bbox). Verifique se a aba está visível.")
        dpr = page.evaluate("window.devicePixelRatio")
        print(f"[i] Canvas bbox (CSS): {bbox} | dpr={dpr}")

        def click_css(x_css, y_css):
            page.mouse.move(x_css, y_css)
            page.mouse.click(x_css, y_css, delay=10)

        recent = deque(maxlen=64)  # (ts, cx_dev, cy_dev, path)
        def is_suppressed(cx_dev, cy_dev, path):
            if args.repeat:
                return False
            now = time.time()
            for ts, x, y, p in list(recent):
                if now - ts > args.suppress_ttl:
                    continue
                if p == path:
                    dx = cx_dev - x
                    dy = cy_dev - y
                    if (dx*dx + dy*dy) ** 0.5 <= args.suppress_radius:
                        return True
            return False

        try:
            if args.watch:
                print("[i] Watch mode ON (Ctrl+C para parar).")
                while True:
                    # screenshot do canvas em device px
                    png = canvas.screenshot(type="png")
                    img = cv2.imdecode(np.frombuffer(png, np.uint8), cv2.IMREAD_COLOR)
                    scene_gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

                    path, score, (tx, ty), (w, h) = best_match(scene_gray, templates)

                    if score >= args.threshold:
                        cx_dev = tx + w / 2.0
                        cy_dev = ty + h / 2.0
                        if not is_suppressed(cx_dev, cy_dev, path):
                            cx_css = bbox["x"] + (cx_dev / dpr)
                            cy_css = bbox["y"] + (cy_dev / dpr)
                            print(f"[click] score={score:.3f} | {path}")
                            click_css(cx_css, cy_css)
                            recent.append((time.time(), cx_dev, cy_dev, path))
                            time.sleep(args.cooldown)
                        else:
                            time.sleep(args.idle)
                    else:
                        time.sleep(args.idle)
            else:
                loops = 1 if args.once else args.max_iters
                for i in range(loops):
                    png = canvas.screenshot(type="png")
                    img = cv2.imdecode(np.frombuffer(png, np.uint8), cv2.IMREAD_COLOR)
                    scene_gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

                    path, score, (tx, ty), (w, h) = best_match(scene_gray, templates)
                    print(f"[{i}] score={score:.3f} | {path}")
                    if score < args.threshold:
                        print("[✓] Sem matches acima do limiar. Encerrando.")
                        break

                    cx_dev = tx + w / 2.0
                    cy_dev = ty + h / 2.0
                    cx_css = bbox["x"] + (cx_dev / dpr)
                    cy_css = bbox["y"] + (cy_dev / dpr)
                    click_css(cx_css, cy_css)
                    time.sleep(args.cooldown)
            print("[i] Concluído.")
        except KeyboardInterrupt:
            print("\n[i] Interrompido por você (Ctrl+C).")
        # Não fechar o Chrome

if __name__ == "__main__":
    main()
