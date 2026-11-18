#include <Arduino.h>
#include <RPC.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <HTInfraredSeeker.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

// =====================================================
// BNO055
// =====================================================

Adafruit_BNO055 bno = Adafruit_BNO055(55);

// IMUPLUS usa giroscopio + acelerometro, SEM magnetometro.
// Em robos com motores/ESC proximos, evita saltos de rumo por interferencia
// magnetica. O yaw continua RELATIVO e pode sofrer drift com o tempo.
// Para comparar, troque por OPERATION_MODE_NDOF e ajuste a calibracao.
const adafruit_bno055_opmode_t MODO_BNO = OPERATION_MODE_IMUPLUS;
const unsigned long BNO_INTERVALO_MS = 10;
const unsigned long BNO_DIAGNOSTICO_MS = 500;
const unsigned long BNO_MAX_IDADE_MS = 250;
const bool DEBUG_BNO = true;

bool bnoCalibrado = false;
bool bnoValido = false;
bool bnoPrecisaZerar = false;
bool bnoTemLeitura = false;
uint8_t bnoSaltosConsecutivos = 0;
float anguloRobo = 0.0f;
float yawBruto = 0.0f;
float offsetOrientacao = 0.0f;
unsigned long ultimaLeituraBNO = 0;
unsigned long ultimaTentativaBNO = 0;
bool calibrar = true;

// Angulos normalizados ao intervalo (-180, +180].
float normalizarAngulo(float angulo) {
  while (angulo > 180.0f) angulo -= 360.0f;
  while (angulo <= -180.0f) angulo += 360.0f;
  return angulo;
}

// =====================================================
// DISPLAY OLED I2C
// =====================================================

#define OLED_LARGURA 128
#define OLED_ALTURA 64
#define OLED_RESET -1
#define OLED_ENDERECO 0x3C

Adafruit_SSD1306 display(OLED_LARGURA, OLED_ALTURA, &Wire, OLED_RESET);
bool oledDisponivel = false;

// =====================================================
// IR SEEKER
// =====================================================

int ballDirecao = 0;
int ballIntens = 0;

// =====================================================
// DRIBBLER BRUSHLESS (ESC)
// =====================================================

#define PIN_ESC 8
#define ESC_MIN 1000
#define ESC_MAX 2000

Servo esc;

// Nivel interno: 1 = 20%, 2 = 30%, ... 9 = 100%.
// Valor inicial solicitado: nivel 2 (30%).

int pulsoAtualDribbler = ESC_MIN;

// =====================================================
// MOTORES
// =====================================================

#define M1_IN1  45
#define M1_IN2  43
#define M1_PWM  6

#define M2_IN1  47 
#define M2_IN2  49
#define M2_PWM  5

#define M3_IN1  39
#define M3_IN2  41
#define M3_PWM  7

#define M4_IN1  53
#define M4_IN2  51
#define M4_PWM  4

#define MOTOR1_DIR 1
#define MOTOR2_DIR 1
#define MOTOR3_DIR 1
#define MOTOR4_DIR 1

#define COMPENSACAO_M4 1.0

// frente, tras, esquerda, direita

#define TRIG1 27
#define ECHO1 29

#define TRIG2 35
#define ECHO2 37

#define TRIG3 23
#define ECHO3 25

#define TRIG4 31
#define ECHO4 33

// =====================================================
// BOTÃO
// =====================================================

#define botao 55
#define botao2 57

#define switchsensor 13

// =====================================================
// TSOPS
// =====================================================

const int NUM_TSOPS = 16;


const int TSOP_TEMPO_JANELA = 50;

// Estado mais recente de cada TSOP e decisao calculada pelo conjunto.
bool tsopDetectado[NUM_TSOPS] = {false};
int direcaoFinalTSOP = -1;

// =====================================================
// VELOCIDADES
// =====================================================


// =====================================================
// REGISTRO DE DEBUG
// =====================================================

String ultimaAcao = "";
bool correcaoOrientacaoAtiva = false;

void registrarAcao(String acao) {
  if (acao != ultimaAcao) {
    Serial.print("[MOVIMENTO] ");
    Serial.println(acao);
    ultimaAcao = acao;
  }
}

// =====================================================
// CONTROLE DO DRIBBLER
// =====================================================

// Aciona o dribbler em um nivel especifico:
// 1 = 20% (1200 us), 2 = 30% (1300 us), ... 9 = 100% (2000 us).


// Aciona usando o nivel definido na variavel velocidadeDribbler.


// Envia o pulso minimo ao ESC: dribbler parado.
void pararDribbler() {
  // Nao congestiona a Serial quando o botao permanece solto.
  if (pulsoAtualDribbler != ESC_MIN) {
    pulsoAtualDribbler = ESC_MIN;
    esc.writeMicroseconds(ESC_MIN);
    Serial.println("[DRIBBLER] Parado");
  }
}

// =====================================================
// CONTROLE INDIVIDUAL DOS MOTORES
// =====================================================

