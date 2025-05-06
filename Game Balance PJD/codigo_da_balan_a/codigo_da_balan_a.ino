#include <HX711_ADC.h> // Inclui a biblioteca para interagir com o chip HX711 (interface para células de carga)
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h> // Inclui a biblioteca para usar a memória EEPROM (para armazenar dados persistentes) em certas placas
#endif


// Definição dos pinos conectados aos módulos HX711
// Pino DOUT (Data Output) do HX711 conectado ao pino digital do microcontrolador
// Pino SCK (Serial Clock) do HX711 conectado ao pino digital do microcontrolador
// Para a primeira célula de carga (A):
const int HX711_dout = 4; // Pino DOUT do HX711_1 conectado ao pino 4 do microcontrolador
const int HX711_sck = 5;  // Pino SCK do HX711_1 conectado ao pino 5 do microcontrolador
// Para a segunda célula de carga (B):
const int HX711_dout_2 = 8; // Pino DOUT do HX711_2 conectado ao pino 8 do microcontrolador
const int HX711_sck_2 = 9;  // Pino SCK do HX711_2 conectado ao pino 9 do microcontrolador

// Criação de objetos da classe HX711_ADC para cada célula de carga
HX711_ADC LoadCell_1(HX711_dout, HX711_sck);   // Objeto para a célula de carga conectada aos pinos 4 e 5
HX711_ADC LoadCell_2(HX711_dout_2, HX711_sck_2); // Objeto para a célula de carga conectada aos pinos 8 e 9

const int calVal_eepromAdress = 0; // Define o endereço na EEPROM onde o valor de calibração será armazenado (inicia no endereço 0)
unsigned long t1 = 0; // Variável para armazenar o tempo da última leitura/impressão da célula de carga 1
unsigned long t2 = 0; // Variável para armazenar o tempo da última leitura/impressão da célula de carga 2


void setup() {
  Serial.begin(9600); // Inicializa a comunicação serial com uma taxa de 9600 bauds
  delay(10);          // Pequeno atraso para estabilizar a comunicação serial
  Serial.println();
  Serial.println("Starting..."); // Imprime uma mensagem de inicialização no monitor serial

  LoadCell_1.begin(); // Inicializa a comunicação com o HX711_1
  LoadCell_2.begin(); // Inicializa a comunicação com o HX711_2
  // LoadCell_1.setReverseOutput(); // Descomente para inverter o sinal de saída da célula de carga 1 (se os valores estiverem negativos quando deveriam ser positivos)
  unsigned long stabilizingtime = 2000; // Define um tempo de estabilização de 2000 milissegundos (2 segundos) para leituras mais precisas após a energização
  boolean _tare = true; // Define uma variável para indicar se a tara (zeragem) deve ser feita na inicialização

  LoadCell_1.start(stabilizingtime, _tare); // Inicia a leitura da célula de carga 1 com tempo de estabilização e tara inicial
  LoadCell_2.start(stabilizingtime, _tare); // Inicia a leitura da célula de carga 2 com tempo de estabilização e tara inicial
  // Verifica se houve timeout durante a tara ou se não houve sinal de alguma das células de carga
  if ((LoadCell_1.getTareTimeoutFlag() || LoadCell_1.getSignalTimeoutFlag()) && (LoadCell_2.getTareTimeoutFlag() || LoadCell_2.getSignalTimeoutFlag()) ){
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations"); // Mensagem de erro se houver timeout
    while (1); // Entra em um loop infinito para parar a execução do programa
  }
  else {
    LoadCell_1.setCalFactor(1.0); // Define o fator de calibração inicial para a célula de carga 1 (será ajustado na calibração)
    LoadCell_2.setCalFactor(1.0); // Define o fator de calibração inicial para a célula de carga 2
    Serial.println("Startup is complete"); // Mensagem de conclusão da inicialização
  }
  while (!LoadCell_1.update()); // Aguarda até que a primeira leitura da célula de carga 1 esteja disponível
  Serial.println("LoadCell_1 ok"); // Informa que a célula de carga 1 está funcionando
  while (!LoadCell_2.update()); // Aguarda até que a primeira leitura da célula de carga 2 esteja disponível
  Serial.println("LoadCell_2 ok"); // Informa que a célula de carga 2 está funcionando
  calibrate(); // Chama a função de calibração para ajustar os fatores de calibração das células de carga
}


