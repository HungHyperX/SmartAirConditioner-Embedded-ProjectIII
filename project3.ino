#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <time.h>
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#define DHT_PIN 15 // Chân kết nối cảm biến DHT22
#define BUTTON_UP 25
#define BUTTON_DOWN 32
#define LED_PIN 2

#define WIFI_SSID "Van Thuyen"
#define WIFI_PASSWORD "123456789"
#define AWS_IOT_SHADOW_TOPIC "hunghyper/sensor/data"

String h;
String t;

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Khai báo các đối tượng
DHTesp dhtSensor;
LiquidCrystal_I2C lcd(0x27, 20, 4);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)

#define DEFAULT_TEMP 30
int currentTemperature = DEFAULT_TEMP;
int previousTemperature = -999;

//IRrecv receiver(IRRECEIVE_PIN);
int btn_value = 0;
bool isLCDOn = true; // Biến trạng thái để theo dõi trạng thái màn hình LCD
// Hàm kết nối WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Hàm gọi lại khi có tin nhắn đến từ MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
    //Serial.print((char)payload[i]);
  }
  Serial.println(message);
  // Kiểm tra và xử lý tin nhắn từ topic
  if (String(topic) == "hunghyper/temperature/control") {
    int tempChange = message.toInt(); // Chuyển chuỗi thành số nguyên
    if (tempChange >= 16 && tempChange <= 30) {
      //digitalWrite(2, LOW);   // Bật LED
      blinkLED(200, 1);
      currentTemperature = tempChange;
      Serial.println("Temperature set to: " + String(currentTemperature));
      lcd.clear();
      lcd.print("Temperature: " + String(currentTemperature) + " C");
    } else {
      Serial.println("Invalid temperature value");
    }
  }
  if (String(topic) == "hunghyper/AC/up_down"){
    int up_down_message = message.toInt();
    if (currentTemperature > 16 && up_down_message == 0) {
      currentTemperature--;
      Serial.println("UP");
    }
    if (currentTemperature < 30 && up_down_message == 1){
      currentTemperature++;
      Serial.println("DOWN");
    }
  }
  if (String(topic) == "hunghyper/AC/on_off"){
    int on_off_message = message.toInt();
    blinkLED(200, 1);
    if (on_off_message == 0) {
      lcd.setCursor(0, 0);
      lcd.print("TURN OFF");
      lcd.noBacklight();
      lcd.clear();
      Serial.println("OFF");
      isLCDOn = false;
    }else{
      
      lcd.backlight();
      lcd.print("TURN ON");
      lcd.clear();
      lcd.print("Temperature: " + String(currentTemperature) + " C");
      Serial.println("ON");
      isLCDOn = true;
    }
  }
}

void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  Serial.println("Connecting to AWS IoT");
  client.setCallback(callback);
 
  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  // Subscribe to a topic
  if (client.subscribe("hunghyper/AC/on_off")) {
    Serial.print("Subscribed to topic: ");

  } else {
    Serial.print("Failed to subscribe to topic: ");
   
  }
  if (client.subscribe("hunghyper/temperature/control")) {
    Serial.print("Subscribed to topic: ");
  } else {
    Serial.print("Failed to subscribe to topic: ");
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  Serial.println("AWS IoT Connected!");
}

// Hàm khôi phục kết nối MQTT
// void reconnect() {
//   while (!client.connected()) {
//     Serial.print("Attempting MQTT connection...");
//     String clientId = "ESP32Client-";
//     clientId += String(random(0xffff), HEX);
//     if (client.connect(clientId.c_str())) {
//       Serial.println("Connected");
//       client.subscribe("hunghyper/temperature/control");
//       client.subscribe("hunghyper/AC/up_down");
//       client.subscribe("hunghyper/AC/on_off");
//       Serial.println("Subscribed to topic: hunghyper/temperature/control");
//       Serial.println("Subscribed to topic: hunghyper/AC/up_down");
//       Serial.println("Subscribed to topic: hunghyper/AC/on_off");
//     } else {
//       Serial.print("failed, rc=");
//       Serial.print(client.state());
//       Serial.println(" try again in 5 seconds");
//       delay(5000);
//     }
//   }
// }

