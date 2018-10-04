
 int sensorPin1 = A0; // forward sensor
 int sensorPin2 = A2; // left sensor
 int sensorPin3 = A3; // right sensor
 int sensorPin4 = A4; // back sensor
 int sensorValue1, sensorValue2, sensorValue3, sensorValue4 = 0;
 //  forward, left, right, back
 
 setup() {
  Serial.begin(9600);
  Serial3.begin(XBeebaud);
  Serial2.begin(9600);
  Serial3.println("Bumper STARTING");
}

void loop() {
  sensorValue1 = digitalRead(sensorPin1);
  sensorValue2 = digitalRead(sensorPin2);
  sensorValue3 = digitalRead(sensorPin3);
  sensorValue4 = digitalRead(sensorPin4);

  if (sensorValue1 > 2 || sensorValue2 >2  || sensorValue3 > 2 || sensorValue4 >2){
    Serial2.write('S');
    Serial2.write('S');
    Serial2.write('S');
    delay(1000);
    if (sensorValue4 >2){
      Serial2.write('B');
      Serial2.write(3+'0');
      delay(1000);
    }
    if (sensorValue1 > 2){
      Serial2.write('R');
      Serial2.write(3+'0');
      delay(1000);
    }
    else if (sensorValue2 >2){
      Serial2.write('R')
      Serial2.write(3+'0');
      delay(1000);
    }
    else if (sensorValue3 >2){
      Serial2.write('L');
      Serial2.write(3+'0');
      delay(1000);
    }
    else if (sensorValue4 >2){
      Serial2.write('F');
      Serial2.write(3+'0');
      delay(500);
      Serial2.write('L');
      Serial2.write(3 + '0');
      delay(1000);
    }
    Serial2.write('F');
    Serial2.write(3+'0');
    delay(1000);
  }
} 
