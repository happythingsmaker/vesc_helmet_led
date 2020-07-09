
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
bool blePassMode = true;

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
    blePassMode  = false;
  }

  // sleep mode
  if (thisMainLoopMillis - lastDataMillis  > 10000) {
    sleepMode = true;
  }


  // VESC(HC12)-> BLE ============================
  while (Serial1.available()) {
    uint8_t inByte = Serial1.read();

    if (blePassMode) {
      Serial2.write(inByte);
    }

    return_mode = UART.getVescValues(inByte);

    if (return_mode > 0 ) {

      if (!blePassMode) {
        pixels.show();
      }


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
    uint8_t inByte = Serial2.read();
    Serial1.write(inByte);
    blePassMode = true;
    lastBLEms = thisMainLoopMillis;
    lastDataMillis = thisMainLoopMillis;
    sleepMode = false;
  }

  // ms
  spin(20);


}


// SPIN ======================================================================================

void spin(const int desiredMillis) {
  static long lastSpinTime;

  if (thisMainLoopMillis - lastSpinTime >  desiredMillis) {
    lastSpinTime = thisMainLoopMillis;
    looping_function();

    if (sleepMode)
    {
      sleep();
    }
  }




}

int brakeState = 0;
enum BRAKE_CASE  {NO_BRAKE, NEUTRAL, ACCELERATION, BRAKE_DECELERATION, BRAKE_REVERSE };
int before_brake_status = 0;

bool blePassModeFirstTime = true;

int ledCount = 0;


void looping_function() {
  if (thisMainLoopMillis - lastTime_COMM_GET_VALUES > 20 && !blePassMode) {
    //    lastTime_COMM_GET_VALUES = thisMainLoopMillis;
    UART.requestVescGetValues();

#ifdef DEBUG
    Serial.println("request data manually");
    Serial.println(lastTime_COMM_GET_VALUES);
#endif

  }

  if (blePassMode) {
    if (blePassModeFirstTime) {
      blePassModeFirstTime = false;
      setColor(100, 5, 5);
      pixels.show();
    }
  } else {
    blePassModeFirstTime = true;
  }

  check_brake();

}


void check_brake(void) {

  if (UART.data.rpm <= 0) {
    brakeState = BRAKE_REVERSE;
  }
  // BRAKE_DECELERATION
  else if (UART.data.avgMotorCurrent < 0) {
    brakeState = BRAKE_DECELERATION;
  }

  // NEUTRAL
  else if (UART.data.avgMotorCurrent == 0 && UART.data.rpm > 0 ) {
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
  int rpm_mapped = map(UART.data.rpm, 0, 20000, 0, 255);
  if (rpm_mapped < 0) {
    rpm_mapped  = 0;
  }

  if (brakeState != before_brake_status) {
    ledCount = 0;
  }

  switch (brakeState) {

    case BRAKE_DECELERATION:

      if (ledCount++ < 5) {
        setColor(255, 0, 0);

      } else if (ledCount++ < 25) {
        setColor(50, 0, 0);
      } else {
        ledCount = 0;
      }

      break;

    case NEUTRAL:

      //      if (ledCount++ < 125) {
      //        setColor(3, 0, 0);
      //      } else if (ledCount++ < 130) {
      //        setColor(30, 30, 30);
      //      } else {
      //        ledCount = 0;
      //      }
      setColor(0, rpm_mapped / 2, rpm_mapped);


      break;

    case ACCELERATION:
      setColor(0, rpm_mapped, rpm_mapped);
      break;


    case BRAKE_REVERSE:
      if (ledCount++ < 150) {
        setColor(ledCount, 0, 0);
      } else if (ledCount++ < 300) {
        setColor(300 - ledCount, 0, 0);
      } else {
        ledCount = 0;
      }

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

void enterBLEModeLED() {
}
