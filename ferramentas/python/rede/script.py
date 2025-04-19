from scapy.all import sniff, IP, TCP, Raw
from datetime import datetime
import json
import os
import hashlib
import string
from multiprocessing import Process, Queue
from viewer import start_ui  # <- você vai criar esse arquivo


# Garante que a pasta 'logs' exista
os.makedirs("logs", exist_ok=True)

def is_printable(data):
    return all(chr(b) in string.printable for b in data)

def parse_packet_factory(queue):
    def parse_packet(packet):
        if IP in packet and TCP in packet:
            payload = bytes(packet[Raw].load) if Raw in packet else b""

            direction = "ENVIADO" if packet[IP].src.startswith("192.168.") else "RECEBIDO"
            payload_hex = payload.hex()
            payload_hash = hashlib.md5(payload).hexdigest() if payload else ""
            payload_preview = payload.decode(errors="ignore")[:50] if is_printable(payload) else ""

            data = {
                "timestamp": datetime.now().isoformat(),
                "direction": direction,
                "src": packet[IP].src,
                "dst": packet[IP].dst,
                "sport": packet[TCP].sport,
                "dport": packet[TCP].dport,
                "protocol": "TCP",
                "flags": packet[TCP].flags.value,
                "len": len(payload),
                "payload_hex": payload_hex,
                "payload_ascii_preview": payload_preview,
                "payload_hash": payload_hash
            }

            with open("logs/traffic.json", "a", encoding="utf-8") as f:
                f.write(json.dumps(data, ensure_ascii=False) + "\n")

            # Envia para a UI em tempo real
            queue.put(data)
    return parse_packet


if __name__ == "__main__":
    queue = Queue()
    ui_process = Process(target=start_ui, args=(queue,))
    ui_process.start()

    try:
        print("[📡] Capturando pacotes... Pressione CTRL+C para parar.")
        sniff(
            filter="host 35.199.111.15 or host 35.247.221.22",
            prn=parse_packet_factory(queue),
            store=0
        )
    except KeyboardInterrupt:
        print("\n⛔ Captura interrompida pelo usuário.")
    finally:
        ui_process.terminate()
