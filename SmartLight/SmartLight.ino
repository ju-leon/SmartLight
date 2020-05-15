#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"


#define LED_PIN D1
#define LED_COUNT 42

const char* SSID = WIFI_SSID;
const char* PSK = PASSWORD;
const char* MQTT_BROKER = BROKER_IP;
 
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
char msg[600];
int value = 0;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int h = 0;
int s = 0;
int v = 0;

void setup() {
  
    strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.show();            // Turn OFF all pixels ASAP
    strip.setBrightness(255); 

    Serial.begin(115200);
    setup_wifi();
    client.setServer(MQTT_BROKER, 1883);
    client.setCallback(callback);
}
 
void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PSK);
 
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());
 
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  String message = String((char*)payload);

  message = message.substring(0, length);
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (strcmp(topic, "openhab/leon/bed/full")==0) {
    setColor(message);
  } else if (strcmp(topic, "openhab/leon/bed/rgb")==0) {

    if(message == "ON") {
      colorWipe(strip.gamma32(strip.ColorHSV(h, s, v)));
    } else if(message == "OFF") {
      colorWipe(strip.Color(0,0,0));
    } else if(message.indexOf(",") == -1) {
      Serial.println("Dimming");
      v = message.toInt();
      v = (v / 100.0) * 255;

      colorWipe(strip.gamma32(strip.ColorHSV(h, s, v)));
      
    } else {
      h = message.substring(0,message.indexOf(',')).toInt();
      h = (h / 360.0) * 65536;
      s = message.substring(message.indexOf(',')+1,message.lastIndexOf(',')).toInt();
      s = (s / 100.0) * 255;
      v = message.substring(message.lastIndexOf(',')+1).toInt();
      v = (v / 100.0) * 255;
  
      Serial.print(h);
      Serial.print(",");
      
      Serial.print(s);
      Serial.print(",");
  
      
      Serial.print(v);
      Serial.println();
      
      colorWipe(strip.gamma32(strip.ColorHSV(h, s, v)));
    }
  } else if(strcmp(topic, "openhab/leon/bed/zone")==0) {
    rainbow(20);
    //markZone(strip.Color(charToHex(payload[0], payload[1]), charToHex(payload[2], payload[3]), charToHex(payload[4], payload[5])),0,20);
  }
}

void setColor(String message) {
  int i = 0;
  while(message != "") {
    String current = message.substring(0,message.indexOf(';'));
    message = message.substring(message.indexOf(';')+1);
    
    int red = current.substring(0,current.indexOf(',')).toInt();
    int green = current.substring(current.indexOf(',')+1,current.lastIndexOf(',')).toInt();
    int blue = current.substring(current.lastIndexOf(',')+1).toInt();

    strip.setPixelColor(i, strip.Color(red,green,blue));
  
    i++;
    
  }

  strip.show();
}

void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}


void markZone(uint32_t color, int from, int to) {
  for(int i=0; i<strip.numPixels() / 2; i++) { 
    if((strip.numPixels() / 2) - i > from && (strip.numPixels() / 2) - i < to) {
      strip.setPixelColor((strip.numPixels() / 2) - i, color);
    } else {
      strip.setPixelColor((strip.numPixels() / 2) - i, strip.Color(0,0,0));
    }
    if((strip.numPixels() / 2) + i > from && (strip.numPixels() / 2) + i < to) {
      strip.setPixelColor((strip.numPixels() / 2) + i, color);
    } else {
      strip.setPixelColor((strip.numPixels() / 2) + i, strip.Color(0,0,0));
    }
    
    strip.show();                
    delay(20);       
  }
  //strip.show();  
}


int charToHex(char a, char b) {
  return ((a - '0') * 16) + (b - '0');
}

void colorWipe(uint32_t color) {
  for(int i=0; i<=strip.numPixels() / 2; i++) { 
    strip.setPixelColor((strip.numPixels() / 2) - i, color);
    strip.setPixelColor((strip.numPixels() / 2) + i, color);   
    strip.show();                
    delay(20);
  }

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "GarageDoor-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("openhab/leon/bed/rgb");
      client.subscribe("openhab/leon/bed/zone");
      client.subscribe("openhab/leon/bed/full");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    //Now will owerflow after 50 days. Should not be an issue.
    unsigned long now = millis();
    if (abs(now - lastMsg) > 2000) {
      lastMsg = now;
      
      snprintf (msg, 50, "Bed light: online, since %ld.000ms", now);
      Serial.print("Publish message: ");
      Serial.println(msg);
      //client.publish("openhab/leon/bed", msg);
      
    }

}
