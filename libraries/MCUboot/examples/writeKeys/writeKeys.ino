#include "FlashIAP.h"
#include "BlockDevice.h"
#include "MBRBlockDevice.h"
#include "FATFileSystem.h"
#include "ecdsa-p256-encrypt-key.h"
#include "ecdsa-p256-signing-key.h"

#define BOOTLOADER_ID     (0x80002F0)
#define SIGNING_KEY_ADDR  (0x8000300)
#define ENCRYPT_KEY_ADDR  (0x8000400)
#define ENCRYPT_KEY_SIZE  (0x0000100)
#define SIGNING_KEY_SIZE  (0x0000100)

mbed::FlashIAP flash;
bool writeKeys = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  String currentBootloaderIdentifier = String((uint8_t*)(BOOTLOADER_ID), 15);

  if(currentBootloaderIdentifier.equals("MCUboot Arduino")) {
    Serial.println("The bootloader comes with a set of default keys to evaluate signing and encryption process");
    Serial.println("If you load the keys, you will need to upload the future sketches with Security Settings -> Signing + Encryption.");
    Serial.println("If you select Security Settings -> None, the sketches will not be executed.");
    Serial.println("Do you want to load the keys? Y/[n]");
    if (waitResponse()) {
      Serial.println("\nPlease notice that loading the keys will enable MCUboot Sketch swap. This will increase the sketch update time after the upload.");
      Serial.println("A violet LED will blink until the sketch is ready to run.");
      Serial.println("Do you want to proceed loading the default keys? Y/[n]");
    }
    writeKeys = waitResponse();

    if (writeKeys) {
      flash.init();
      flash.program(&enc_priv_key, ENCRYPT_KEY_ADDR, ENCRYPT_KEY_SIZE);
      flash.program(&ecdsa_pub_key, SIGNING_KEY_ADDR, SIGNING_KEY_SIZE);
      flash.deinit();
      setupMCUbootOTAData();
      Serial.println("\nKey configuration complete. It's now safe to reboot or disconnect your board.");
    }
  } else {
    Serial.println("Before writing keys you need to update your bootloader.");
    Serial.println("Please run: Examples -> STM32H747_System -> STM32H747_updateBootloader and select MCUboot");
  }

}

bool waitResponse() {
  bool confirmation = false;
  while (confirmation == false) {
    if (Serial.available()) {
      char choice = Serial.read();
      switch (choice) {
        case 'y':
        case 'Y':
          confirmation = true;
          return true;
          break;
        case 'n':
        case 'N':
          confirmation = true;
          return false;
          break;
        default:
          continue;
      }
    }
  }
}

void printProgress(uint32_t offset, uint32_t size, uint32_t threshold, bool reset) {
  static int percent_done = 0;
  if (reset == true) {
    percent_done = 0;
    Serial.println("Flashed " + String(percent_done) + "%");
  } else {
    uint32_t percent_done_new = offset * 100 / size;
    if (percent_done_new >= percent_done + threshold) {
      percent_done = percent_done_new;
      Serial.println("Flashed " + String(percent_done) + "%");
    }
  }
}

void setupMCUbootOTAData() {
  mbed::BlockDevice* qspi = mbed::BlockDevice::get_default_instance();
  mbed::MBRBlockDevice ota_data(qspi, 2);
  mbed::FATFileSystem ota("ota");

  int err = ota.mount(&ota_data);
  if (err) {
    err = ota.reformat(&ota_data);
    if (err) {
      Serial.println("Error creating MCUboot files in OTA partition");
    }
  }

  const char buffer[128] = {0xFF};
  int size = 0;
  memset((void*)buffer, 0xFF, 128);

  FILE* fp = fopen("/ota/scratch.bin", "rb+");

  if(fp == nullptr) {
    fp = fopen("/ota/scratch.bin", "wb");
  }

  const int scratch_file_size = 128 * 1024;
  fseek(fp, 0, SEEK_END);
  int fsize = ftell(fp);

  if (fsize != scratch_file_size) {
    Serial.println("\nCreating scratch file");
    fseek(fp, 0, SEEK_SET);
    printProgress(size, scratch_file_size, 10, true);
    while (size < scratch_file_size) {
      int ret = fwrite(buffer, 128, 1, fp);
      if (ret != 1) {
        Serial.println("Error writing scratch file");
        break;
      }
      size += sizeof(buffer);
      printProgress(size, scratch_file_size, 10, false);
    }
  }
  fclose(fp);

  fp = fopen("/ota/update.bin", "rb+");

  if(fp == nullptr) {
    fp = fopen("/ota/update.bin", "wb");
  }

  const int update_file_size = 15 * 128 * 1024;
  fseek(fp, 0, SEEK_END);
  fsize = ftell(fp);

  if (fsize != update_file_size) {
    Serial.println("\nCreating update file");
    size = 0;
    fseek(fp, 0, SEEK_SET);
    printProgress(size, update_file_size, 10, true);
    while (size < update_file_size) {
      int ret = fwrite(buffer,  128, 1, fp);
      if (ret != 1) {
        Serial.println("Error writing update file");
        break;
      }
      size += sizeof(buffer);
      printProgress(size, update_file_size, 5, false);
    }
  }
  fclose(fp);
}

void loop() {
  delay(1000);
}
