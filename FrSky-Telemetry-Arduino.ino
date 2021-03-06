/*
 * FrSky Telemetry Display for Arduino
 *
 * Main Logic
 * Copyright 2016 by Thomas Buck <xythobuz@xythobuz.de>
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <xythobuz@xythobuz.de> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you
 * think this stuff is worth it, you can buy me a beer in return.   Thomas Buck
 * ----------------------------------------------------------------------------
 */
#include "i2c.h"
#include "oled.h"
#include "frsky.h"
#include "logo.h"
#include "config.h"
#include "beeper.h"
#include "options.h"
#include "debounce.h"
#include "led.h"
#include "menu.h"

FrSky frsky(&Serial);
uint8_t showingLogo = 0;
uint8_t redrawScreen = 0;
int16_t rssiRx = 0;
int16_t rssiTx = 0;
int16_t voltageBattery = 0;
uint8_t analog1 = 0;
uint8_t analog2 = 0;
uint8_t analogs[2] = { 0, 0 };
String userDataString = "None Received";
int16_t noWarnVoltage[MODEL_COUNT];
int16_t warningVoltage[MODEL_COUNT];
int16_t alarmVoltage[MODEL_COUNT];
uint8_t ledBrightness = LED_PWM;
uint8_t currentModel = 0;

// Print battery voltage with dot (-402 -> "-4.02V")
String voltageToString(int16_t voltage) {
    String volt = String(abs(voltage) / 100);
    String fract = String(abs(voltage) % 100);
    String s;
    if (voltage < 0) {
        s += '-';
    }
    s += volt + ".";
    for (int i = 0; i < (2 - fract.length()); i++) {
        s += '0';
    }
    s += fract + "V";
    return s;
}

void drawInfoScreen(uint8_t conf) {
    String state;
    if (conf == 0) {
        state = "OK";
    } else if (conf == 1) {
        state = "Checksum Error";
    } else if (conf == 2) {
        state = "Wrong Version";
    } else if (conf == 3) {
        state = "Invalid Models";
    } else {
        state = "Unknown";
    }

    writeLine(0, "FrSky Telemetry");
    writeLine(1, "Version: " + String(versionString));
    writeLine(2, "Patch Level: " + String(PATCH_LEVEL_STRING));
    writeLine(3, "www.xythobuz.de");
    writeLine(4, "EEPROM Config:");
    writeLine(5, state + "!");
    writeLine(6, "Licensing:");
    writeLine(7, "Beer Ware *\\0/*");
}

void dataHandler(uint8_t a1, uint8_t a2, uint8_t q1, uint8_t q2) {
    redrawScreen = 1;

    // Low-Pass Filter for voltages
    analog1 = (((analog1 << BATTERY_FILTER_BETA) - analog1) + a1) >> BATTERY_FILTER_BETA;
    analog2 = (((analog2 << BATTERY_FILTER_BETA) - analog2) + a2) >> BATTERY_FILTER_BETA;

    rssiRx = map(q1, 0, 255, 0, 100);
    rssiTx = map(q2, 0, 255, 0, 100);

    analogs[0] = analog1;
    analogs[1] = analog2;

    voltageBattery = map(analogs[batteryAnalogs[currentModel] - 1],
            batteryValuesMin[currentModel], batteryValuesMax[currentModel],
            batteryVoltagesMin[currentModel], batteryVoltagesMax[currentModel]);
}

void alarmThresholdHandler(FrSky::AlarmThreshold alarm) {
}

void userDataHandler(const uint8_t* buf, uint8_t len) {
    redrawScreen = 1;

    String s;
    for (int i = 0; i < len; i++) {
        s += buf[i];
    }
    userDataString = s;
}