void motorWrite(int in1, int in2, int pwmPin, int speedValue) {
  speedValue = constrain(speedValue, -255, 255);

  if (speedValue > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, speedValue);
  }
  else if (speedValue < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    analogWrite(pwmPin, -speedValue);
  }
  else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    analogWrite(pwmPin, 0);
  }
}

// =====================================================
// SET MOTOR
// =====================================================

void setMotor(int motor, int speedValue) {
  switch(motor){
    case 1:
      motorWrite(M1_IN1, M1_IN2, M1_PWM,
                 speedValue * MOTOR1_DIR);
      break;

    case 2:
      motorWrite(M2_IN1, M2_IN2, M2_PWM,
                 speedValue * MOTOR2_DIR);
      break;

    case 3:
      motorWrite(M3_IN1, M3_IN2, M3_PWM,
                 speedValue * MOTOR3_DIR);
      break;

    case 4:
      motorWrite(M4_IN1, M4_IN2, M4_PWM,
                 speedValue * MOTOR4_DIR);
      break;
  }
}

// =====================================================
// PARAR
// =====================================================

void parar() {
  registrarAcao("ROBO PARADO");

  setMotor(1, 0);
  setMotor(2, 0);
  setMotor(3, 0);
  setMotor(4, 0);
}

// =====================================================
// MOVIMENTOS
// =====================================================

// =====================================================
// ROTAÇÃO NO PRÓPRIO EIXO
// dir = +1 / -1
// =====================================================

// =====================================================
// BNO055 - CALIBRAÇÃO
// =====================================================

void mostrarStatusCalibracao(uint8_t sys, uint8_t gyro,
                              uint8_t accel, uint8_t mag) {
  if (!oledDisponivel) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("CALIBRANDO BNO055");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 15);
  display.print("SYS:   "); display.print(sys); display.println("/3");

  display.setCursor(0, 26);
  display.print("GYRO:  "); display.print(gyro); display.println("/3");

  display.setCursor(0, 37);
  display.print("ACCEL: "); display.print(accel); display.println("/3");

  display.setCursor(0, 48);
  display.print("MAG:   "); display.print(mag); display.println("/3");

  display.display();
}

void bno_calibrar() {
  Serial.println("[BNO055] Deixe o robo IMOVEL ate GYRO = 3/3.");
  Serial.println("[BNO055] IMUPLUS nao exige calibracao MAG.");

  uint8_t sys = 0, gyro = 0, accel = 0, mag = 0;
  while (true) {
    bno.getCalibration(&sys, &gyro, &accel, &mag);

    Serial.print("[CALIBRACAO] SYS: "); Serial.print(sys);
    Serial.print(" | GYRO: "); Serial.print(gyro);
    Serial.print(" | ACCEL: "); Serial.print(accel);
    Serial.print(" | MAG: "); Serial.println(mag);
    mostrarStatusCalibracao(sys, gyro, accel, mag);

    // A orientacao em yaw depende principalmente do giroscopio.
    // Em IMUPLUS, MAG fica em 0 e nao deve bloquear o setup.
    // Se usar NDOF no futuro, calibra tambem o magnetometro.
    bool ok = (gyro == 3);
    if (MODO_BNO == OPERATION_MODE_NDOF) ok = ok && (mag >= 2);

    if (ok) {
      bnoCalibrado = true;
      Serial.println("[BNO055] Calibracao do giro concluida.");
      if (oledDisponivel) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(7, 8);
        display.println("CALIBRADO!");
        display.setTextSize(1);
        display.setCursor(0, 39);
        display.println("Ative o BOTAO 2");
        display.println("para definir o ZERO");
        display.display();
      }
      break;
    }
    delay(200);
  }
}

// Atualiza a orientacao sem zerar automaticamente a referencia.
// Rejeita amostras incompativeis com uma rotacao fisica plausivel.
void atualizarOrientacaoBNO(bool forcar = false) {
  unsigned long agora = millis();
  if (!forcar && (agora - ultimaTentativaBNO < BNO_INTERVALO_MS)) return;
  ultimaTentativaBNO = agora;

  if (bnoPrecisaZerar) {
    bnoValido = false;
    return;
  }

  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  float novoYaw = euler.x();

  if (!isfinite(novoYaw) || novoYaw < -0.1f || novoYaw >= 360.5f) {
    bnoValido = false;
    Serial.println("[BNO055] ERRO: leitura de yaw invalida.");
    return;
  }

  if (bnoTemLeitura) {
    float salto = fabsf(normalizarAngulo(novoYaw - yawBruto));
    unsigned long dt = agora - ultimaLeituraBNO;
    // Limite conservador: 1080 graus/s + 15 graus de folga.
    // Nao confundir a passagem de 359 -> 0 com salto.
    float saltoMaximo = 15.0f + 1080.0f * (float)dt / 1000.0f;
    if (saltoMaximo > 75.0f) saltoMaximo = 75.0f;
    if (salto > saltoMaximo) {
      // Uma leitura isolada pode ser ruido I2C; rejeita a amostra.
      // Tres rejeicoes seguidas indicam possivel reset/perda do heading.
      bnoSaltosConsecutivos++;
      bnoValido = false;
      if (false) {
        bnoPrecisaZerar = true;
        Serial.print("[BNO055] REFERENCIA PERDIDA! Salto: ");
        Serial.print(salto, 1);
        Serial.println(" graus. Reinicie e calibre.");
      }
      return;
    }
  }

  bnoSaltosConsecutivos = 0;
  yawBruto = novoYaw;
  anguloRobo = normalizarAngulo(yawBruto - offsetOrientacao);
  ultimaLeituraBNO = agora;
  bnoTemLeitura = true;
  bnoValido = true;
}

