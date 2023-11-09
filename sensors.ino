#include <DHT.h>
#include <DHT_U.h>
#include <FirebaseESP32.h>
#include <WiFi.h>

// #define WIFI_SSID "untan"
// #define WIFI_PASSWORD ""
#define WIFI_SSID "dims"
#define WIFI_PASSWORD "kata sandi"
#define FIREBASE_HOST "deteksi-dini-kebakaran-49169-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyDGXyuMff5dbI7te0X4IuyfcQfCsomjZp8"
#define DHTPIN 5
#define DHTTYPE DHT22
#define MAXTEMP 40

#define m -0.44953  // Gradien garis grafik
#define b 1.23257   // Titik perpotongan sumbu y grafik
#define Ro 4.73    // Resistansi sensor pada udara bersih

DHT dht(DHTPIN, DHTTYPE);

int fire_digital = 4;
int smoke_analog = 32;
int buzzer = 15;
int water_pump = 19;
int batas = 300;

FirebaseData firebaseData;

void setup() {
  Serial.begin(9600);
  pinMode(fire_digital, INPUT); 
  pinMode(smoke_analog, INPUT); 
  pinMode(buzzer, OUTPUT);
  pinMode(water_pump, OUTPUT);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi. IP Address: ");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop() {
  int nilai_api = digitalRead(fire_digital);
  int nilai_gas = analogRead(smoke_analog);
  float suhu = dht.readTemperature();

  Serial.print("Api: ");
  Serial.println(nilai_api);

  // Kalibrasi sensor gas
  float analogValue = analogRead(smoke_analog);
  float Rs = (5.0 * (4095.0 - analogValue)) / analogValue;
  float ratio = Rs / Ro;
  float gas_ppm = pow(10, ((log10(ratio) - b) / m));

  Serial.print("Gas: ");
  Serial.print(gas_ppm);
  Serial.println(" PPM");

  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.println("°C");

  // Kondisi sensor api
  if (nilai_api == LOW) {
    Serial.println("Ada api");
    digitalWrite(buzzer, HIGH);
    digitalWrite(water_pump, LOW);
    Firebase.setString(firebaseData, "/Sensors/Sensor Api/Kondisi", "Api Terdeteksi");
  }

  // Kondisi sensor gas
  else if (gas_ppm >= batas && nilai_api == HIGH) {
    Serial.println("Ada gas");
    digitalWrite(buzzer, HIGH);
    Firebase.setString(firebaseData, "/Sensors/Sensor Gas/Kondisi", "Gas Berlebih");
  }

  // Kondisi sensor suhu
  else if (suhu >= MAXTEMP && nilai_api == HIGH && gas_ppm < batas) {
    Serial.println("Suhu tinggi");
    digitalWrite(buzzer, HIGH);
    Firebase.setString(firebaseData, "/Sensors/Sensor Suhu/Kondisi", "Suhu Tinggi");
  }

  else {
    Serial.println("Tidak ada kebakaran");
    Serial.println("============================================================");
    digitalWrite(buzzer, LOW);
    digitalWrite(water_pump, HIGH);
    Firebase.setString(firebaseData, "/Sensors/Sensor Api/Kondisi", "Aman");
    Firebase.setString(firebaseData, "/Sensors/Sensor Gas/Kondisi", "Aman");
    Firebase.setString(firebaseData, "/Sensors/Sensor Suhu/Kondisi", "Aman");
  }

  Firebase.setBool(firebaseData, "/Sensors/Sensor Api/Value", (nilai_api == LOW)); 
  Firebase.setFloatDigits(2);
  Firebase.setFloat(firebaseData, "/Sensors/Sensor Gas/Value", gas_ppm);
  Firebase.setFloat(firebaseData, "/Sensors/Sensor Suhu/Value", suhu);
  delay(500);
}