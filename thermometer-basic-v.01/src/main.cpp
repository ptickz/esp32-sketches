#include <Arduino.h>
#include <DHT.h>

constexpr uint8_t DHT_PIN = 15;
constexpr uint8_t DHT_SENSOR_TYPE = DHT11;
DHT dht_sensor(DHT_PIN, DHT_SENSOR_TYPE);

void setup()
{
    Serial.begin(115200);
    dht_sensor.begin();
}

void loop()
{
    float humi = dht_sensor.readHumidity();
    float tempC = dht_sensor.readTemperature();

    if (isnan(tempC) || isnan(humi))
    {
        Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
        Serial.print("Humidity: ");
        Serial.print(humi);
        Serial.print("%");

        Serial.print("  |  ");

        Serial.print("Temperature: ");
        Serial.print(tempC);
        Serial.print("°C");
        Serial.print('\n');
    }

    delay(2000);
}