// Define o zero SOMENTE quando o robo estiver parado.
// Nao tenta mascarar uma perda de referencia do proprio BNO055.
void zerarOrientacao() {
  if (bno.getMode() != MODO_BNO) {
    bnoValido = false;
    bnoPrecisaZerar = true;
    Serial.println("[BNO055] Modo incorreto: reinicie para recalibrar.");
    return;
  }
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  float novoYaw = euler.x();
  if (!isfinite(novoYaw) || novoYaw < -0.1f || novoYaw >= 360.5f) {
    bnoValido = false;
    Serial.println("[BNO055] Nao foi possivel definir o zero.");
    return;
  }
  offsetOrientacao = novoYaw;
  yawBruto = novoYaw;
  anguloRobo = 0.0f;
  bnoTemLeitura = true;
  bnoSaltosConsecutivos = 0;
  bnoPrecisaZerar = false;
  bnoValido = true;
  ultimaTentativaBNO = millis();
  ultimaLeituraBNO = ultimaTentativaBNO;
  Serial.print("[BNO055] ZERO definido. Offset: ");
  Serial.println(offsetOrientacao, 1);
}

// Usa o ultimo yaw validado, atualizado periodicamente.
float lerOrientacao() {
  atualizarOrientacaoBNO();
  return anguloRobo;
}

bool bnoProntoParaJogar() {
  return bnoCalibrado && bnoValido && !bnoPrecisaZerar &&
         (millis() - ultimaLeituraBNO <= BNO_MAX_IDADE_MS);
}

void diagnosticoBNO() {
  static unsigned long ultimo = 0;
  if (!DEBUG_BNO || millis() - ultimo < BNO_DIAGNOSTICO_MS) return;
  ultimo = millis();

  uint8_t sys = 0, gyro = 0, accel = 0, mag = 0;
  bno.getCalibration(&sys, &gyro, &accel, &mag);
  adafruit_bno055_opmode_t modoAtual = bno.getMode();
  if (modoAtual != MODO_BNO) {
    bnoValido = false;
    bnoPrecisaZerar = true;
    Serial.println("[BNO055] MODO MUDOU! Reinicie o robo.");
  }

  Serial.print("[BNO] YAW_REL="); Serial.print(anguloRobo, 1);
  Serial.print(" | RAW="); Serial.print(yawBruto, 1);
  Serial.print(" | CAL SYS/G/A/M="); Serial.print(sys);
  Serial.print('/'); Serial.print(gyro);
  Serial.print('/'); Serial.print(accel);
  Serial.print('/'); Serial.print(mag);
  Serial.print(" | MODO="); Serial.print((int)modoAtual);
  Serial.print(" | IDADE(ms)="); Serial.print(millis() - ultimaLeituraBNO);
  Serial.print(" | OK="); Serial.println(bnoProntoParaJogar() ? "SIM" : "NAO");
}

// 'z' no Monitor Serial redefine o ZERO apenas com o botao de jogo solto.
void verificarComandosSerial() {
  if (!Serial.available()) return;
  char comando = Serial.read();
  if (comando == 'z' || comando == 'Z') {
    if (digitalRead(botao) == HIGH && !bnoPrecisaZerar) {
      zerarOrientacao();
    } else {
      Serial.println("[BNO055] Solte o botao de jogo. Se houve erro, reinicie.");
    }
  }
}

// =====================================================
// ERRO ANGULAR (menor caminho até o alvo)
// =====================================================

#define ROT_KP              1.4f
#define ROT_VELOCIDADE_MIN  15
#define ROT_VELOCIDADE_MAX  60

#define ROT_ENTRAR           5.0f   // histerese: ativa a partir daqui
#define ROT_SAIR             2.0f   // desativa ao voltar para perto de zero
// =========================
// TRACKBALL XY — movimentos discretos + PID de orientação para manter 0°
// "Ir atrás da bola": quando a bola está muito perto (intensidade alta) e alinhada,
// o robô recua/desacelera em vez de continuar avançando sobre ela
// =========================
// ROTAÇÃO PARA ÂNGULO ALVO
// =========================

float calcularErro(float anguloAtual, float anguloAlvo) {
  float erro = anguloAlvo - anguloAtual;
  while (erro >  180) erro -= 360;
  while (erro < -180) erro += 360;
  return erro;
}


// =====================================================
// CORREÇÃO DE ÂNGULO (BNO055)
// =====================================================

// Gira até o ângulo alvo. Retorna true quando chegou.


// =====================================================
// LEITURA DO IR SEEKER
// =====================================================
// =====================================================
// LEITURA DOS SENSORES ULTRASSONICOS
// =====================================================

