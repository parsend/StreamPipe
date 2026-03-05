#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BIN_PATH="/usr/local/bin/streampipe"
SERVICE_SRC="deploy/streampipe.service"
ENV_SRC="deploy/streampipe.env.example"
SERVICE_NAME="streampipe"
SERVICE_DEST="/etc/systemd/system/${SERVICE_NAME}.service"
ENV_DEST="/etc/streampipe/streampipe.env"
DATA_DIR="/var/lib/streampipe"

if [ ! -x "${BUILD_DIR}/streampipe" ]; then
  echo "build/${SERVICE_NAME} not found or not executable"
  exit 1
fi

if ! id -u streampipe >/dev/null 2>&1; then
  useradd --system --shell /usr/sbin/nologin --home-dir /var/lib/streampipe streampipe
fi

mkdir -p /etc/streampipe "${DATA_DIR}"
cp "${BUILD_DIR}/streampipe" "${BIN_PATH}"
chmod 755 "${BIN_PATH}"
cp "${SERVICE_SRC}" "${SERVICE_DEST}"
cp "${ENV_SRC}" "${ENV_DEST}"
chown root:root "${BIN_PATH}" "${SERVICE_DEST}"
chown -R streampipe:streampipe "${DATA_DIR}" /etc/streampipe
chmod 640 "${ENV_DEST}"
chmod 644 "${SERVICE_DEST}"

systemctl daemon-reload
systemctl enable --now "${SERVICE_NAME}"

echo "installed ${SERVICE_NAME}, check: systemctl status ${SERVICE_NAME}"
