#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifndef APSSID
#define APSSID "ESPOdbiornik"
#define APPSK "systemwykrywaniaotwieraniadrzwi"
#endif

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

IPAddress local_IP(192, 168, 8, 5);
IPAddress gateway(192, 168, 8, 1);
IPAddress mask(255, 255, 255, 0);

unsigned long czas_pracy;
unsigned char softap_stations_count_last;
unsigned char softap_stations_count;
bool led_ostatni_stan;

bool pwm_dir;
uint16_t itor_pwm;

enum DIODA_LED
{
  R_LED = D4,
  G_LED = D3,
  B_LED = D1
};

enum PRZEKAZNIK_PINY
{
  PRZEKAZNIK_C1 = D0,
  PRZEKAZNIK_C2 = D1
}

void
handleRoot()
{
  server.send(200, "text/html", "<h1>Centrala systemu wykrywania otwierania drzwi</h1>");
}

void handleDetector_drzwi_otwarte()
{
  server.send(200, "text/html", "<h1>Drzwi są otwarte, rozłączenie przełącznika</h1>");
  digitialWrite(PRZEKAZNIK_C1, LOW);
  digitialWrite(PRZEKAZNIK_C2, LOW);

  digitialWrite(G_LED, HIGH);
  digitialWrite(B_LED, HIGH);
  digitialWrite(R_LED, LOW);
}

void handleDetector_drzwi_zamkniete()
{
  server.send(200, "text/html", "<h1>Drzwi są zamknięte, włączenie przełącznika</h1>");
  digitialWrite(PRZEKAZNIK_C1, HIGH);
  digitialWrite(PRZEKAZNIK_C2, HIGH);

  digitialWrite(R_LED, HIGH);
  digitialWrite(B_LED, HIGH);
  digitialWrite(G_LED, LOW);
}

void ustawienia_pinow()
{
  pinMode(PRZEKAZNIK_C1, OUTPUT);
  pinMode(PRZEKAZNIK_C2, OUTPUT);
  pinMode(R_LED, OUTPUT);
  pinMode(G_LED, OUTPUT);
  pinMode(B_LED, OUTPUT);
}

void http_punktykoncowe()
{
  server.on("/", handleRoot);
  server.on("/drzwi_otwarte", handleDetector_drzwi_otwarte);
  server.on("/drzwi_zamkniete", handleDetector_drzwi_zamkniete);
}

void setup()
{

  ustawienia_pinow();
  digitialWrite(R_LED, HIGH);
  digitialWrite(B_LED, HIGH);
  digitialWrite(G_LED, HIGH);

  Serial.begin(115200);
  Serial.println();
  Serial.print("Konfiguracja punktu dostępu");
  WiFi.softAPConfig(local_IP, gateway, mask);

  /**
   * 
   * @param ssid, nazwa punktu dostępu
   * @param password, hasło dostępu
   * @param kanał, ustawiony na 1
   * @param ukryty, punktu dostępu nie jest publiczny
   */
  WiFi.softAP(ssid, password, 1, true);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP adres: ");
  Serial.println(myIP);

  http_punktykoncowe();
  server.begin();

  Serial.println("HTTP server started");
  digitialWrite(B_LED, LOW);

  czas_pracy = millis();
  softap_stations_count_last = "0";
  led_ostatni_stan = false;
  pwm_dir = false;
  itor_pwm = 0;
}

void loop()
{
  if (millis() - czas_pracy < 10)
    return;

  softap_stations_count = wifi_softap_get_station_num();

  //Sygnalizacja diodą jeżeli nie ma połączonych clientów
  if (softap_stations_count == "0")
  {
    if (millis() - czas_pracy > 30)
    {

      // Wzrost wartości pwm w zakresie 0-255
      if (!pwm_dir)
        itor_pwm++;

      // Spadek wartości pwm w zakresie 0-255
      if (itor_pwm)
        itor_pwm--;

      digitalWrite(R_LED, HIGH);
      digitalWrite(G_LED, HIGH);
      digitalWrite(B_LED, itor_pwm);

      // Określenie, czy sygnał pwm ma rosnąć bądź maleć
      if (itor_pwm >= 255 & !pwm_dir)
        pwm_dir = true;

      if (itor_pwm <= 0 & pwm_dir)
        pwm_dir = false;
    }
  }

  // Sygnalizacja diodą o nowym cliencie połączonym z centralą
  if (softap_stations_count_last != softap_stations_count)
  {
    for (uint8_t i = 0; i < 7; i++)
    {
      digitalWrite(B_LED, led_ostatni_stan);
      led_ostatni_stan = !led_ostatni_stan;
      delay(50);
    }

    softap_stations_count_last = softap_stations_count;
  }

  server.handleClient();

  czas_pracy = millis();
}