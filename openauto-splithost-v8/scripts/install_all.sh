#!/usr/bin/env bash
set -euo pipefail

# Config overridable via env
SSID="${SSID:-OpenAuto-AP}"
PSK="${PSK:-OpenAuto1234}"
IFACE="${IFACE:-wlan0}"
AP_NET="${AP_NET:-192.168.50.1/24}"
DHCP_START="${DHCP_START:-192.168.50.50}"
DHCP_END="${DHCP_END:-192.168.50.200}"
LINGER="${LINGER:-true}"  # enable linger by default

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_LOCAL="$HOME/.local/share/openauto-splithost/bin"
UNIT_USER_DIR="$HOME/.config/systemd/user"

echo "==> Installation des dépendances (Qt/X11/WebEngine/BT/Wi‑Fi/Audio)…"
sudo apt update
sudo apt install -y \
  build-essential cmake pkg-config git \
  qtbase5-dev qtbase5-private-dev qttools5-dev-tools \
  libxcb1-dev libxcb-keysyms1-dev libxcb-icccm4-dev libxcb-ewmh-dev \
  libx11-xcb-dev libx11-dev \
  qtwebengine5-dev qml-module-qtwebchannel libqt5webchannel5-dev \
  pulseaudio-utils alsa-utils \
  bluez bluez-tools rfkill hostapd dnsmasq iw iproute2 \
  v4l-utils pulseaudio-module-bluetooth || { echo 'deps install failed'; exit 1; }

echo "==> Préparation dossiers locaux…"
mkdir -p "$BIN_LOCAL" "$UNIT_USER_DIR" "$ROOT/hub/www"

echo "==> Build AASDK + OpenAuto CE (si nécessaire)…"
AUTOAPP_DEFAULT="/opt/openauto-ce/openauto/build/autoapp"
if [[ ! -x "$AUTOAPP_DEFAULT" ]]; then
  sudo bash "$ROOT/openauto/build_openauto.sh"
fi
AUTOAPP_PATH="$AUTOAPP_DEFAULT"
if [[ ! -x "$AUTOAPP_PATH" ]]; then
  if command -v autoapp >/dev/null 2>&1; then AUTOAPP_PATH="$(command -v autoapp)"; fi
fi
if [[ ! -x "$AUTOAPP_PATH" ]]; then
  echo "⚠️ Impossible de trouver autoapp. Vous pouvez le builder plus tard et mettre à jour AUTOAPP_PATH."
fi
echo "AUTOAPP_PATH=$AUTOAPP_PATH"

echo "==> Build Hub Web (Svelte)…"
if command -v npm >/dev/null 2>&1; then
  bash "$ROOT/scripts/build_hub_web.sh"
else
  echo "⚠️ Node/npm non disponible, je saute le build web (vous pourrez le faire plus tard)."
fi

echo "==> Build SplitHost (Qt)…"
bash "$ROOT/scripts/build_all.sh"
SPLITHOST_BIN="$ROOT/build/apps/splithost/splithost"
sudo install -m 0755 "$SPLITHOST_BIN" /usr/local/bin/splithost

echo "==> Installation scripts binaires (wait-network, select-audio)…"
install -m 0755 "$ROOT/openauto-ce-debian-bullseye-i386/bin/wait-network.sh" "$BIN_LOCAL/wait-network.sh"
install -m 0755 "$ROOT/openauto-ce-debian-bullseye-i386/bin/select-audio.sh" "$BIN_LOCAL/select-audio.sh"

echo "==> Udev (vendors Android)…"
sudo install -m 0644 "$ROOT/openauto-ce-debian-bullseye-i386/udev/51-android.rules" /etc/udev/rules.d/51-android.rules
sudo udevadm control --reload-rules && sudo udevadm trigger

echo "==> Wi‑Fi AP + DHCP + Bluetooth (hostapd/dnsmasq)…"
# Write minimal hostapd/dnsmasq and net unit using our variables
sudo tee /etc/hostapd/hostapd.conf >/dev/null <<EOF
interface=${IFACE}
driver=nl80211
ssid=${SSID}
hw_mode=g
channel=6
ieee80211n=1
wmm_enabled=1
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_key_mgmt=WPA-PSK
wpa_passphrase=${PSK}
rsn_pairwise=CCMP
EOF
sudo sed -i 's|^#\?DAEMON_CONF=.*|DAEMON_CONF="/etc/hostapd/hostapd.conf"|' /etc/default/hostapd

sudo tee /etc/systemd/system/openauto-net@.service >/dev/null <<'EOF'
[Unit]
Description=Configure static IP for %I (OpenAuto AP)
Before=hostapd.service dnsmasq.service
Wants=hostapd.service dnsmasq.service
[Service]
Type=oneshot
ExecStart=/sbin/ip link set dev %I up
ExecStart=/sbin/ip addr flush dev %I
ExecStart=/sbin/ip addr add 192.168.50.1/24 dev %I
RemainAfterExit=yes
[Install]
WantedBy=multi-user.target
EOF
sudo systemctl daemon-reload
sudo systemctl enable --now "openauto-net@${IFACE}.service"

sudo mkdir -p /etc/dnsmasq.d
sudo tee /etc/dnsmasq.d/openauto.conf >/dev/null <<EOF
interface=${IFACE}
bind-interfaces
dhcp-range=${DHCP_START},${DHCP_END},255.255.255.0,12h
domain-needed
bogus-priv
EOF
sudo rfkill unblock wifi bluetooth || true
sudo systemctl enable --now bluetooth
sudo systemctl unmask hostapd || true
sudo systemctl enable --now hostapd dnsmasq

echo "==> Groupes utilisateur (plugdev,audio,video)…"
sudo usermod -aG plugdev,audio,video "$USER" || true

echo "==> Services systemd (user) — OpenAuto + audio…"
# openauto.service (user)
tee "$UNIT_USER_DIR/openauto.service" >/dev/null <<EOF
[Unit]
Description=OpenAuto CE (user session)
After=default.target
StartLimitIntervalSec=0
[Service]
Type=simple
Environment=AUTOAPP_PATH=${AUTOAPP_PATH}
Environment=WAIT_NET=${BIN_LOCAL}/wait-network.sh
ExecStartPre=\${WAIT_NET} ${IFACE}
ExecStart=\${AUTOAPP_PATH}
Restart=on-failure
RestartSec=2
[Install]
WantedBy=default.target
EOF

# openauto-audio.service (user)
tee "$UNIT_USER_DIR/openauto-audio.service" >/dev/null <<EOF
[Unit]
Description=Fixer micro USB et sortie jack pour OpenAuto (PulseAudio)
After=default.target
[Service]
Type=simple
ExecStart=${BIN_LOCAL}/select-audio.sh --watch
Restart=always
RestartSec=1
[Install]
WantedBy=default.target
EOF

systemctl --user daemon-reload
systemctl --user enable --now openauto-audio.service || true
systemctl --user enable --now openauto.service || true

if [[ "$LINGER" == "true" ]]; then
  echo "==> loginctl enable-linger (démarrage user sans session graphique)…"
  sudo loginctl enable-linger "$USER" || true
fi

echo ""
echo "✅ Installation terminée."
echo "   SSID=${SSID}  PSK=${PSK}  IFACE=${IFACE}"
echo "   Binaire SplitHost: /usr/local/bin/splithost"
echo "   Service user openauto.service: $(systemctl --user is-enabled openauto.service 2>/dev/null || true)"
echo "   Pour lancer manuellement: scripts/run_splithost.sh ${AUTOAPP_PATH:-/opt/openauto-ce/openauto/build/autoapp}"