void loop() {
  static boolean newDataReady_1 = 0; // Variável estática para indicar se novos dados estão prontos da célula de carga 1
  static boolean newDataReady_2 = 0; // Variável estática para indicar se novos dados estão prontos da célula de carga 2
  const int serialPrintInterval = 0; // Define o intervalo (em milissegundos) para imprimir os dados na porta serial (0 para imprimir o mais rápido possível)

  // Verifica se novos dados estão disponíveis e inicia a próxima conversão para cada célula de carga
  if (LoadCell_1.update()) newDataReady_1 = true;

  // Se novos dados estiverem prontos para a célula de carga 1
  if (newDataReady_1) {
    // Verifica se o intervalo de impressão serial expirou
    if (millis() > t1 + serialPrintInterval) {
     float i = LoadCell_1.getData(); // Obtém o valor suavizado da célula de carga 1
     Serial.print("Load_cell_1 output val: ");
      Serial.print(i);
      Serial.print(" : ");
      newDataReady_1 = 0; // Reseta a flag de novos dados
      t1 = millis();      // Atualiza o tempo da última impressão
    }
  }
  if (LoadCell_2.update()) newDataReady_2 = true; // Verifica se novos dados estão disponíveis para a célula de carga 2

  // Se novos dados estiverem prontos para a célula de carga 2
  if (newDataReady_2) {
    // Verifica se o intervalo de impressão serial expirou
    if (millis() > t2 + serialPrintInterval) {
      float i = LoadCell_2.getData(); // Obtém o valor suavizado da célula de carga 2
     Serial.print("Load_cell_2 output val: ");
      Serial.println(i);
      newDataReady_2 = 0; // Reseta a flag de novos dados
      t2 = millis();      // Atualiza o tempo da última impressão
    }
  }
 if(newDataReady_1 && newDataReady_2){
   // Faça a comparação entre os dados das duas células de carga aqui dentro
}
 // Lógica para determinar o estado com base nos valores das células de carga
 // Se o valor absoluto da soma das leituras for menor que 59.4, imprime "E"
 //if(abs(LoadCell_1.getData() + LoadCell_2.getData()) < 59.4){
 //  Serial.println ("E");
 //}
 else {
   // Se o valor da célula de carga 1 for maior que o da célula de carga 2, imprime "A"
 if(LoadCell_1.getData() > LoadCell_2.getData()){
    Serial.println ("A");
   }
   // Se o valor da célula de carga 1 for menor que o da célula de carga 2, imprime "B"
  if(LoadCell_1.getData() < LoadCell_2.getData()){
    Serial.println ("B");
    }
  }

  // Recebe comandos do monitor serial
  if (Serial.available() > 0) {
    char inByte = Serial.read(); // Lê o byte recebido
    if (inByte == 't'){
      LoadCell_1.tareNoDelay();
      LoadCell_2.tareNoDelay(); // Inicia a tara (zeragem) de ambas as células de carga sem bloquear o loop
    }
    else if (inByte == 'r') calibrate(); // Se receber 'r', inicia o processo de calibração
    else if (inByte == 'c') changeSavedCalFactor(); // Se receber 'c', inicia a função para alterar o fator de calibração salvo
  }

 delay(100); // Pequeno atraso
  // Verifica se a última operação de tara da célula de carga 2 foi concluída
  if (LoadCell_2.getTareStatus() == true) {
    Serial.println("Tare complete"); // Informa que a tara foi concluída
  }
}