// Mede a distancia em centimetros.
// Retorna -1 caso nao receba um eco.

long medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // 22 ms permite medir aproximadamente 3,7 m;
  // o limite traseiro de 270 cm continua coberto.
  unsigned long duracao = pulseIn(echoPin, HIGH, 22000UL);
  if (duracao == 0) return -1;
  return duracao / 58;
}

// Apenas UM ultrassonico por ciclo de 28ms, evitando que 4 pulseIn()
// bloqueiem a malha de orientacao. As rotinas de decisao usam o cache.
long distanciaUltrassom[4] = {-1, -1, -1, -1};

const unsigned long INTERVALO_ULTRASSOM_MS = 28;

void atualizarUltrassonicos() {
  static unsigned long ultimoPing = 0;
  static int indice = 0;
  if (millis() - ultimoPing < INTERVALO_ULTRASSOM_MS) return;
  ultimoPing = millis();

  switch (indice) {
    case 0: distanciaUltrassom[0] = medirDistancia(TRIG1, ECHO1); break;
    case 1: distanciaUltrassom[1] = medirDistancia(TRIG2, ECHO2); break;
    case 2: distanciaUltrassom[2] = medirDistancia(TRIG3, ECHO3); break;
    case 3: distanciaUltrassom[3] = medirDistancia(TRIG4, ECHO4); break;
  }

  indice = (indice + 1) % 4;
  // Depois da leitura que pode bloquear, atualizar o gyro imediatamente.
  atualizarOrientacaoBNO();
}

long lerFrente()   { return distanciaUltrassom[0]; }
long lerTras()     { return distanciaUltrassom[1]; }
long lerEsquerda() { return distanciaUltrassom[2]; }
long lerDireita()  { return distanciaUltrassom[3]; }

void lerSeeker() {
  InfraredResult resultado = InfraredSeeker::ReadAC();

  ballDirecao = resultado.Direction;
  ballIntens = resultado.Strength;

  Serial.print("[IR SEEKER] Direcao: ");
  Serial.print(ballDirecao);
  Serial.print(" | Intensidade: ");
  Serial.println(ballIntens);
}
// =====================================================
// LEITURA DOS TSOPS
// =====================================================

// Verifica se dois TSOPs ativos são vizinhos.
// O TSOP 6 (índice 5) está defeituoso, então 5 e 7
// são considerados vizinhos.


rtos::Mutex travaAnel;
int direcaoRecebida = -1;
unsigned int mascaraRecebida = 0;
unsigned long instanteAnel = 0;
bool recebeuAnel = false;

void receberAnel(int direcao, unsigned int mascara) {
  travaAnel.lock();
  direcaoRecebida = direcao;
  mascaraRecebida = mascara;
  instanteAnel = millis();
  recebeuAnel = true;
  travaAnel.unlock();
}

void atualizarCacheAnel() {
  travaAnel.lock();
  // Margem de uma janela adicional para entrega da proxima amostra.
  bool recente = recebeuAnel && millis() - instanteAnel <= 2 * TSOP_TEMPO_JANELA;
  direcaoFinalTSOP = recente ? direcaoRecebida : -1;
  unsigned int mascara = recente ? mascaraRecebida : 0;
  travaAnel.unlock();
  for (int i = 0; i < NUM_TSOPS; ++i) tsopDetectado[i] = mascara & (1u << i);
}


