# ECG_ESP32

### Dependências
idf.py add-dependency espressif/mdns \
idf.py add-dependency joltwallet/littlefs==1.20.1

### LittleFS / Partições
idf.py menuconfig -> partition -> custom partition(csv) -> partitions.csv \
idf.py menuconfig -> Serial Flash Config -> Flash Size -> 4MB