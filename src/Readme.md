# Connection Pin Set-Up for by CCNG

## MFRC522 Reader/PCD to Arduino Uno

| Signal    | MFRC522 Pin | Arduino Uno Pin |
|-----------|-------------|-----------------|
| RST/Reset | RST         | 9               |
| SPI SS    | SDA(SS)     | 10              |
| SPI MOSI  | MOSI        | 11 / ICSP-4     |
| SPI MISO  | MISO        | 12 / ICSP-1     |
| SPI SCK   | SCK         | 13 / ICSP-3     |


## Relay-Key-Contact to Arduino Uno

| Relay-Key-Contact Pin | Arduino Uno Pin |
|-----------------------|-----------------|
| VCC                   | 5V              |
| GND                   | GND             |
| SIGNAL                | 7               |


## Relay-switch Start ENGINE to Arduino Uno
| Relay-start Pin       | Arduino Uno Pin |
|-----------------------|-----------------|
| VCC                   | 5V              |
| GND                   | GND             |
| SIGNAL                |  4              |

## Trigger contact VOLTAGE to Arduino Uno
|   SENSOR Read Pin     | Arduino Uno Pin |
|-----------------------|-----------------|
|   relay OUTCONTACT    |       6         |


## READ ENGINE_VOLTAGE to Arduino Uno

| From indicator car    | Arduino Uno Pin |
|-----------------------|-----------------|
| Relay-start Pin       | Arduino Uno Pin |
|Indicator ACCU_voltage |       A1        |

> **Note:** Still in the Develop stage...!!