// =====================================================
// TELAS DO DISPLAY OLED
// =====================================================
void mostrarGiroscopioOLED() {
  if (!oledDisponivel) return;

  // =========================================
  // LEITURA DO ANGULO
  // =========================================

  float yaw = lerOrientacao();

  // =========================================
  // LEITURA DOS ULTRASSONICOS
  // =========================================

  long frente = lerFrente();
  long tras = lerTras();
  long esquerda = lerEsquerda();
  long direita = lerDireita();

  // =========================================
  // CONFIGURACAO DO DISPLAY
  // =========================================

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Titulo
  display.setCursor(0, 0);
  display.println("GIROSCOPIO BNO055");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // =========================================
  // ANGULO NUMERICO
  // =========================================

  display.setCursor(0, 17);
  display.print("ANG: ");
  if (bnoProntoParaJogar()) {
    display.print(yaw, 1);
    display.print(" deg");
  } else {
    display.print("BNO INVAL.");
  }

  // =========================================
  // REPRESENTACAO VISUAL DA ORIENTACAO
  // =========================================

  const int centroX = 108;
  const int centroY = 29;
  const int raio = 14;

  // Circulo de referencia
  display.drawCircle(centroX, centroY, raio, SSD1306_WHITE);

  // Marca a referencia de zero grau (para frente)
  display.drawLine(
    centroX, centroY - raio,
    centroX, centroY - raio + 4,
    SSD1306_WHITE
  );

  // Calcula a posicao da ponta da seta.
  // Zero grau aponta para cima.
  float radianos = yaw * PI / 180.0;

  int pontaX = centroX + (int)(sin(radianos) * 11);
  int pontaY = centroY - (int)(cos(radianos) * 11);

  // Seta indicando a orientacao atual
  display.drawLine(
    centroX, centroY,
    pontaX, pontaY,
    SSD1306_WHITE
  );

  display.fillCircle(pontaX, pontaY, 2, SSD1306_WHITE);
  display.fillCircle(centroX, centroY, 2, SSD1306_WHITE);

  // =========================================
  // LEITURAS DOS ULTRASSONICOS
  // =========================================

  display.drawLine(0, 46, 127, 46, SSD1306_WHITE);

  // Frente e traseira
  display.setCursor(0, 48);

  display.print("F:");
  if (frente < 0) display.print("--");
  else display.print(frente);
  display.print("cm");

  display.print("  T:");
  if (tras < 0) display.print("--");
  else display.print(tras);
  display.print("cm");

  // Esquerda e direita
  display.setCursor(0, 57);

  display.print("E:");
  if (esquerda < 0) display.print("--");
  else display.print(esquerda);
  display.print("cm");

  display.print("  D:");
  if (direita < 0) display.print("--");
  else display.print(direita);
  display.print("cm");

  display.display();
}
void mostrarRingTSOPOLED() {
  if (!oledDisponivel) return;

  // Atualiza o IR Seeker para exibir a intensidade junto ao ring TSOP.
  lerSeeker();

  // Atualiza os 16 estados e calcula a decisao final pelo algoritmo existente.
  int decisao = direcaoFinalTSOP;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("RING TSOP   DEC:");
  if (decisao == -1) display.print("--");
  else display.print(decisao);

  // Os sensores ficam igualmente distribuidos em um circulo.
  // TSOP 1 fica no topo; a numeracao segue no sentido horario.
  const int centroX = 64;
  const int centroY = 34;
  const int raio = 18;

  for (int i = 0; i < NUM_TSOPS; i++) {
    float angulo = -PI / 2.0 + (2.0 * PI * i / NUM_TSOPS);
    int x = centroX + (int)(raio * cos(angulo));
    int y = centroY + (int)(raio * sin(angulo));

    if (tsopDetectado[i]) {
      display.fillCircle(x, y, 3, SSD1306_WHITE);
    } else {
      display.drawCircle(x, y, 3, SSD1306_WHITE);
    }
  }

  // Valor central: decisao final retornada por lerDirecaoTSOP().
  display.setTextSize(1);
  display.setCursor(54, 30);
  display.print("D:");
  if (decisao == -1) display.print("--");
  else display.print(decisao);

  // Intensidade da bola informada pelo IR Seeker.
  display.setCursor(0, 56);
  display.print("INTENSIDADE: ");
  display.print(ballIntens);

  display.display();
}

void atualizarDisplayOLED() {
  static unsigned long ultimaAtualizacaoOLED = 0;
  const unsigned long INTERVALO_OLED = 120;

  if (millis() - ultimaAtualizacaoOLED < INTERVALO_OLED) return;
  ultimaAtualizacaoOLED = millis();

  // Botao 2 em 0: tela do giroscopio.
  // Botao 2 em 1: tela do ring de TSOPs.
  if (digitalRead(botao2) == LOW) {
    mostrarGiroscopioOLED();
  } else {
    mostrarRingTSOPOLED();
  }
}

// =====================================================
// MOVIMENTAÇÃO BASEADA NO TSOP + IR SEEKER
// =====================================================

  // TSOP 0 = MEIO
     // Serial.println("[DECISAO] Nenhum sensor detectado");
    //  Serial.println("[DECISAO] TSOP 2/3: DIAGONAL TRASEIRA ESQUERDA");
     // Serial.println("[DECISAO] TSOP 4/5: TRAS");
     // Serial.println("[DECISAO] TSOP 9/10/11: DIAGONAL TRASEIRA ESQUERDA");

     // Serial.println("[DECISAO] TSOP 12/13: DIAGONAL TRASEIRA DIREITA");
     // Serial.println("[DECISAO] Direcao invalida: PARAR");

// =====================================================
// BOTÃO - ZERAR ORIENTAÇÃO
// =====================================================

// =====================================================
// SETUP
// =====================================================
// =====================================================
// TESTE INDIVIDUAL DOS MOTORES
// =====================================================

// Desliga todos os motores antes do teste.

