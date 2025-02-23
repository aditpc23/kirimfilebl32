#include <Arduino.h>
#include "BluetoothSerial.h"
#include "FS.h"
#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true
#define BUFFER_SIZE 409600  // Buffer untuk menyimpan data sementara sebelum ditulis ke file

BluetoothSerial SerialBT;
bool receivingFile = false;
String fileName = "";
String fileBuffer = "";
unsigned long lastReceiveTime = 0;

void startReceivingFile(const String &name) {
  fileName = "/myplaylist/" + name;
  fileBuffer = "";  // Kosongkan buffer
  receivingFile = true;
  lastReceiveTime = millis();
  Serial.println("Menerima file: " + fileName);
}

void receiveFileData() {
  while (SerialBT.available()) {
    char c = SerialBT.read();
    fileBuffer += c;
    lastReceiveTime = millis();
  }
}

void stopReceivingFile() {
  if (receivingFile) {
    Serial.println("Transfer selesai. Menyimpan file...");
    
    // Pastikan folder myplaylist ada
    if (!LittleFS.exists("/myplaylist")) {
      LittleFS.mkdir("/myplaylist");
    }

    // Simpan buffer ke file
    File file = LittleFS.open(fileName, FILE_WRITE);
    if (file) {
      file.print(fileBuffer);
      file.close();
      Serial.println("File berhasil disimpan: " + fileName);
    } else {
      Serial.println("Gagal menyimpan file!");
    }
    
    receivingFile = false;
    fileBuffer = "";  // Bersihkan buffer setelah menyimpan
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT_FileTransfer");

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("Gagal mount LittleFS!");
    return;
  }

  Serial.println("Bluetooth siap. Kirim nama file untuk memulai transfer.");
}

void loop() {
  if (SerialBT.available()) {
    if (!receivingFile) {
      // Menerima nama file pertama kali
      fileName = SerialBT.readStringUntil('\n');
      fileName.trim();
      startReceivingFile(fileName);
    } else {
      // Menerima isi file
      receiveFileData();
    }
  }

  // Jika tidak ada data baru selama 2 detik, anggap transfer selesai
  if (receivingFile && (millis() - lastReceiveTime > 2000)) {
    stopReceivingFile();
  }
}