void calibrate() {
  Serial.println("*");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false; // Flag para controlar o loop de espera da tara
  boolean teste = false;   // Variável booleana (parece não ser usada efetivamente)
  while (_resume == false) {
    LoadCell_1.update(); // Atualiza a leitura da célula de carga 1
    LoadCell_2.update(); // Atualiza a leitura da célula de carga 2
    // Verifica se há dados disponíveis no monitor serial
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read(); // Lê o byte recebido
        if (inByte == 't') {
          LoadCell_1.tareNoDelay(); // Inicia a tara da célula de carga 1
          LoadCell_2.tareNoDelay(); // Inicia a tara da célula de carga 2
          teste= true; // Define a variável teste como true (parece não ter efeito no fluxo)
        }
      }
    }

    // Verifica se a tara da célula de carga 1 foi concluída
    if (LoadCell_1.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true; // Sai do loop de espera da tara
    }
    // Verifica se a tara da célula de carga 2 foi concluída
    if (LoadCell_2.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true; // Sai do loop de espera da tara
    }
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float known_mass = 0; // Variável para armazenar a massa conhecida inserida pelo usuário
  _resume = false;      // Reseta a flag para o próximo loop de espera
  while (_resume == false) {
    LoadCell_1.update(); // Atualiza a leitura da célula de carga 1
    LoadCell_2.update(); // Atualiza a leitura da célula de carga 2
    // Verifica se há dados disponíveis no monitor serial
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat(); // Tenta ler um número de ponto flutuante da entrada serial
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true; // Sai do loop de espera da massa conhecida
      }
    }
  }

  LoadCell_1.refreshDataSet(); // Atualiza o conjunto de dados para garantir que a leitura da massa conhecida seja precisa
  LoadCell_2.refreshDataSet(); // Atualiza o conjunto de dados para garantir que a leitura da massa conhecida seja precisa
  float newCalibrationValue1 = LoadCell_1.getNewCalibration(known_mass); // Calcula o novo fator de calibração para a célula de carga 1
   float newCalibrationValue2 = LoadCell_2.getNewCalibration(known_mass); // Calcula o novo fator de calibração para a célula de carga 2

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue1);
  Serial.print(newCalibrationValue2);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false; // Reseta a flag para o loop de espera da confirmação de salvamento
  while (_resume == false) {
    // Verifica se há dados disponíveis no monitor serial
    if (Serial.available() > 0) {
      char inByte = Serial.read(); // Lê o byte recebido
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512); // Inicializa a EEPROM com um tamanho de 512 bytes para ESP8266 e ESP32
#endif
//        EEPROM.put(calVal_eepromAdress, newCalibrationValue1); // Salva o novo fator de calibração na EEPROM (linha comentada para a primeira célula)
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit(); // Grava as alterações na EEPROM (para ESP8266 e ESP32)
#endif
//        EEPROM.get(calVal_eepromAdress, newCalibrationValue1); // Lê o valor da EEPROM (linha comentada para a primeira célula)
        Serial.print("Value ");
//        Serial.print(newCalibrationValue1); // Imprime o valor salvo (linha comentada para a primeira célula)
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true; // Sai do loop de espera
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true; // Sai do loop de espera
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
  float oldCalibrationValue = LoadCell_2.getCalFactor(); // Obtém o fator de calibração atual da célula de carga 2
  boolean _resume = false; // Flag para controlar o loop de espera da nova calibração
  Serial.println("*");
  Serial.print("Current value is: ");
  Serial.println(oldCalibrationValue);
  Serial.println("Now, send the new value from serial monitor, i.e. 696.0");
  float newCalibrationValue; // Variável para armazenar o novo valor de calibração inserido pelo usuário
  while (_resume == false) {
    // Verifica se há dados disponíveis no monitor serial
    if (Serial.available() > 0) {
      newCalibrationValue = Serial.parseFloat(); // Tenta ler um número de ponto flutuante da entrada serial
      if (newCalibrationValue != 0) {
        Serial.print("New calibration value is: ");
        Serial.println(newCalibrationValue);
        LoadCell_2.setCalFactor(newCalibrationValue); // Define o novo fator de calibração para a célula de carga 2
        _resume = true; // Sai do loop de espera
      }
    }
  }
  _resume = false; // Reseta a flag para o loop de espera da confirmação de salvamento
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");
  while (_resume == false) {
    // Verifica se há dados disponíveis no monitor serial
    if (Serial.available() > 0) {
      char inByte = Serial.read(); // Lê o byte recebido
      if (inByte == 'y') {
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512); // Inicializa a EEPROM
#endif
        EEPROM.put(calVal_eepromAdress, newCalibrationValue); // Salva o novo fator de calibração na EEPROM
#if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit(); // Grava as alterações na EEPROM
#endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue); // Lê o valor da EEPROM (para confirmação)
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true; // Sai do loop de espera
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true; // Sai do loop de espera
      }
    }
  }
}
