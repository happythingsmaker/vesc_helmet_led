
#include <Adafruit_NeoPixel.h>
#include <VescUart.h>

#define PIN         PB9// On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS   24
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);



VescUart UART;

// BLE side
HardwareSerial Serial2(PA3, PA2);

void setup() {

  UART.setSerialPort(&Serial1);
  //  UART.setDebugPort(&Serial);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  Serial.begin(115200);
  Serial1.begin(115200);
  Serial2.begin(115200);

}


int pooling_cnt = 0;
const int POOLING_MAX = 50000;
bool flag = false;
long lastTime_COMM_GET_VALUES = 0;


long lastBLEms = 0;

long loopingMS = 0;
bool bleRequested = false;

int return_mode  = 0;
long seconds = 1000;
bool valueRead = false;

long lastMS = 0;

// MAIN LOOP START! ========================================
void loop() {

  //  standalone mode without BLE
    if (millis() - lastBLEms > 5000) {
      bleRequested  = false;
    }


  // VESC(HC12)-> BLE ============================
  while (Serial1.available()) {
    uint8_t inByte = Serial1.read();

    if (bleRequested) {
      Serial2.write(inByte);
    }

    return_mode = UART.getVescValues(inByte);

    //    if (return_mode == 4) {
    if (return_mode > 0) {
      Serial.print("return_mode ");
      Serial.println(return_mode);
      Serial.println(millis() - lastTime_COMM_GET_VALUES );
      lastTime_COMM_GET_VALUES = millis();

    } else {
    }
  }

  // BLE -> VESC(HC12) =============================
  while (Serial2.available()) {
    volatile  uint8_t inByte = Serial2.read();
    Serial1.write(inByte);
    bleRequested = true;
    lastBLEms = millis();
  }



  //every (    )ms =====================================
  if (millis() - loopingMS > 100) {
    loopingMS = millis();
    looping_function();
  }
}

void looping_function() {

  if (millis() - lastTime_COMM_GET_VALUES > 100 && !bleRequested) {
    UART.requestVescGetValues();
    Serial.println("request data manually");
    Serial.println(lastTime_COMM_GET_VALUES);
    bleRequested = false;

  }
    check_brake();


}

bool brake = false;
bool before_brake_status = false;


void check_brake(void) {
  // brake case 1 : negative
  if (UART.data.avgMotorCurrent < 0) {
    Serial.println("brake: current nagative");
    brake = true;
  }
  // brake case 2 : low speed with nagative
  else if (UART.data.rpm < 0) {
    Serial.println("brake: low reverse");
    brake = true;
  }
  else {
    Serial.println("no brake");
    brake = false;

  }


  if (brake) {
    if (brake != before_brake_status) {
      setColor(200, 10, 10);
      pixels.show();   // Send the updated pixel colors to the hardware.
    }
    before_brake_status = true;
  }
  else {
    if (brake != before_brake_status) {
      setColor(1, 10, 10);
      pixels.show();   // Send the updated pixel colors to the hardware.
    }
    before_brake_status = false;
  }
}


void setColor(int r, int g, int b) {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));

  }
}
