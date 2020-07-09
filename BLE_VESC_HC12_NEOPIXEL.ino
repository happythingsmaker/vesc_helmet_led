
#include <Adafruit_NeoPixel.h>
#include <VescUart.h>

//#define DEBUG 1
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

#ifdef DEBUG
  Serial.begin(115200);
#endif
  Serial1.begin(115200);
  Serial2.begin(115200);

}

long lastTime_COMM_GET_VALUES = 0;

long lastBLEms = 0;
bool bleRequested = false;

int return_mode  = 0;

long lastMainloopMillis = 0;
long thisMainLoopMillis = 0;
long lastDataMillis = 0;
bool sleepMode = false;


// MAIN LOOP START! ========================================
void loop() {
  thisMainLoopMillis = millis();

  //  standalone mode without BLE
  if (thisMainLoopMillis - lastBLEms > 5000) {
    bleRequested  = false;
  }

  // sleep mode
  if (thisMainLoopMillis - lastDataMillis  > 10000) {
    sleepMode = true;
  }


  // VESC(HC12)-> BLE ============================
  while (Serial1.available()) {
    uint8_t inByte = Serial1.read();

    if (bleRequested) {
      Serial2.write(inByte);
    }

    return_mode = UART.getVescValues(inByte);


    if (return_mode > 0) {
      check_brake();
      pixels.show();

#ifdef DEBUG
      Serial.print("return_mode ");
      Serial.println(return_mode);
      Serial.println(thisMainLoopMillis - lastTime_COMM_GET_VALUES );
#endif
      lastTime_COMM_GET_VALUES = thisMainLoopMillis;

    } else {
    }

    lastDataMillis = thisMainLoopMillis;
    sleepMode = false;
  }

  // BLE -> VESC(HC12) =============================
  while (Serial2.available()) {
    volatile  uint8_t inByte = Serial2.read();
    Serial1.write(inByte);
    bleRequested = true;
    lastBLEms = thisMainLoopMillis;
    lastDataMillis = thisMainLoopMillis;
    sleepMode = false;
  }

  // ms
  spin(20);

}


// SPIN ======================================================================================
long currnetTime;

void spin(int desiredMillis) {
  static long lastSpinTime;

  if (thisMainLoopMillis - lastSpinTime >  desiredMillis) {
    lastSpinTime = currnetTime;
    looping_function();
  }

  if (sleepMode)
  {
    sleep();
  }


}

int brakeState = 0;
enum BRAKE_CASE  {NO_BRAKE, NEUTRAL, ACCELERATION, BRAKE_DECELERATION, BRAKE_REVERSE };
int before_brake_status = 0;

int ledCount = 0;
void looping_function() {
  if (thisMainLoopMillis - lastTime_COMM_GET_VALUES > 20 && !bleRequested) {
    lastTime_COMM_GET_VALUES = thisMainLoopMillis;
    UART.requestVescGetValues();

#ifdef DEBUG
    Serial.println("request data manually");
    Serial.println(lastTime_COMM_GET_VALUES);
#endif

  }
  //  check_brake();
#ifdef DEBUG
  Serial.println(UART.data.rpm);
#endif


}


void check_brake(void) {

  // BRAKE_DECELERATION
  if (UART.data.avgMotorCurrent < 0) {
    brakeState = BRAKE_DECELERATION;
  }

  // NEUTRAL
  else if (UART.data.avgMotorCurrent == 0 && UART.data.rpm >= 0 ) {
    brakeState = NEUTRAL;
  }

  // ACCELERATION
  else if (UART.data.avgMotorCurrent > 0 && UART.data.rpm > 0 ) {
    brakeState = ACCELERATION;
  }

  // BRAKE_REVERSE
  else if (UART.data.rpm <= 0) {
    brakeState = BRAKE_REVERSE;
  }

  int tempRPM = UART.data.rpm;
  if (tempRPM > 15000) {
    tempRPM = 15000;
  }
  int rpm_mapped = map(UART.data.rpm, 0, 15000, 0, 255);

  if (brakeState != before_brake_status) {
    ledCount = 0;
  }

  switch (brakeState) {

    case BRAKE_DECELERATION:
      setColor(255, 0, 0);
      break;

    case NEUTRAL:
      if (ledCount++ < 5) {
        setColor(255, 255, 255);

      } else if (ledCount++ < 130) {
        setColor(30, 3, 3);
      } else {
        ledCount = 0;
      }

      break;

    case ACCELERATION:
      setColor(rpm_mapped / 2, rpm_mapped / 2, rpm_mapped);
      break;


    case BRAKE_REVERSE:
      setColor(150, 0, 0);
      break;

    default:
      break;


  }


  before_brake_status = brakeState;
}


void setColor(int r, int g, int b) {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));

  }
}

void sleep() {
  for (int i = 0; i < 25; i ++) {
    setColor(i, i / 10, i / 10);
    pixels.show();
    delay(10);
  }
  for (int i = 25; i >= 0; i --) {
    setColor(i, i / 10, i / 10);
    delay(10);
    pixels.show();
  }
  delay(3000);

}
