#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif


//pins:
//A
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin
//B
const int HX711_dout_2 = 8; //mcu > HX711 dout pin
const int HX711_sck_2 = 9; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell_1(HX711_dout, HX711_sck);// A
HX711_ADC LoadCell_2(HX711_dout_2, HX711_sck_2);//B

const int calVal_eepromAdress = 0;
unsigned long t1 = 0;
unsigned long t2 = 0;


void setup() {
  Serial.begin(9600); delay(10);
  Serial.println();
  Serial.println("Starting...");

  LoadCell_1.begin();
  LoadCell_2.begin();
 // LoadCell_1.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step

  LoadCell_1.start(stabilizingtime, _tare);
  LoadCell_2.start(stabilizingtime, _tare);
  if ((LoadCell_1.getTareTimeoutFlag() || LoadCell_1.getSignalTimeoutFlag()) && (LoadCell_2.getTareTimeoutFlag() || LoadCell_2.getSignalTimeoutFlag()) ){
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell_1.setCalFactor(1.0); 
    LoadCell_2.setCalFactor(1.0); // valor de calibração definido pelo usuário (flutuante), o valor inicial 1,0 pode ser usado para este esboço
    Serial.println("Startup is complete");
  }
  while (!LoadCell_1.update());
  Serial.println("LoadCell_1 ok");
  while (!LoadCell_2.update());
  Serial.println("LoadCell_2 ok");
  calibrate(); //iniciar procedimento de calibração
}


void loop() {
  static boolean newDataReady_1 = 0;
  static boolean newDataReady_2 = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell_1.update()) newDataReady_1 = true;

  // get smoothed value from the dataset:
  if (newDataReady_1) {
    if (millis() > t1 + serialPrintInterval) {
     float i = LoadCell_1.getData();
     Serial.print("Load_cell_1 output val: ");
      Serial.print(i);
      Serial.print(" : ");
      newDataReady_1 = 0;
      t1 = millis();
    }
  }
  if (LoadCell_2.update()) newDataReady_2 = true;

  if (newDataReady_2) {
    if (millis() > t2 + serialPrintInterval) {
      float i = LoadCell_2.getData();
     Serial.print("Load_cell_2 output val: ");
      Serial.println(i);
      newDataReady_2 = 0;
      t2 = millis();
    }
  }
 if(newDataReady_1 && newDataReady_2){
   //faca a comparacao aqui dentro 
}
 //if(abs(LoadCell_1.getData() + LoadCell_2.getData()) < 59.4){
   //Serial.println ("E");
 //}
 else {
 if(LoadCell_1.getData() > LoadCell_2.getData()){
    Serial.println ("A"); 
   }
  if(LoadCell_1.getData() < LoadCell_2.getData()){
    Serial.println ("B");
    }
  }
    
  // receive command from serial terminal
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't'){
      LoadCell_1.tareNoDelay(); 
      LoadCell_2.tareNoDelay(); //tare
    }
    else if (inByte == 'r') calibrate(); //calibrate
    else if (inByte == 'c') changeSavedCalFactor(); //edit calibration value manually
  }

 delay(100);
  // verifique se a última operação de tara foi concluída
  if (LoadCell_2.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

}

void calibrate() {
  Serial.println("*");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false;
  boolean teste = false;
  while (_resume == false) {
    LoadCell_1.update();
    LoadCell_2.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') {
          LoadCell_1.tareNoDelay();
          LoadCell_2.tareNoDelay();
          teste= true;
        }
      }
    }
    
    if (LoadCell_1.getTareStatus() == true) { //conferir que a tara está ok
      Serial.println("Tare complete");

      
      _resume = true;
    }
    if (LoadCell_2.getTareStatus() == true) { //conferir que a tara está ok
      Serial.println("Tare complete");

      
      _resume = true;
    }
    
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell_1.update();
    LoadCell_2.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell_1.refreshDataSet();
  LoadCell_2.refreshDataSet(); //atualize o conjunto de dados para ter certeza de que a massa conhecida foi medida corretamente
  float newCalibrationValue1 = LoadCell_1.getNewCalibration(known_mass); //get the new calibration value
   float newCalibrationValue2 = LoadCell_2.getNewCalibration(known_mass); //get the new calibration value

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue1);
  Serial.print(newCalibrationValue2);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
//        EEPROM.put(calVal_eepromAdress, newCalibrationValue1);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
//        EEPROM.get(calVal_eepromAdress, newCalibrationValue1);
        Serial.print("Value ");
//        Serial.print(newCalibrationValue1);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;

      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }

  Serial.println("End calibration");
  Serial.println("*");
  Serial.println("To re-calibrate, send 'r' from serial monitor.");
  Serial.println("For manual edit of the calibration value, send 'c' from serial monitor.");
  Serial.println("*");
}


void changeSavedCalFactor() {
  float oldCalibrationValue = LoadCell_2.getCalFactor();
  boolean _resume = false;
  Serial.println("*");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  float newCalibrationValue;
  while (_resume == false) {
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat();
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell_2.setCalFactor(newCalibrationValue);
        _resume = true;
      }
    }
  }
  _resume = false;
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue);
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }
  Serial.println("End change calibration value");
  Serial.println("*");
}
