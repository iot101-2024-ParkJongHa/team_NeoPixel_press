#include <ConfigPortal32.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

char*               ssid_pfix = (char*)"NeoPixel_FSR";
String              user_config_html = ""
    "<p><input type='text' name='broker' placeholder='MQTT Server'>";

char                mqttServer[100];
const int           mqttPort = 1883;

unsigned long       interval = 10000;
unsigned long       lastPublished = - interval;
 
WiFiClient wifiClient;
PubSubClient client(wifiClient);
void msgCB(char* topic, byte* payload, unsigned int length); // message Callback
void pubStatus();

///////////////////////////////////////////////

int FSRsensor = A0; // A0에 FSR 센서 연결

#define ledPinA 15 
#define ledNumA 8
#define ledPinB 2
#define ledNumB 24
Adafruit_NeoPixel pixelsA(ledNumA, ledPinA, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelsB(ledNumB, ledPinB, NEO_GRB + NEO_KHZ800);

void waterDropA(int i, int R, int G, int B, int maxTail = 4, int tail = 1) {
    int divider = exp(tail);
    pixelsA.setPixelColor(i, pixelsA.Color(R/divider, G/divider, B/divider));
    if (i < ledNumA && tail < maxTail) {
        waterDropA(++i, R, G, B, maxTail, ++tail);
    }
}
void waterDropB(int i, int R, int G, int B, int maxTail = 4, int tail = 1) {
    int divider = exp(tail);
    pixelsB.setPixelColor(i, pixelsB.Color(R/divider, G/divider, B/divider));
    if (i < ledNumB && tail < maxTail) {
        waterDropB(++i, R, G, B, maxTail, ++tail);
    }
}

///////////////////////////////////////////////

void loop() {
    client.loop();

    unsigned long currentMillis = millis();
    if(currentMillis - lastPublished >= interval) {
        lastPublished = currentMillis;
        pubStatus(); // 압력 센서 정보 pub
    }
}

void setup() {
    Serial.begin(115200);

    pixelsA.begin();
    pixelsB.begin();

    loadConfig();
    // *** If no "config" is found or "config" is not "done", run configDevice ***
    if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // main setup
    Serial.printf("\nIP address : "); Serial.println(WiFi.localIP());

    if (cfg.containsKey("broker")) {
            sprintf(mqttServer, (const char*)cfg["broker"]);
    }
    client.setServer(mqttServer, mqttPort);
    client.setCallback(msgCB); // 메시지 callback - neopixel topic sub

    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect("ESP32Relay")) {
            Serial.println("connected");  
        } else {
            Serial.print("failed with state "); Serial.println(client.state());
            delay(2000);
        }
    }
 
    client.subscribe("id/yourname/NeoPixel/cmd"); // 받을 메시지 설정
}

void pubStatus() 
{
    int value;
    char buf[10];

    value= analogRead(FSRsensor);
    Serial.println(value);
    //value = map(value, 0, 1023, 0, 255); 
    sprintf(buf, "%d", value);
    client.publish("id/yourname/sensor/evt/press", buf);
}

void msgCB(char* topic, byte* payload, unsigned int length) 
{
    char msgBuffer[20];
    if(!strcmp(topic, "id/yourname/NeoPixel/cmd")) {
        int i;
        for(i = 0; i < (int)length; i++) {
            msgBuffer[i] = payload[i];
        } 
        msgBuffer[i] = '\0';
        Serial.printf("\n%s -> %s", topic, msgBuffer);
        if(!strcmp(msgBuffer, "box")) 
        {
            int R = random(0, 255);
            int G = random(0, 255);
            int B = random(0, 255);

            for (int i = ledNumB-1; i >= 0 ; i--) 
            {
                pixelsB.clear();
                waterDropB(i, R, G, B);
                pixelsB.show();
                delay(100);
            }
        } 
        else if(!strcmp(msgBuffer, "door")) 
        {
            int R = random(0, 255);
            int G = random(0, 255);
            int B = random(0, 255);

            for (int i = ledNumA-1; i >= 0 ; i--) 
            {
                pixelsA.clear();
                waterDropA(i, R, G, B);
                pixelsA.show();
                delay(100);
            }
        }
    }
    //pubStatus();
}