// Hàm khởi tạo
void setup() {
  pinMode(LED_PIN, OUTPUT); // Khởi tạo chân LED
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  //Serial.begin(115200);
  Serial.begin(9600);
  //receiver.enableIRIn();
  setup_wifi();
  //client.setServer(MQTT_SERVER, 1883);
  //client.setCallback(callback);

  client.subscribe("hunghyper/temperature/control"); // Đăng ký topic để nhận lệnh
  client.publish("hunghyper/temperature/control", "Testing from ESP32");

  dhtSensor.setup(DHT_PIN, DHTesp::DHT11);
  connectAWS();
  // Cấu hình NTP để lấy thời gian từ máy chủ
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // GMT+7 cho giờ Việt Nam

  // Kiểm tra thời gian ban đầu
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time");
  } else {
    Serial.println("Time synchronized successfully!");
  }

  lcd.init();
  lcd.backlight();
  lcd.print("System Starting...");
  delay(2000);
  lcd.clear();
}

void loop() {
  // if (!client.connected()) {
  //   reconnect();
  // }
  if (!client.connected()) {
    connectAWS();
  }
  client.loop();

  // Kiểm tra trạng thái các nút nhấn
  if (digitalRead(BUTTON_UP) == LOW ) {
    if (currentTemperature < 30)
      currentTemperature++; // Tăng nhiệt độ
    Serial.println("Increased temperature: " + String(currentTemperature));
    delay(200); // Tránh kích hoạt nhiều lần khi giữ nút
  }

  if (digitalRead(BUTTON_DOWN) == LOW) {
    if (currentTemperature > 16)
      currentTemperature--; // Giảm nhiệt độ
    Serial.println("Decreased temperature: " + String(currentTemperature));
    delay(200); // Tránh kích hoạt nhiều lần khi giữ nút
  }

  if (currentTemperature != previousTemperature) {
    lcd.clear();
    lcd.print("Temperature: " + String(currentTemperature) + " C");
    blinkLED(300, 1);
    // Cập nhật giá trị nhiệt độ trước đó
    previousTemperature = currentTemperature;
  }

  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    // Đọc dữ liệu từ cảm biến
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    t = String(data.temperature, 2);
    h = String(data.humidity, 1);
    

    // Lấy thời gian hiện tại
    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) {
      String currentTime = String(timeInfo.tm_year + 1900) + "-" +
                           String(timeInfo.tm_mon + 1) + "-" +
                           String(timeInfo.tm_mday) + " " +
                           String(timeInfo.tm_hour) + ":" +
                           String(timeInfo.tm_min) + ":" +
                           String(timeInfo.tm_sec);

      StaticJsonDocument<256> doc;
      doc["temperature"] = t;
      doc["humidity"] = h;
      doc["timestamp"] = currentTime;

      char shadowUpdatePayload[256];
      serializeJson(doc, shadowUpdatePayload);

      // Tạo chuỗi JSON
      String payload = "{";
      payload += "\"temperature\":" + t + ","; // Dữ liệu từ cảm biến
      //payload += "\"manualTemp\":" + String(currentTemperature) + ",";  // Giá trị nhiệt độ chỉnh tay
      payload += "\"humidity\":" + h + ",";
      payload += "\"timestamp\":\"" + currentTime + "\"";
      payload += "}";

      // In và gửi chuỗi JSON
      Serial.print("Publishing message: ");
      Serial.println(payload);
      client.publish("hunghyper/sensor/data", payload.c_str()); // Gửi JSON qua topic duy nhất

      // Publish to the Thing Shadow update topic
      //client.publish(AWS_IOT_SHADOW_TOPIC, shadowUpdatePayload);
    } else {
      Serial.println("Failed to obtain time");
    }
  }
}

void blinkLED(int delayTime, int blinkCount) {
  for (int i = 0; i < blinkCount; i++) {
    digitalWrite(LED_PIN, HIGH);  // Bật LED
    delay(delayTime);             // Chờ một khoảng thời gian
    digitalWrite(LED_PIN, LOW);   // Tắt LED
    delay(delayTime);             // Chờ một khoảng thời gian
  }
}


