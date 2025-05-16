Em USB, todo dispositivo é identificado por dois números de 16 bits:

1. **VID (Vendor ID)** – identifica o fabricante (por exemplo, 0x045E é da Microsoft).
2. **PID (Product ID)** – identifica o modelo/produto dentro da linha do fabricante (por exemplo, 0x028E pode ser um teclado específico).

> **Spoofar (spoofing) de VID/PID** significa interceptar ou alterar esses valores antes que o sistema operacional os reconheça, fazendo com que o dispositivo “pense” ser outro.

---

## Por que alguém faz spoofing de VID/PID?

* **Bypass de restrições de driver**
  Alguns drivers proprietários só carregam se o dispositivo tiver VID/PID conhecidos. Spoofar faz o SO carregar um driver genérico ou o driver de outro hardware compatível.

* **Evasão de anti-cheat ou detecção**
  Anti-cheats (como EAC, BattlEye etc.) às vezes inspecionam a árvore de dispositivos USB para detectar hardwares “não autorizados” (joysticks, interceptadores de teclado…). Alterar VID/PID pode camuflar seu dispositivo de entrada como um genérico.

* **Teste e desenvolvimento**
  Durante o desenvolvimento de firmware USB, é útil testar como o host reage a diferentes VIDs/PIDs sem trocar de hardware.

---

## Como se faz o spoofing?

1. **No firmware do dispositivo**
   Se você controla o firmware (por exemplo, em um microcontrolador que implementa USB), basta trocar as constantes de descriptor para outro VID/PID.

2. **No Windows**

   * Criar ou alterar um *filter driver* (filtro em modo kernel) que intercepte IRPs de PnP e reescreva os descriptors antes de repassá-los ao stack de USB.
   * Usar um mapeador de driver (kdmapper, ect.) para injetar código que sobrescreva o valor em memória.

3. **No Linux**

   * Usar `usbfs` ou um gadget driver (no caso de USB OTG) para mascarar o descriptor.
   * Ferramentas como `usbip` permitem criar dispositivos virtuais com VID/PID arbitrários.

---

## Riscos e cuidados

* **Assinatura de driver (Driver Signature Enforcement)**
  No Windows 10+ é preciso driver assinado para filtros kernel; mapeadores não assinados podem causar queda de sistema ou falha no carregamento.

* **Incompatibilidades**
  Se você spoofar para um VID/PID de um dispositivo que requer firmware ou protocolos específicos, o SO pode tentar usar comandos inválidos e travar o barramento.

* **Detecção avançada**
  Anti-cheats mais sofisticados não olham só VID/PID, mas padrões de tráfego USB, timings e outros metadados.

---

### Resumo rápido

* **VID** = identificação do fabricante.
* **PID** = identificação do produto.
* **Spoofar** = falsificar esses IDs no driver/firmware.
* **Por quê?** Para forçar carregamento de drivers, camuflar hardware contra sistemas de segurança ou emulá-los em testes.

abordagem  (ex.: filtro WDF no Windows, gadget no Linux)