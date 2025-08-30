# OpenAuto SplitHost — v8 (tout-en-un)
- SplitHost (Qt/X11) + **Hub Web** (Svelte/Tailwind/DaisyUI)
- OpenAuto CE intégré (build script), **AP Wi‑Fi + BT + DHCP**, **udev**, **audio jack + micro USB**.
- **Installe tout en 1 commande** : `scripts/install_all.sh`

## Démarrage rapide
```bash
# 1) Lancer l'install complète (peut demander sudo plusieurs fois)
bash scripts/install_all.sh

# 2) Lancer l'app (si non démarrée via service)
scripts/run_splithost.sh /opt/openauto-ce/openauto/build/autoapp
```
Personnaliser SSID/PSK/IFACE:
```bash
SSID=MaVoiture PSK=Mot2Passe IFACE=wlan0 bash scripts/install_all.sh
```
