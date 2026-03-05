BUILD_DIR := build
TARGET := streampipe

.PHONY: all clean run-sim run-docker

all:
	cmake -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --parallel

clean:
	rm -rf $(BUILD_DIR)

run-sim:
	$(MAKE) all
	./$(BUILD_DIR)/$(TARGET) --mode sim --window 24 --threshold 3.0 --interval-ms 200

run-docker:
	docker build -t streampipe .
	docker run --rm -it --name streampipe --network host streampipe
