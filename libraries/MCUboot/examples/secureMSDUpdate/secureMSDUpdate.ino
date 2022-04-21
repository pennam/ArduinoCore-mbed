/*
   MCUboot update using QSPI as USB Mass Storage
   This example shows how to update firmware using MCUboot and the QSPI flash mounted as USB MSD

   1 - Bootstrap this sketch on your board
   2 - Open the USB MSD and copy inside the OTA partition the firmware update.bin file
   3 - Unmount the USB MSD
   4 - Open the serial monitor and start the update process
   5 - The Sketch will pad the update file and mark the update as pending
   6 - Reset the board
   7 - MCUboot will take care of the swap and revert process
*/

#include "PluggableUSBMSD.h"
#include "BlockDevice.h"
#include "MBRBlockDevice.h"
#include "FATFileSystem.h"

mbed::BlockDevice* qspi = mbed::BlockDevice::get_default_instance();
mbed::MBRBlockDevice ota_data(qspi, 2);
mbed::FATFileSystem ota("ota");
USBMSD MassStorage(qspi);

bool applyUpdate = false;
bool confirmUpdate = false;

void USBMSD::begin()
{
  int err = ota.mount(&ota_data);
  if (err) {
    Serial.println("Please run WiFiFirmwareUpdater before");
    return;
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial);

  MassStorage.begin();

  Serial.println("Are you ready to apply a new update? Y/[n]");
  applyUpdate = waitResponse();

  if (applyUpdate) {
    setUpdatePending();

    Serial.println("Do you want to make the update permanent? Y/[n]");
    confirmUpdate = waitResponse();

    if (confirmUpdate) {
      setImageOKFlag();
    }
    Serial.println("Done, waiting reset");
  } else {
    Serial.println("No update pending. It's now safe to reboot or disconnect your board.");
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

void setImageOKFlag() {
  FILE* update_file = fopen("/ota/update.bin", "rb+");
  const uint8_t buf[1] = { 0x01 };

  fseek(update_file, ((15 * 128 * 1024) - 64), SEEK_SET);
  int ret = fwrite(buf, 1, 1, update_file);
  if (ret != 1) {
    Serial.println("Error writing Image OK flag");
  }
  fclose(update_file);
}

void setUpdatePending() {
  FILE* update_file = fopen("/ota/update.bin", "rb+");

  const uint32_t boot_img_magic[] = {
    0xf395c277,
    0x7fefd260,
    0x0f505235,
    0x8079b62c,
  };

  if (update_file == nullptr) {
    Serial.println("Cannot open update file");
    return;
  }

  fseek(update_file, 0, SEEK_END);
  int fsize = ftell(update_file);

  Serial.print("File update.bin size ");
  Serial.println( fsize );

  if (fsize < 0x1E0000) {
    const int update_file_size = 15 * 128 * 1024;
    const char buffer[128] = {0xFF};

    Serial.println("\nPadding update file");

    printProgress(fsize, update_file_size, 10, true);
    while (fsize < update_file_size) {
      int ret = fwrite(buffer, 1, 1, update_file);
      if (ret != 1) {
        Serial.println("Error writing update file");
        break;
      }
      fsize += 1;
      printProgress(fsize, update_file_size, 10, false);
    }
  }
  Serial.println("Flashed 100%");

  fseek(update_file, ((15 * 128 * 1024) - 32), SEEK_SET);
  int ret = fwrite(boot_img_magic, 16, 1, update_file);
  if (ret != 1) {
    Serial.println("Error writing magic");
  }

  fclose(update_file);
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

void loop() {
  delay(1000);
}
