  #include <WiFi.h>
  #include <PubSubClient.h>
  #include <string.h>

  byte led = 32;
  byte led2 = 2;
  byte ldr = 35;
  byte temp = 34;

  // WiFi
  const char *ssid = ""; // Enter your WiFi name
  const char *password = "";  // Enter WiFi password

  // MQTT Broker
  const char *mqtt_broker = "broker.emqx.io";
  const char *topic = "/botao";
  const char *topic2 = "/luminosidade";
  const char *topic3 = "/temperatura";
  const char *mqtt_username = "you_username";
  const char *mqtt_password = "you_password";
  const int mqtt_port = 1883;

  const int ledChannel = 0;
  const int ledResolution = 8;

  const int termistorPin = temp;  // Pino analógico onde o termistor está conectado
  const float termistorNominal = 10000;  // Resistência nominal do termistor
  const float tempNominal = 25.0;  // Temperatura nominal do termistor em graus Celsius
  const float beta = 3950;

  WiFiClient espClient;
  PubSubClient client(espClient);

  void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(ldr, INPUT);
  pinMode(temp, INPUT);
  // connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
      String client_id = "esp32-client-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("Public emqx mqtt broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  }
  // publish and subscribe
  String ldrValue = String(analogRead(ldr));
  client.publish(topic2, ldrValue.c_str());
  client.subscribe(topic);
  String tempValue = String(analogRead(temp));
  client.publish(topic3, tempValue.c_str());
  client.subscribe(topic);
  // Configure LED PWM
  ledcSetup(ledChannel, 5000, ledResolution);
  ledcAttachPin(led, ledChannel);
  }

  void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    int valorldr;
    int luminosidade;
    
    valorldr = analogRead(ldr);
    luminosidade = map(valorldr, 0, 1023, 0, 255);
    Serial.print("Valor lido do LDR : ");
    Serial.print(valorldr);
    Serial.print(" ** Luminosidade : ");
    Serial.println(luminosidade);
    // analogWrite(led, luminosidade);
    
    String message = "";
    for (int i = 0; i < length; i++) {
      message += (char)payload[i];
    }
    Serial.println(message);
    Serial.println();
    Serial.println("-----------------------");
    
    // if (message.equals("ligar") || message.equals("1")) {
    //   digitalWrite(led2, HIGH);
    //   Serial.println("LED ligado");
    // } else if (message.equals("desligar") || message.equals("0")) {
    //   digitalWrite(led2, LOW);
    //   Serial.println("LED desligado");
    // }
  }
  float convertToTemperature(int termistorValue) {
    const float seriesResistor = 10000.0;  // Valor da resistência em ohms
    const float termistorNominal = 10000.0;  // Valor nominal da resistência do termistor em ohms
    const float termistorCoefficient = 3950.0;  // Coeficiente do termistor
    const float termistorTemperature = 25.0;  // Temperatura nominal do termistor em graus Celsius
    float voltage = termistorValue * (3.3 / 4095.0);  // Tensão medida pelo ADC
    float resistance = (3.3 * seriesResistor / voltage) - seriesResistor;  // Resistência do termistor

    float temperature = (resistance - termistorNominal) / termistorCoefficient;  // Aproximação linear
    temperature += termistorTemperature;  // Offset de temperatura

    return temperature;
  }


  void loop() {

    client.loop(); // Manter a conexão com o MQTT Broker ativa

    // Ler valor do sensor LDR
    int ldrValue = analogRead(ldr);
    int tempValue = analogRead(temp);

    // Converter o valor para porcentagem
    int ldrPercentage = map(ldrValue, 0, 4095, 100, 0);

    // Ajustar o brilho do LED de acordo com a porcentagem
    int ledBrightness = map(ldrPercentage, 100, 0, 0, pow(2, ledResolution) - 1);
    ledcWrite(ledChannel, ledBrightness);

    // Publicar valor em porcentagem no tópico
    String ldrPercentageStr = String(ldrPercentage);
    client.publish(topic2, ldrPercentageStr.c_str());

    float temperature = convertToTemperature(tempValue);
    String temperatureStr = String(temperature);
    client.publish(topic3, temperatureStr.c_str());

    delay(1000); // Aguardar 1 segundo antes de enviar o próximo valor

  }