// motor = 1, 2, 3 ou 4
// pinoPWM = pino PWM correspondente
// velocidade = -255 a 255 (padrão 100)


  // Primeiro desliga TODOS os motores.

  // Confere se o PWM informado pertence ao motor.

  // Define o sentido de rotacao.

  // Aciona somente o PWM selecionado.

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Inicializa o ESC com sinal minimo: o dribbler comeca parado.
  esc.attach(PIN_ESC, ESC_MIN, ESC_MAX);
  esc.writeMicroseconds(ESC_MIN); // pulso seguro, mesmo na 1a inicializacao
  pararDribbler();
  Serial.println("[SETUP] Armando ESC com dribbler parado...");
  delay(4000);
  Serial.println("[SETUP] ESC pronto; dribbler permanece parado.");

  // Inicializa o display OLED I2C (endereco padrao 0x3C).
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ENDERECO)) {
    oledDisponivel = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Iniciando robo...");
    display.display();
    Serial.println("[SETUP] OLED encontrado");
  } else {
    oledDisponivel = false;
    Serial.println("[ERRO] OLED nao encontrado no endereco 0x3C");
  }

  Serial.println();
  Serial.println("================================");
  Serial.println(" INICIANDO SISTEMA DO ROBO");
  Serial.println("================================");

  // Motores
  int inPins[] = {
    M1_IN1, M1_IN2,
    M2_IN1, M2_IN2,
    M3_IN1, M3_IN2,
    M4_IN1, M4_IN2
  };

  for (int i = 0; i < 8; i++) {
    pinMode(inPins[i], OUTPUT);
    digitalWrite(inPins[i], LOW);
  }

  int pwmPins[] = {
    M1_PWM, M2_PWM, M3_PWM, M4_PWM
  };

  for (int i = 0; i < 4; i++) {
    pinMode(pwmPins[i], OUTPUT);
    analogWrite(pwmPins[i], 0);
  }

  Serial.println("[SETUP] Pinos dos motores configurados");

  RPC.begin();
  RPC.bind("anel", receberAnel);
// =====================================================
// SENSORES ULTRASSONICOS
// =====================================================

pinMode(TRIG1, OUTPUT);
pinMode(ECHO1, INPUT);

pinMode(TRIG2, OUTPUT);
pinMode(ECHO2, INPUT);

pinMode(TRIG3, OUTPUT);
pinMode(ECHO3, INPUT);

pinMode(TRIG4, OUTPUT);
pinMode(ECHO4, INPUT);

digitalWrite(TRIG1, LOW);
digitalWrite(TRIG2, LOW);
digitalWrite(TRIG3, LOW);
digitalWrite(TRIG4, LOW);

Serial.println("[SETUP] Ultrassonicos configurados");
  // Botoes
  pinMode(botao, INPUT_PULLUP);
  pinMode(botao2, INPUT_PULLUP);
  pinMode(switchsensor, INPUT_PULLUP);

  Serial.println("[SETUP] Botao 1 e botao 2 configurados");
  Serial.println("[SETUP] Botao 2 LOW = Giroscopio | HIGH = Ring TSOP");

  // Parar motores
  parar();

  // BNO055
  Serial.println("[SETUP] Inicializando BNO055...");

  if (!bno.begin(MODO_BNO)) {
    Serial.println("[ERRO] BNO055 nao encontrado!");
    Serial.println("[ERRO] Verifique alimentacao e conexoes I2C");

    while (1) {
      // O programa para aqui se o BNO055 nao for encontrado.
    }
  }

  Serial.println("[SETUP] BNO055 encontrado");

  delay(1000);
  Serial.println("[BNO055] Modo IMUPLUS: sem magnetometro.");
  Serial.println("[BNO055] Calibre PARADO; o gyro deve ficar em 3/3.");

  if (calibrar) {
    // Antes de calibrar, o botao 2 precisa estar desligado (LOW).
    while (digitalRead(botao2) != HIGH) {
      Serial.println("[BNO055] Desligue o BOTAO 2 para iniciar a calibracao.");

      if (oledDisponivel) {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("PREPARAR CALIBRACAO");
        display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
        display.setTextSize(2);
        display.setCursor(5, 22);
        display.println("DESLIGUE");
        display.setCursor(5, 42);
        display.println("BOTAO 2");
        display.display();
      }

      delay(200);
    }

    Serial.println("[BNO055] Botao 2 desligado. Iniciando calibracao.");
    bno_calibrar();

    // Apos calibrar, aguarda a ativacao do botao 2 para definir o zero.
    while (digitalRead(botao2) == HIGH) {
      delay(20);
    }

    delay(50); // debounce
    if (digitalRead(botao2) == LOW) {
      zerarOrientacao();
      Serial.println("[BNO055] Ponto zero definido pelo botao 2.");
    }
  } else {
    bnoCalibrado = true;  // Somente para testes sem espera; calibração recomendada.
    zerarOrientacao();
  }

  atualizarOrientacaoBNO(true);
  Serial.println("[SETUP] IR Seeker pronto");
  Serial.println("[SETUP] TSOPs prontos");
  Serial.println("[SETUP] Sistema iniciado!");
  Serial.println("[SETUP] Botao solto = motores parados");
  Serial.println("[SETUP] Botao pressionado = controle ativo");
  Serial.println("================================");
}

