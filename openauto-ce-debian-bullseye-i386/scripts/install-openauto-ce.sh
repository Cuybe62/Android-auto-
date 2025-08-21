#!/usr/bin/env bash
set -euo pipefail

echo "[1/5] Dépendances…"
sudo apt-get update
sudo apt-get install -y build-essential cmake git pkg-config   libusb-1.0-0-dev libssl-dev libprotobuf-dev protobuf-compiler   libboost-all-dev qtbase5-dev qtmultimedia5-dev libqt5multimedia5-plugins   libqt5svg5-dev qtchooser qt5-qmake qtbase5-dev-tools   librtaudio-dev libcurl4-openssl-dev libudev-dev   gstreamer1.0-plugins-base gstreamer1.0-plugins-good   gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav   gstreamer1.0-alsa gstreamer1.0-pulseaudio pulseaudio   libqt5xcbqpa5 libxkbcommon-x11-0 libx11-xcb1   libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0 libxcb-xkb1

echo "[2/5] AASDK (clone + build)…"
cd "$HOME"
rm -rf aasdk aasdk_build
git clone --depth=1 -b development https://github.com/opencardev/aasdk.git
mkdir aasdk_build && cd aasdk_build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=14 -DCMAKE_INSTALL_PREFIX=/usr/local ../aasdk
make -j"$(nproc)" && sudo make install && sudo ldconfig

echo "[3/5] OpenAuto CE (clone + build)…"
cd "$HOME"
rm -rf openauto openauto_build
git clone --depth=1 https://github.com/humeman/openauto.git
mkdir openauto_build && cd openauto_build
cmake -DCMAKE_BUILD_TYPE=Release -DRPI3_BUILD=FALSE -DCMAKE_PREFIX_PATH=/usr/local ../openauto
make -j"$(nproc)"

echo "[4/5] Règle udev Android…"
sudo tee /etc/udev/rules.d/51-android.rules >/dev/null <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", MODE="0666", GROUP="plugdev"
EOF
sudo groupadd plugdev 2>/dev/null || true
sudo usermod -aG plugdev "$USER"
sudo udevadm control --reload-rules && sudo udevadm trigger

echo "[5/5] Terminé."
echo "Binaire : $HOME/openauto/bin/autoapp"
echo "Lance :   $HOME/openauto/bin/autoapp"
