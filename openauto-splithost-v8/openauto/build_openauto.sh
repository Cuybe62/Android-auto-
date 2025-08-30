#!/usr/bin/env bash
set -euo pipefail
WORKDIR="${WORKDIR:-/opt/openauto-ce}"
AASDK_REPO="${AASDK_REPO:-https://github.com/f1xpl/aasdk.git}"
OPENAUTO_REPO="${OPENAUTO_REPO:-https://github.com/f1xpl/openauto.git}"
sudo apt update
sudo apt install -y build-essential cmake git libusb-1.0-0-dev libssl-dev   libprotobuf-dev protobuf-compiler libqt5multimedia5 qtmultimedia5-dev   libboost-all-dev libbluetooth-dev
sudo mkdir -p "$WORKDIR"; sudo chown -R "$USER":"$USER" "$WORKDIR"
cd "$WORKDIR"
[[ -d aasdk ]] || git clone --depth=1 "$AASDK_REPO" aasdk
[[ -d openauto ]] || git clone --depth=1 "$OPENAUTO_REPO" openauto
mkdir -p "$WORKDIR/aasdk/build" && cd "$WORKDIR/aasdk/build"
cmake -DCMAKE_BUILD_TYPE=Release -DAASDK_WITH_EXAMPLES=OFF ..
make -j"$(nproc)"
sudo make install
mkdir -p "$WORKDIR/openauto/build" && cd "$WORKDIR/openauto/build"
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j"$(nproc)"
echo "Binaire: $WORKDIR/openauto/build/autoapp"