// =====================================================
// LOOP
void movimentarPorTSOPComCorrecao() {
  const int V = 200;
  const int V_FUGA = 240;
  const long DIST_PERIGO = 18;
  const long DIST_LIBERAR = 20;
  const long LIMITE_TRAS = 270;
  const long RETORNO_TRAS = 260;

  const int VELOCIDADE_TREMIDA = 120;
  const unsigned long TEMPO_PULSO_TREMIDA = 70;

  static int sensorFuga = -1;
  static bool retornando = false;

  static unsigned long tempoTremida = 0;
  static int etapaTremida = 0;

  InfraredResult resultado = InfraredSeeker::ReadAC();

  ballDirecao = resultado.Direction;
  ballIntens = resultado.Strength;

  // Indices: 0 = frente, 1 = tras, 2 = esquerda, 3 = direita.
  // Le cada sensor uma vez; o traseiro e lido por ultimo para
  // que a decisao do limite use a leitura mais recente.
  long distancia[4];
  distancia[0] = lerFrente();
  distancia[2] = lerEsquerda();
  distancia[3] = lerDireita();
  distancia[1] = lerTras();

  const int oposto[4] = {1, 0, 3, 2};
  const long minimoOposto[4] = {110, 100, 150, 150};
  bool trasValido = distancia[1] > 0;

  // A memoria evita alternar entre perseguir e recuar perto de 270.
  // Uma leitura invalida nao apaga um retorno que ja estava ativo.
  if (trasValido) {
    if (distancia[1] >= LIMITE_TRAS) retornando = true;
    else if (distancia[1] <= RETORNO_TRAS) retornando = false;
  }

  // Entre os sensores que atendem as duas condicoes,
  // escolhe o mais proximo. -1 e 0 nao sao distancias validas.
  int escolhido = -1;
  long menor = max(DIST_LIBERAR, 48L);

  for (int i = 0; i < 4; i++) {
    long limiar;

    if (i == 0 || i == 1) {
      limiar = 48;  // Frente e tras: fugir abaixo de 48 cm.
    }
    else {
      limiar = (i == sensorFuga) ? DIST_LIBERAR : DIST_PERIGO;
    }

    if (distancia[i] > 0 && distancia[i] < limiar &&
        distancia[oposto[i]] > minimoOposto[i] &&
        distancia[i] < menor) {
      menor = distancia[i];
      escolhido = i;
    }
  }
  sensorFuga = escolhido;

  int m1 = 0, m2 = 0, m3 = 0, m4 = 0;

  if (!trasValido) {
    // Sem eco traseiro, suspende a translacao ate recuperar a leitura.
    // A correcao angular continua sendo aplicada abaixo.
  }
  else if (sensorFuga != -1 || retornando) {
    // x positivo = direita; y positivo = frente.
    int x = 0, y = 0;
    switch (sensorFuga) {
      case 0: y = -V_FUGA; break;  // Frente perto: recuar.
      case 1: y =  V_FUGA; break;  // Tras perto: avancar.
      case 2: x =  V_FUGA; break;  // Esquerda perto: ir a direita.
      case 3: x = -V_FUGA; break;  // Direita perto: ir a esquerda.
    }

    // O limite traseiro tem prioridade no eixo frente/tras.
    // Se houver fuga lateral, combina os dois em uma diagonal.
    if (retornando) y = -V_FUGA;

    m1 = -y - x;
    m2 =  y - x;
    m3 = -y + x;
    m4 =  y + x;

    // Limita a diagonal antes de acrescentar a correcao angular.
    int pico = max(max(abs(m1), abs(m2)), max(abs(m3), abs(m4)));
    if (pico > V_FUGA) {
      float fator = (float)V_FUGA / pico;
      m1 *= fator; m2 *= fator; m3 *= fator; m4 *= fator;
    }
  }
  else {
    // A busca pela bola so comanda a translacao fora da fuga.
    int direcao = direcaoFinalTSOP;

    if (direcao < 0 || direcao > 15) {
      // Sem deteccao TSOP: pulso para a esquerda e depois para a direita.
      unsigned long agora = millis();

      switch (etapaTremida) {
        case 0:
          m1 = -VELOCIDADE_TREMIDA;
          m2 = -VELOCIDADE_TREMIDA;
          m3 = VELOCIDADE_TREMIDA;
          m4 = VELOCIDADE_TREMIDA;

          tempoTremida = agora;
          etapaTremida = 1;
          break;

        case 1:
          if (agora - tempoTremida >= TEMPO_PULSO_TREMIDA) {
            m1 = VELOCIDADE_TREMIDA;
            m2 = VELOCIDADE_TREMIDA;
            m3 = -VELOCIDADE_TREMIDA;
            m4 = -VELOCIDADE_TREMIDA;

            tempoTremida = agora;
            etapaTremida = 2;
          }
          else {
            m1 = -VELOCIDADE_TREMIDA;
            m2 = -VELOCIDADE_TREMIDA;
            m3 = VELOCIDADE_TREMIDA;
            m4 = VELOCIDADE_TREMIDA;
          }
          break;

        case 2:
          if (agora - tempoTremida >= TEMPO_PULSO_TREMIDA) {
            m1 = 0;
            m2 = 0;
            m3 = 0;
            m4 = 0;

            etapaTremida = 0;
          }
          else {
            m1 = VELOCIDADE_TREMIDA;
            m2 = VELOCIDADE_TREMIDA;
            m3 = -VELOCIDADE_TREMIDA;
            m4 = -VELOCIDADE_TREMIDA;
          }
          break;
      }
    }
    else {
      etapaTremida = 0;

      switch (direcao) {
        case 0:
          // Direcao 0: avancar.
          m1 = -V; m2 = V; m3 = -V; m4 = V;
          break;

        case 1:
          if (ballDirecao == 5 || ballDirecao == 6) {
            m1 = -V; m2 = V; m3 = -V; m4 = V;
          }
          else {
            m1 = V; m2 = V; m3 = -V; m4 = -V;
          }
          break;

        case 15:
          if (ballDirecao == 5 || ballDirecao == 6) {
            m1 = -V; m2 = V; m3 = -V; m4 = V;
          }
          else {
            m1 = -V; m2 = -V; m3 = V; m4 = V;
          }
          break;

        case 2:
          m1 = 0; m2 = V; m3 = -V; m4 = 0;
          break;

        case 3:
          m1 = V; m2 = 0; m3 = 0; m4 = -V;
          break;

        case 4:
        case 5:
        case 9:
        case 10:
          m1 = V; m2 = -V; m3 = V; m4 = -V;
          break;

        case 6:
        case 7:
        case 8:
          m1 = 0; m2 = -V; m3 = V; m4 = 0;
          break;

        case 11:
          m1 = 0; m2 = -V; m3 = V; m4 = 0;
          break;

        case 12:
        case 13:
        case 14:
        if (ballDirecao == 5 || ballDirecao == 6) {
            m1 = -V; m2 = V; m3 = -V; m4 = V;
          }
          else {
             m1 = -V; m2 = 0; m3 = 0; m4 = V;

          }
          break;

        default:
          break;
      }
    }
  }

  // O gyro pode ter sido atualizado durante a janela TSOP de 50ms.
  // Em caso de salto/reset de referencia, a prioridade e PARAR.
  float erro = calcularErro(lerOrientacao(), 0.0f);

  // Histerese: evita chacoalhar ao redor do zero.
  float moduloErro = fabsf(erro);
  if (!correcaoOrientacaoAtiva && moduloErro >= ROT_ENTRAR) {
    correcaoOrientacaoAtiva = true;
  } else if (correcaoOrientacaoAtiva && moduloErro <= ROT_SAIR) {
    correcaoOrientacaoAtiva = false;
  }

  int correcao = 0;
  if (correcaoOrientacaoAtiva) {
    correcao = constrain((int)(moduloErro * ROT_KP),
                         ROT_VELOCIDADE_MIN, ROT_VELOCIDADE_MAX);
    // Mantem o mesmo sentido de correcao que o codigo original.
    if (erro > 0.0f) correcao = -correcao;
  }

  // Prioriza a orientacao: reserva espaco no PWM para o giro,
  // em vez de saturar os motores e enfraquecer a correcao.
  int picoTranslacao = max(max(abs(m1), abs(m2)), max(abs(m3), abs(m4)));
  int limiteTranslacao = 255 - abs(correcao);
  if (picoTranslacao > limiteTranslacao) {
    float fator = (float)limiteTranslacao / (float)picoTranslacao;
    m1 = (int)(m1 * fator);
    m2 = (int)(m2 * fator);
    m3 = (int)(m3 * fator);
    m4 = (int)(m4 * fator);
  }
  m1 += correcao;
  m2 += correcao;
  m3 += correcao;
  m4 += correcao;
  m4 = (int)(m4 * COMPENSACAO_M4);

  // Saturacao final de seguranca (inclui eventual compensacao de M4).
  int maior = max(max(abs(m1), abs(m2)), max(abs(m3), abs(m4)));
  if (maior > 255) {
    float fator = 255.0f / (float)maior;
    m1 = (int)(m1 * fator);
    m2 = (int)(m2 * fator);
    m3 = (int)(m3 * fator);
    m4 = (int)(m4 * fator);
  }

  setMotor(1, m1);
  setMotor(2, m2);
  setMotor(3, m3);
  setMotor(4, m4);
}
void loop() {
  // Ordem: amostra o BNO primeiro; nenhum dispositivo deve trocar seu zero.
  atualizarOrientacaoBNO();
  //verificarComandosSerial();

  // Um ultrassonico por vez, com leituras armazenadas.
  atualizarUltrassonicos();

  // OLED usa leituras em cache; nao dispara mais 4 pulseIn() seguidos.
  atualizarCacheAnel();
  atualizarDisplayOLED();

  // Diagnostico para identificar reset do modo, recalibracao e saltos.
  //diagnosticoBNO();

  // Botao solto = parado; somente o botao 2 do setup define o zero.
  if (digitalRead(botao) == HIGH) {
    correcaoOrientacaoAtiva = false;
    parar();
    pararDribbler();
    return;
  }

  // Se o BNO falhar ou perder a referencia, bloquear os motores.
 // if (!bnoProntoParaJogar()) {
 //   correcaoOrientacaoAtiva = false;
 //   parar();
  //  pararDribbler();
   // return;
 // }
  //Serial.println(digitalRead(botao));
  //setMotor(4, 255);
 //Serial.println(ballDirecao);
  movimentarPorTSOPComCorrecao();
  // O dribbler continua desabilitado como no loop original.
}