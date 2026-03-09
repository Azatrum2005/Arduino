#include "Arduino.h"
#include "Audio.h"
#include "FS.h"
#include "SD_MMC.h"

#define SD_MMC_CMD 15
#define SD_MMC_CLK 14
#define SD_MMC_D0  2
#define I2S_BCLK   26
#define I2S_DOUT   22
#define I2S_LRC    25

// #define BUTTON_PIN1 13
// #define BUTTON_PIN2 27
int xyzPins[] = {13, 4, 27};

String trackList[50]; 
int trackCount = 0;
int currentIndex = 0;
int volflag = 0;
int trackflag = 0;
int playstate = 1;
int play = 1;
// unsigned long lastPressTime = 0;
// int pressCount1 = 0;
// int pressCount2 = 0;
// const unsigned long doublePressInterval1 = 500;
// const unsigned long doublePressInterval2 = 100;
// int lastState1 = HIGH;
// int lastState2 = HIGH;
int resumePos = 0;
int volume = 10;

Audio audio;

void loadTrackList() {
  File root = SD_MMC.open("/music");
  File file;
  while ( (file = root.openNextFile()) ) {
    if (!file.isDirectory()) {
      trackList[trackCount++] = file.path();//String("/music/") + file.name();
    }
    file.close();
  }
  root.close();
}
// void audio_info(const char *info) {
//   Serial.print("info        ");
//   Serial.println(info);
// }
void audio_eof_mp3(const char *info) { 
  Serial.println("looping...");
  audio.connecttoFS(SD_MMC, trackList[currentIndex].c_str());
}

void setup() {
  Serial.begin(115200);
  // pinMode(BUTTON_PIN1, INPUT);
  // pinMode(BUTTON_PIN2, INPUT);
  pinMode(xyzPins[2], INPUT);
  
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true, true, 20000, 5)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD_MMC card attached");
    return;
  }
  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(volume);  // 0...21
  loadTrackList();
  audio.connecttoFS(SD_MMC, trackList[currentIndex].c_str());

}

void loop() {
  audio.loop();
  int vol = analogRead(xyzPins[0]);
  int track = analogRead(xyzPins[1]);
  play = digitalRead(xyzPins[2]);
  if (play == 0){
    delay(300);
    playstate=!playstate;
  }
  if (playstate==0 && play==0){
    resumePos = audio.getFilePos();
    audio.stopSong();
    Serial.println("Paused");
  }
  if (playstate==1 && play==0){
    audio.connecttoFS(SD_MMC, trackList[currentIndex].c_str(),resumePos);
    Serial.println("Resumed");
  }

  if (vol==0 || vol==4095){
    delay(500);
    volflag=1;
  }
  if (track==0 || track==4095){
    delay(500);
    trackflag=1;
  }

  if (volflag == 1) {
    if (vol == 0) {
      volume = min(21,volume + 2);
    } else if (vol == 4095) {
      volume = max(0,volume - 4);
    }
    Serial.printf("Volume: %d\n",volume);
    audio.setVolume(volume);
    volflag=0;
  }
  if (trackflag == 1) {
    if (track <= 0) {
      currentIndex = (currentIndex + 1) % trackCount;
    } else if (track == 4095) {
      currentIndex = (currentIndex - 1 + trackCount) % trackCount;
    }
    Serial.printf("Playing: %s\n", trackList[currentIndex].c_str());
    audio.stopSong();
    audio.connecttoFS(SD_MMC, trackList[currentIndex].c_str());
    trackflag=0;
  }

  // int state1 = digitalRead(BUTTON_PIN1);
  // // static int lastState = HIGH;
  // if (state1 == LOW && lastState1 == HIGH) {
  //   delay(100);
  //   pressCount1++;
  //   lastPressTime = millis();
  // }
  // lastState1 = state1;

  // int state2 = digitalRead(BUTTON_PIN2);
  // if (state2 == LOW && lastState2 == HIGH) {
  //   delay(200);
  //   pressCount2++;
  //   lastPressTime = millis();
  // }
  // lastState2 = state2;

  // if (pressCount1 > 0 && millis() - lastPressTime > doublePressInterval1) {
  //   if (pressCount1 == 1) {
  //     currentIndex = (currentIndex + 1) % trackCount;
  //   } else if (pressCount1 >= 2) {
  //     currentIndex = (currentIndex - 1 + trackCount) % trackCount;
  //   }
  //   Serial.printf("Playing: %s\n", trackList[currentIndex].c_str());
  //   audio.stopSong();
  //   audio.connecttoFS(SD_MMC, trackList[currentIndex].c_str());
  //   pressCount1 = 0;
  // }

  // if (pressCount2 > 0 && millis() - lastPressTime > doublePressInterval2) {
  //   if (pressCount2 == 1) {
  //     volume = min(21,volume + 2);
  //   } else if (pressCount2 >= 2) {
  //     volume = max(0,volume - 4);
  //   }
  //   Serial.printf("Volume: %d\n",volume);
  //   audio.setVolume(volume);
  //   pressCount2 = 0;
  // }

}
