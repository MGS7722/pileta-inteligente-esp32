#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_DS18B20   4
#define PIN_RELE      26   // Módulo relé
#define PIN_LED_CALOR 14   // LED verde (calentador ON)
#define PIN_LED_OK    27   // LED rojo  (calentador OFF)

const float TEMP_OBJETIVO = 23.0;  // Bajar para testear — subir a 30 para uso real
const float HISTERESIS    = 5.0;

OneWire            oneWire(PIN_DS18B20);
DallasTemperature  sensores(&oneWire);
LiquidCrystal_I2C  lcd(0x27, 16, 2);

bool calentadorEncendido = false;

void setup() {
    Serial.begin(115200);

    pinMode(PIN_RELE,      OUTPUT);
    pinMode(PIN_LED_CALOR, OUTPUT);
    pinMode(PIN_LED_OK,    OUTPUT);

    digitalWrite(PIN_RELE,      HIGH);  // Relé apagado al arrancar (activo-bajo)
    digitalWrite(PIN_LED_CALOR, LOW);
    digitalWrite(PIN_LED_OK,    HIGH);

    sensores.begin();
    sensores.setResolution(12);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Control Pileta");
    lcd.setCursor(0, 1);
    lcd.print("Iniciando...");
    delay(2000);
    lcd.clear();

    Serial.println("Sistema listo.");
}

void loop() {
    sensores.requestTemperatures();
    float temp = sensores.getTempCByIndex(0);

    if (temp == DEVICE_DISCONNECTED_C) {
        lcd.setCursor(0, 0);
        lcd.print("ERROR SENSOR!   ");
        lcd.setCursor(0, 1);
        lcd.print("Revisar cables  ");
        Serial.println("ERROR: sensor desconectado.");
        delay(2000);
        return;
    }

    // Lógica de control con histéresis
    if (!calentadorEncendido && temp < (TEMP_OBJETIVO - HISTERESIS)) {
        calentadorEncendido = true;
        digitalWrite(PIN_RELE,      LOW);   // Activa el relé
        digitalWrite(PIN_LED_CALOR, HIGH);
        digitalWrite(PIN_LED_OK,    LOW);
        Serial.println(">>> Calentador ENCENDIDO");
    } else if (calentadorEncendido && temp >= TEMP_OBJETIVO) {
        calentadorEncendido = false;
        digitalWrite(PIN_RELE,      HIGH);  // Desactiva el relé
        digitalWrite(PIN_LED_CALOR, LOW);
        digitalWrite(PIN_LED_OK,    HIGH);
        Serial.println(">>> Calentador APAGADO");
    }

    // Mostrar en LCD
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temp, 1);
    lcd.print(" ");
    lcd.print((char)223);
    lcd.print("C    ");

    lcd.setCursor(0, 1);
    lcd.print(calentadorEncendido ? "Calent: ENCENDIDO" : "Calent: APAGADO  ");

    // Mostrar en Serial
    Serial.print("Temp: ");
    Serial.print(temp, 1);
    Serial.print(" C  |  ");
    Serial.println(calentadorEncendido ? "ENCENDIDO" : "APAGADO");

    delay(2000);
}
