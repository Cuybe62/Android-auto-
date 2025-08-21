# OpenAuto CE — Debian/Raspios Bullseye i386 (x86_64)

Guide d’installation **testé sur** : Raspios/Debian Bullseye i386 (2022-07-01), CPU x86_64, interface graphique (X11).  
Objectif : compiler **AASDK** (fork maintenu) + **OpenAuto CE** (fork patché), installer les dépendances, configurer l’USB Android et **démarrer automatiquement** au login.

## Résumé rapide

```bash
# Dépendances
sudo apt-get update
sudo apt-get install -y build-essential cmake git pkg-config   libusb-1.0-0-dev libssl-dev libprotobuf-dev protobuf-compiler   libboost-all-dev qtbase5-dev qtmultimedia5-dev libqt5multimedia5-plugins   libqt5svg5-dev qtchooser qt5-qmake qtbase5-dev-tools   librtaudio-dev libcurl4-openssl-dev libudev-dev   gstreamer1.0-plugins-base gstreamer1.0-plugins-good   gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav   gstreamer1.0-alsa gstreamer1.0-pulseaudio pulseaudio   libqt5xcbqpa5 libxkbcommon-x11-0 libx11-xcb1   libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-render-util0 libxcb-xkb1

# AASDK (fork à jour)
cd ~ && git clone --depth=1 -b development https://github.com/opencardev/aasdk.git
mkdir ~/aasdk_build && cd ~/aasdk_build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=14 -DCMAKE_INSTALL_PREFIX=/usr/local ../aasdk
make -j"$(nproc)" && sudo make install && sudo ldconfig

# OpenAuto CE (fork patché)
cd ~ && git clone --depth=1 https://github.com/humeman/openauto.git
mkdir ~/openauto_build && cd ~/openauto_build
cmake -DCMAKE_BUILD_TYPE=Release -DRPI3_BUILD=FALSE -DCMAKE_PREFIX_PATH=/usr/local ../openauto
make -j"$(nproc)"

# Règle udev (USB Android)
sudo tee /etc/udev/rules.d/51-android.rules >/dev/null <<'EOF'
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", MODE="0666", GROUP="plugdev"
EOF
sudo groupadd plugdev 2>/dev/null || true
sudo usermod -aG plugdev "$USER"
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Lancer :
```bash
~/openauto/bin/autoapp
```

## Démarrage auto (service systemd *user*)
Utiliser `systemd/openauto.service` fourni puis activer :
```bash
mkdir -p ~/.config/systemd/user
cp systemd/openauto.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now openauto.service
```
> Conseil : si tu veux démarrer même sans ouvrir de session graphique : `sudo loginctl enable-linger "$USER"`.

## Dépannage rapide
- **Qt/X11** : lancer depuis la session graphique (pas via sudo). Si SSH : `DISPLAY=:0` et `XAUTHORITY=/home/<user>/.Xauthority`.
- **H.264 manquant** : vérifier `gstreamer1.0-libav` est installé.
- **Pas de son** : `gstreamer1.0-pulseaudio` + `pavucontrol` pour choisir la sortie.
- **USB non détecté** : ajouter l’`idVendor` exact vu via `lsusb` dans `/etc/udev/rules.d/51-android.rules`.

## Licence
Ce repo ne contient que des scripts/units. AASDK et OpenAuto CE appartiennent à leurs auteurs respectifs (voir leurs LICENSE dans les dépôts clonés).
