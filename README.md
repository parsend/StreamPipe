```text
StreamPipe

Edge pipeline for Linux with fixed point rolling statistics and anomaly detection

features
- reads values from sim, uart, spi, i2c
- computes moving average and variance in fixed point
- detects anomalies through z-score
- writes only anomaly events to WAL
- sends alerts by UDP only
```

```bash
cmake -S . -B build
cmake --build build
./build/streampipe --mode sim --window 24 --threshold 3.0 --interval-ms 200
```

```bash
make
```

```text
supported modes
sim uses internal generator
uart reads text line based signed integers
spi and i2c read little endian signed 32 bit raw values
```

```text
project state
linux only target with C11 and pthread
no heavy external dependencies
optional sse4.2 crc path
```

```text
github artifacts
.github/workflows/ci.yml
.gitignore
Makefile
docker/Dockerfile
deploy/streampipe.service
deploy/streampipe.env.example
scripts/run-sim.sh
scripts/release.sh
scripts/install-service.sh
```

```bash
--mode sim|uart|spi|i2c
--device /dev/ttyUSB0
--sensor-id 1
--window 24
--threshold 3.0
--interval-ms 200
--wal-path streampipe.wal
--alert-host 127.0.0.1
--alert-port 9999
```

```bash
cmake -S . -B build && cmake --build build
sudo cp build/streampipe /usr/local/bin/
sudo cp deploy/streampipe.service /etc/systemd/system/
sudo cp deploy/streampipe.env.example /etc/streampipe/streampipe.env
sudo systemctl daemon-reload
sudo systemctl enable --now streampipe
```

```text
extra docs
docs/quickstart.md
CONTRIBUTING.md
LICENSE
```
