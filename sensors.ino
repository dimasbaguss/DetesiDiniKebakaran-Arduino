#include <DHT.h>
#include <DHT_U.h>
#include <FirebaseESP32.h>
#include <WiFi.h>

#define WIFI_SSID "Workshop_AP"
#define WIFI_PASSWORD ""
#define FIREBASE_HOST "deteksi-dini-kebakaran-49169-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "AIzaSyDGXyuMff5dbI7te0X4IuyfcQfCsomjZp8"

#define DHTPIN 5
#define DHTTYPE DHT22
#define MAXTEMP 40
#define RL 10       // Resistance Load
#define m -0.44953  // Gradien
#define b 1.23257   // Titik perpotongan
#define Ro 4.51     // Resistansi udara bersih

DHT dht(DHTPIN, DHTTYPE);
int fire_digital = 4;
int smoke_analog = 32;
int buzzer = 15;
int water_pump = 19;
int batas = 400;
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
  float VRL = analogRead(nilai_gas) * (3.3/4095.0);
  float Rs = ((3.3*RL)/VRL)-RL;
  float ratio = Rs/Ro;
  float gas_ppm = pow(10, ((log10(ratio)-b)/m));

  Serial.print("Rs/Ro: ");
  Serial.print(ratio); 
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