void setup(void) {
    // Initialize with default values (can be changed with EEPROM contents)
    for (int i = 0; i < MODEL_COUNT; i++) {
        noWarnVoltage[i] = batteryMinWarnLevel[i];
        warningVoltage[i] = batteryLowWarnLevel[i];
        alarmVoltage[i] = batteryHighWarnLevel[i];
    }

    // Wait a bit before beeping for the first time, so there's no confusion with
    // the initialization beep of the FrSky TX module
    delay(200);

    // Set output pin directions
    pinMode(BEEPER_OUTPUT, OUTPUT);
    pinMode(LED_OUTPUT, OUTPUT);
    pinMode(S1_INPUT, INPUT);
    pinMode(S2_INPUT, INPUT);

    // Pull ups for push buttons enabled
    digitalWrite(S1_INPUT, HIGH);
    digitalWrite(S2_INPUT, HIGH);

    // Initialization beep
    tone(BEEPER_OUTPUT, INIT_FREQ);

    initLED();
    setLED(ledBrightness);

    // Initialize hardware
    Serial.begin(BAUDRATE);
    i2c_init();
    i2c_OLED_init();
    clear_display();
    delay(50);
    i2c_OLED_send_cmd(0x20);
    i2c_OLED_send_cmd(0x02);
    i2c_OLED_send_cmd(0xA6);

    // Read initial EEPROM config, draw splash screen
    uint8_t ret = readConfig();
    drawInfoScreen(ret);

    // Disable Beeper and LED, wait to show splash screen
    noTone(BEEPER_OUTPUT);
    setLED(0);
    delay(DISPLAY_SHOW_INFO_SCREEN);

    // Show logo and initialize state machine
    drawLogo(bootLogo);
    showingLogo = 1;

#if defined(MODEL_COUNT) && (MODEL_COUNT > 1)
    // Show initial model selection, if multiple models are enabled
    writeLine(2, "Model " + String(currentModel + 1) + " active");
    if (modelName[currentModel].length() > 0) {
        writeLine(3, modelName[currentModel]);
    }

    // Signal main-loop to clear text after a while
    showingLogo = 4;
#endif // defined(MODEL_COUNT) && (MODEL_COUNT > 1)

    // Handler functions receiving the telemetry data
    frsky.setDataHandler(&dataHandler);
    frsky.setAlarmThresholdHandler(&alarmThresholdHandler);
    frsky.setUserDataHandler(&userDataHandler);

    // Second initialization beep and LED blink
    setLED(ledBrightness);
    tone(BEEPER_OUTPUT, INIT_FREQ);
    delay(100);
    noTone(BEEPER_OUTPUT);
    setLED(0);
}

void loop(void) {
    frsky.poll();
    beeperTask();

    if (!showingLogo) {
        // Enable battery alarm beeper as required
        if (voltageBattery > noWarnVoltage[currentModel]) {
            if (voltageBattery > warningVoltage[currentModel]) {
                setBeeper(BEEPER_STATE_OFF);
            } else if (voltageBattery > alarmVoltage[currentModel]) {
                setBeeper(BEEPER_STATE_LOW);
            } else {
                setBeeper(BEEPER_STATE_HIGH);
            }
        } else {
            setBeeper(BEEPER_STATE_OFF);
        }
    }

    static unsigned long lastTime = 0;
    if (redrawScreen && ((millis() - lastTime) > DISPLAY_MAX_UPDATE_RATE)) {
        redrawScreen = 0;
        lastTime = millis();

        // Clear the display removing the logo on first received packet
        if (showingLogo) {
            showingLogo = 0;
            clear_display();
        }

        writeLine(0, "RSSI Rx: " + String(rssiRx) + "%");
        writeLine(1, "RSSI Tx: " + String(rssiTx) + "%");
        writeLine(2, "A1 Value: " + String(analog1));
        writeLine(3, "A2 Value: " + String(analog2));
        writeLine(4, "User Data:");
        writeLine(5, userDataString);
        writeLine(6, "Battery Volt:");
        writeLine(7, voltageToString(voltageBattery));
    } else if ((!redrawScreen) && ((millis() - lastTime) > DISPLAY_REVERT_LOGO_TIME) && (!showingLogo)) {
        // Show the logo again if nothing has been received for a while
        drawLogo(bootLogo);
        showingLogo = 2;

        // Beep twice when we lost the connection
        setBeeper(BEEPER_STATE_LOW);
        lastTime = millis();
    } else if ((showingLogo == 2) && ((millis() - lastTime) > BATTERY_LOW_WARN_ON)) {
        showingLogo = 3;
        setBeeper(BEEPER_STATE_HIGH);
    } else if ((showingLogo == 3) && ((millis() - lastTime) > BATTERY_HIGH_WARN_ON)) {
        // Turn beeper off again after losing connection
        setBeeper(BEEPER_STATE_OFF);
        showingLogo = 1;
    } else if (showingLogo == 4) {
        // New model has been selected, start 'timer' to overwrite text
        lastTime = millis();
        showingLogo = 5;
    } else if ((showingLogo == 5) && ((millis() - lastTime) > DISPLAY_REVERT_LOGO_TIME)) {
        // Reset, overwriting the model select text after a while
        drawLogo(bootLogo);
        showingLogo = 1;
    } else if (showingLogo && (!redrawScreen)) {
        // Only handle menu inputs when we're in 'idle' mode...
        static Debouncer s1(S1_INPUT);
        if (s1.poll()) {
            drawMenu(MENU_NEXT);
        }

        static Debouncer s2(S2_INPUT);
        if (s2.poll()) {
            drawMenu(MENU_OK);
        }
    }
}

