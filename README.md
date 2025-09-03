# ECG_ESP32

### Submódulo
git add-submodule https://github.com/dianlight/esp-idf-lib.git

### Dependências
idf.py add-dependency espressif/mdns
idf.py add-dependency joltwallet/littlefs==1.20.1

### LittleFS
idf.py menuconfig -> partition -> custom partition(csv) -> partitions.csv