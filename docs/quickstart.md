```text
StreamPipe

core flow
- build with cmake
- run in sim mode
- check WAL and UDP alerts
```

```bash
cmake -S . -B build
cmake --build build
./build/streampipe --mode sim --window 24 --threshold 3.0 --interval-ms 200
```

```text
minimal run args
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
./build/streampipe --mode uart --device /dev/ttyUSB0 --sensor-id 2 --interval-ms 200
```
