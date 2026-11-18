// =====================================================
// GOLEIRO - RoboCup Junior  (nucleo M7)
// Estrategia:
//  - Laterais: nunca menos de 30 cm de nenhuma parede (regra de fuga).
//  - Sem bola: busca 180 cm na frente, 40 cm atras e 85 cm de cada lado.
//  - Limites: avanco para a 170 cm da frente e recuo a 23 cm do fundo.
//  - Defesa: desliza para esquerda/direita conforme a direcao da bola,
//    sempre mantendo o angulo 0 com correcao em tempo real.
//
// TSOPs: 11 sensores, direcoes 1..11 em sentido anti-horario.
// 1 = frente; 2..6 = esquerda; 7..11 = direita; lacuna entre 6 e 7.
//
// Comandos pelo Monitor Serial (terminar com ENTER / "Nova linha"):
//   t        liga/desliga o MODO TESTE DE MOTORES
//   N V      (modo teste) motor N (1..4, ou 0 = todos) na velocidade V (-255..255)
//   p        (modo teste) para todos os motores
//   x        sai do modo teste
//   u        liga/desliga a impressao continua dos 4 ultrassonicos
//   z        redefine o ZERO do giroscopio (botao de jogo solto)
//   h        mostra esta ajuda
// =====================================================

#include <Arduino.h>
#include <RPC.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

// =====================================================
// BNO055
// =====================================================

Adafruit_BNO055 bno = Adafruit_BNO055(55);

// IMUPLUS usa giroscopio + acelerometro, SEM magnetometro.
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
int pulsoAtualDribbler = ESC_MIN;

// =====================================================
// MOTORES
// =====================================================

#define M1_IN1  46
#define M1_IN2  48
#define M1_PWM  4

#define M2_IN1  38
#define M2_IN2  40
#define M2_PWM 2

#define M3_IN1  52
#define M3_IN2  50
#define M3_PWM  5

#define M4_IN1  44
#define M4_IN2  42
#define M4_PWM 3

#define MOTOR1_DIR 1
#define MOTOR2_DIR 1
#define MOTOR3_DIR 1
#define MOTOR4_DIR 1

#define COMPENSACAO_M4 1.0

// ULTRASSONICOS: 1 = frente, 2 = tras, 3 = esquerda, 4 = direita
#define TRIG1 28
#define ECHO1 26

#define TRIG2 36
#define ECHO2 34

#define TRIG3 32
#define ECHO3 30

#define TRIG4 24
#define ECHO4 22

// Indices usados nas funcoes de leitura por direcao.
const int US_FRENTE   = 0;
const int US_TRAS     = 1;
const int US_ESQUERDA = 2;
const int US_DIREITA  = 3;

const int PINOS_TRIG[4] = { TRIG1, TRIG2, TRIG3, TRIG4 };
const int PINOS_ECHO[4] = { ECHO1, ECHO2, ECHO3, ECHO4 };
const char* const NOMES_US[4] = { "FRENTE", "TRAS", "ESQUERDA", "DIREITA" };

// =====================================================
// BOTOES
// =====================================================

#define botao 11
#define botao2 10

#define switchsensor 13

// =====================================================
// TSOPs (recebidos do M4)
// =====================================================
// 11 sensores, direcoes fisicas 1..11 em sentido anti-horario.
// 1 = frente; 2..6 = esquerda; 7..11 = direita.
// A traseira nao tem sensor (lacuna entre 6 e 7).
// O M4 envia direcao 1..11 (ou -1) e mascara de 11 bits.
// Bit k corresponde ao sensor fisico k + 1.

const int NUM_TSOPS = 11;
const int TSOP_TEMPO_JANELA = 50;

// Angulos em graus a partir da frente, positivos no sentido horario.
// Numeracao fisica 1..11 em sentido ANTI-HORARIO; indice = numero - 1.
// A lacuna traseira fica entre os sensores 6 e 7.
const float ANGULOS_TSOP_GRAUS[NUM_TSOPS] = {
  0.0f, 337.5f, 315.0f, 292.5f, 270.0f, 247.5f,
  112.5f, 90.0f, 67.5f, 45.0f, 22.5f
};

// Pino do M4 de cada TSOP: indice k = sensor fisico k + 1. Diagnostico.
const int PINOS_TSOP_M4[NUM_TSOPS] = {
  31, 33, 35, 39, 41, 43,  // TSOPs 1..6
  29, 37, 25, 23, 27       // TSOPs 7..11
};

bool tsopDetectado[NUM_TSOPS] = {false};
int direcaoFinalTSOP = -1;     // 1..11 ou -1

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

void pararDribbler() {
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

void setMotor(int motor, int speedValue) {
  switch (motor) {
    case 1:
      motorWrite(M1_IN1, M1_IN2, M1_PWM, speedValue * MOTOR1_DIR);
      break;
    case 2:
      motorWrite(M2_IN1, M2_IN2, M2_PWM, speedValue * MOTOR2_DIR);
      break;
    case 3:
      motorWrite(M3_IN1, M3_IN2, M3_PWM, speedValue * MOTOR3_DIR);
      break;
    case 4:
      motorWrite(M4_IN1, M4_IN2, M4_PWM, speedValue * MOTOR4_DIR);
      break;
  }
}

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

void moverFrente(int VELOCIDADE = 200) {
  registrarAcao("MOVENDO PARA FRENTE");

  setMotor(1, -VELOCIDADE);
  setMotor(2, VELOCIDADE);
  setMotor(3, -VELOCIDADE);
  setMotor(4, VELOCIDADE * COMPENSACAO_M4);
}

void moverEsquerda(int VELOCIDADE = 200) {
  registrarAcao("MOVENDO PARA ESQUERDA");

  setMotor(1, VELOCIDADE);
  setMotor(2, VELOCIDADE);
  setMotor(3, -VELOCIDADE);
  setMotor(4, -VELOCIDADE * COMPENSACAO_M4);
}

void moverDireita(int VELOCIDADE = 200) {
  registrarAcao("MOVENDO PARA DIREITA");

  setMotor(1, -VELOCIDADE);
  setMotor(2, -VELOCIDADE);
  setMotor(3, VELOCIDADE);
  setMotor(4, VELOCIDADE * COMPENSACAO_M4);
}

void moverTras(int VELOCIDADE = 200) {
  registrarAcao("MOVENDO PARA TRAS");

  setMotor(1, VELOCIDADE);
  setMotor(2, -VELOCIDADE);
  setMotor(3, VELOCIDADE);
  setMotor(4, -VELOCIDADE * COMPENSACAO_M4);
}

void moverDiagonalTrasEsquerda(int VELOCIDADE = 200) {
  registrarAcao("DIAGONAL: TRAS + ESQUERDA");

  setMotor(1, VELOCIDADE);
  setMotor(2, 0);
  setMotor(3, 0);
  setMotor(4, -VELOCIDADE);
}
void moverDiagonalFrenteDireita(int VELOCIDADE = 200) {

  setMotor(1, -VELOCIDADE);
  setMotor(2, 0);
  setMotor(3, 0);
  setMotor(4, VELOCIDADE);
}

void moverDiagonalTrasDireita(int VELOCIDADE = 200) {
  registrarAcao("DIAGONAL: TRAS + DIREITA");

  setMotor(1, 0);
  setMotor(2, -VELOCIDADE);
  setMotor(3, VELOCIDADE);
  setMotor(4, 0);
}
void moverDiagonalFrenteEsquerda(int VELOCIDADE = 200) {
  registrarAcao("DIAGONAL: TRAS + ESQUERDA");

  setMotor(1,0);
  setMotor(2, VELOCIDADE);
  setMotor(3, -VELOCIDADE);
  setMotor(4,0);
}

// =====================================================
// TESTE INDIVIDUAL DE MOTORES
// =====================================================
// Ative com 't' no Monitor Serial. Depois envie "N V", por exemplo:
//   2 150    -> motor 2 a +150
//   3 -200   -> motor 3 a -200 (sentido inverso)
//   0 100    -> todos os motores a +100
// Por seguranca o motor para sozinho apos TESTE_TIMEOUT_MS; reenvie o
// comando para continuar girando. 'p' para tudo, 'x' sai do modo.

bool modoTesteMotores = false;
int testeMotorAtivo = -1;                 // -1 = nenhum
unsigned long testeUltimoComando = 0;
const unsigned long TESTE_TIMEOUT_MS = 3000;

// Gira SOMENTE o motor escolhido (motor 0 = todos). Retorna false se invalido.
bool testarMotor(int motor, int velocidade) {
  if (motor < 0 || motor > 4) {
    Serial.println("[TESTE] Motor invalido. Use 1 a 4 (0 = todos).");
    return false;
  }
  velocidade = constrain(velocidade, -255, 255);

  for (int m = 1; m <= 4; m++) setMotor(m, 0);

  if (motor == 0) {
    for (int m = 1; m <= 4; m++) setMotor(m, velocidade);
  } else {
    setMotor(motor, velocidade);
  }

  testeMotorAtivo = (velocidade == 0) ? -1 : motor;
  testeUltimoComando = millis();

  Serial.print("[TESTE] Motor ");
  if (motor == 0) Serial.print("TODOS"); else Serial.print(motor);
  Serial.print(" -> velocidade ");
  Serial.println(velocidade);
  return true;
}

void pararTesteMotores() {
  for (int m = 1; m <= 4; m++) setMotor(m, 0);
  testeMotorAtivo = -1;
}

// Chamada a cada loop() enquanto o modo teste esta ligado.
void atualizarTesteMotores() {
  if (testeMotorAtivo != -1 &&
      millis() - testeUltimoComando > TESTE_TIMEOUT_MS) {
    pararTesteMotores();
    Serial.println("[TESTE] Tempo esgotado: motores parados.");
  }
}

void alternarModoTesteMotores() {
  modoTesteMotores = !modoTesteMotores;
  pararTesteMotores();
  correcaoOrientacaoAtiva = false;
  pararDribbler();

  if (modoTesteMotores) {
    Serial.println("[TESTE] MODO TESTE DE MOTORES LIGADO.");
    Serial.println("[TESTE] Envie: N V  (N = 1..4, 0 = todos | V = -255..255)");
    Serial.println("[TESTE] 'p' para parar | 'x' para sair.");
  } else {
    Serial.println("[TESTE] Modo teste desligado. Jogo normal.");
  }
}

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
    float saltoMaximo = 15.0f + 1080.0f * (float)dt / 1000.0f;
    if (saltoMaximo > 75.0f) saltoMaximo = 75.0f;
    if (salto > saltoMaximo) {
      bnoSaltosConsecutivos++;
      bnoValido = false;
      if (bnoSaltosConsecutivos >= 3) {
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

// =====================================================
// ERRO ANGULAR (menor caminho até o alvo)
// =====================================================

#define ROT_KP              1.5f
#define ROT_VELOCIDADE_MIN  15
#define ROT_VELOCIDADE_MAX  90

#define ROT_ENTRAR           5.0f   // histerese: ativa a partir daqui
#define ROT_SAIR             2.0f   // desativa ao voltar para perto de zero

float calcularErro(float anguloAtual, float anguloAlvo) {
  float erro = anguloAlvo - anguloAtual;
  while (erro >  180) erro -= 360;
  while (erro < -180) erro += 360;
  return erro;
}

// =====================================================
// LEITURA DOS SENSORES ULTRASSONICOS
// =====================================================

// Mede a distancia em centimetros. Retorna -1 caso nao receba um eco.
long medirDistancia(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

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

  distanciaUltrassom[indice] = medirDistancia(PINOS_TRIG[indice],
                                              PINOS_ECHO[indice]);

  indice = (indice + 1) % 4;
  atualizarOrientacaoBNO();
}

// Le o ultrassonico de UMA direcao (US_FRENTE, US_TRAS, US_ESQUERDA,
// US_DIREITA). Retorna cm, ou -1 se nao houve eco.
//  - imediato = false: devolve o ultimo valor do cache (nao bloqueia).
//  - imediato = true : dispara uma medicao agora (bloqueia ate ~22 ms);
//    util para testes e diagnostico.
long lerUltrassom(int direcao, bool imediato = false) {
  if (direcao < 0 || direcao > 3) return -1;
  if (imediato) {
    distanciaUltrassom[direcao] = medirDistancia(PINOS_TRIG[direcao],
                                                 PINOS_ECHO[direcao]);
  }
  return distanciaUltrassom[direcao];
}

long lerFrente()   { return lerUltrassom(US_FRENTE); }
long lerTras()     { return lerUltrassom(US_TRAS); }
long lerEsquerda() { return lerUltrassom(US_ESQUERDA); }
long lerDireita()  { return lerUltrassom(US_DIREITA); }

// Imprime as 4 direcoes no Serial. Se imediato = true, mede cada uma agora.
void imprimirUltrassonicos(bool imediato = false) {
  Serial.print("[US]");
  for (int d = 0; d < 4; d++) {
    long cm = lerUltrassom(d, imediato);
    Serial.print(' ');
    Serial.print(NOMES_US[d]);
    Serial.print('=');
    if (cm < 0) Serial.print("--"); else Serial.print(cm);
    Serial.print("cm");
  }
  Serial.println();
}

bool imprimirUltrassomContinuo = false;

void atualizarImpressaoUltrassom() {
  static unsigned long ultimo = 0;
  if (!imprimirUltrassomContinuo || millis() - ultimo < 250) return;
  ultimo = millis();
  imprimirUltrassonicos(false);
}

void lerSeeker() {
  //InfraredResult resultado = InfraredSeeker::ReadAC();

  //ballDirecao = resultado.Direction;
  //ballIntens = resultado.Strength;

  Serial.print("[IR SEEKER] Direcao: ");
  Serial.print(10);
  Serial.print(" | Intensidade: ");
  Serial.println(10);
}

// =====================================================
// TSOPs (recebido do nucleo M4 via RPC)
// =====================================================

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
  bool recente = recebeuAnel && millis() - instanteAnel <= 2 * TSOP_TEMPO_JANELA;
  int direcao = recente ? direcaoRecebida : -1;
  unsigned int mascara = recente ? mascaraRecebida : 0;
  travaAnel.unlock();

  // Direcao valida: 1..11. Qualquer outro valor vira "bola nao vista".
  direcaoFinalTSOP = (direcao >= 1 && direcao <= NUM_TSOPS) ? direcao : -1;

  for (int i = 0; i < NUM_TSOPS; ++i) {
    tsopDetectado[i] = (mascara & (1u << i)) != 0;
  }
}




void imprimirDiagnosticoTSOP() {
  static unsigned long ultimaImpressao = 0;

  if (millis() - ultimaImpressao < 250) return;
  ultimaImpressao = millis();

  Serial.print("[TSOP] Pinos M4 recebendo sinal: ");

  bool algumDetectado = false;

  for (int i = 0; i < NUM_TSOPS; i++) {
    if (tsopDetectado[i]) {
      if (algumDetectado) {
        Serial.print(", ");
      }

      Serial.print(PINOS_TSOP_M4[i]);
      Serial.print(" (sensor ");
      Serial.print(i + 1);
      Serial.print(")");

      algumDetectado = true;
    }
  }

  if (!algumDetectado) {
    Serial.print("nenhum");
  }

  Serial.print(" | Direcao escolhida: ");
  Serial.println(direcaoFinalTSOP);
}


// =====================================================
// TELAS DO DISPLAY OLED
// =====================================================
void mostrarGiroscopioOLED() {
  if (!oledDisponivel) return;

  float yaw = lerOrientacao();

  long frente = lerFrente();
  long tras = lerTras();
  long esquerda = lerEsquerda();
  long direita = lerDireita();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("GIROSCOPIO BNO055");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 17);
  display.print("ANG: ");
  if (bnoProntoParaJogar()) {
    display.print(yaw, 1);
    display.print(" deg");
  } else {
    display.print("BNO INVAL.");
  }

  const int centroX = 108;
  const int centroY = 29;
  const int raio = 14;

  display.drawCircle(centroX, centroY, raio, SSD1306_WHITE);
  display.drawLine(centroX, centroY - raio,
                   centroX, centroY - raio + 4, SSD1306_WHITE);

  float radianos = yaw * PI / 180.0;
  int pontaX = centroX + (int)(sin(radianos) * 11);
  int pontaY = centroY - (int)(cos(radianos) * 11);

  display.drawLine(centroX, centroY, pontaX, pontaY, SSD1306_WHITE);
  display.fillCircle(pontaX, pontaY, 2, SSD1306_WHITE);
  display.fillCircle(centroX, centroY, 2, SSD1306_WHITE);

  display.drawLine(0, 46, 127, 46, SSD1306_WHITE);

  display.setCursor(0, 48);
  display.print("F:");
  if (frente < 0) display.print("--"); else display.print(frente);
  display.print("cm");
  display.print("  T:");
  if (tras < 0) display.print("--"); else display.print(tras);
  display.print("cm");

  display.setCursor(0, 57);
  display.print("E:");
  if (esquerda < 0) display.print("--"); else display.print(esquerda);
  display.print("cm");
  display.print("  D:");
  if (direita < 0) display.print("--"); else display.print(direita);
  display.print("cm");

  display.display();
}

void mostrarRingTSOPOLED() {
  if (!oledDisponivel) return;

  lerSeeker();

  int decisao = direcaoFinalTSOP;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("RING TSOP   DEC:");
  if (decisao == -1) display.print("--");
  else display.print(decisao);

  const int centroX = 64;
  const int centroY = 34;
  const int raio = 18;

  // Desenha os 11 sensores na posicao fisica real do anel.
  for (int i = 0; i < NUM_TSOPS; i++) {
    float angulo = -PI / 2.0f + ANGULOS_TSOP_GRAUS[i] * PI / 180.0f;
    int x = centroX + (int)(raio * cos(angulo));
    int y = centroY + (int)(raio * sin(angulo));

    if (tsopDetectado[i]) {
      display.fillCircle(x, y, 3, SSD1306_WHITE);
    } else {
      display.drawCircle(x, y, 3, SSD1306_WHITE);
    }
  }

  display.setTextSize(1);
  display.setCursor(54, 30);
  display.print("D:");
  if (decisao == -1) display.print("--");
  else display.print(decisao);

  display.setCursor(0, 56);
  display.print("INTENSIDADE: ");
  display.print(ballIntens);

  display.display();
}

void mostrarTesteMotoresOLED() {
  if (!oledDisponivel) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("TESTE DE MOTORES");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 16);
  display.println("Envie pelo Serial:");
  display.println("  N V  (ex: 2 150)");
  display.println("p = parar | x = sair");
  display.setCursor(0, 50);
  display.print("Ativo: ");
  if (testeMotorAtivo == -1) display.print("nenhum");
  else if (testeMotorAtivo == 0) display.print("todos");
  else display.print(testeMotorAtivo);
  display.display();
}

void atualizarDisplayOLED() {
  static unsigned long ultimaAtualizacaoOLED = 0;
  const unsigned long INTERVALO_OLED = 120;

  if (millis() - ultimaAtualizacaoOLED < INTERVALO_OLED) return;
  ultimaAtualizacaoOLED = millis();

  if (modoTesteMotores) {
    mostrarTesteMotoresOLED();
    return;
  }

  // Botao 2 em 0: tela do giroscopio.
  // Botao 2 em 1: tela do ring de TSOPs.
  if (digitalRead(botao2) == LOW) {
    mostrarGiroscopioOLED();
  } else {
    mostrarRingTSOPOLED();
  }
}

// =====================================================
// COMANDOS PELO MONITOR SERIAL (linha terminada em ENTER)
// =====================================================

void mostrarAjudaSerial() {
  Serial.println("---- COMANDOS ----");
  Serial.println("t      : liga/desliga modo teste de motores");
  Serial.println("N V    : (teste) motor N (1..4, 0=todos), velocidade V (-255..255)");
  Serial.println("p      : (teste) parar motores");
  Serial.println("x      : (teste) sair do modo teste");
  Serial.println("u      : liga/desliga leitura continua dos ultrassonicos");
  Serial.println("us     : le os 4 ultrassonicos agora (medicao imediata)");
  Serial.println("z      : redefine o ZERO do giroscopio (botao solto)");
  Serial.println("h      : esta ajuda");
  Serial.println("------------------");
}

void tratarComandoSerial(char* texto) {
  // Remove espacos no inicio.
  while (*texto == ' ' || *texto == '\t') texto++;
  if (*texto == '\0') return;

  // Aceita virgula/ponto e virgula como separador.
  for (char* c = texto; *c; ++c) {
    if (*c == ',' || *c == ';') *c = ' ';
  }

  // Comecou com numero: comando de motor "N V".
  if ((*texto >= '0' && *texto <= '9') || *texto == '-') {
    if (!modoTesteMotores) {
      Serial.println("[TESTE] Envie 't' primeiro para entrar no modo teste.");
      return;
    }
    int motor = 0, velocidade = 0;
    if (sscanf(texto, "%d %d", &motor, &velocidade) == 2) {
      testarMotor(motor, velocidade);
    } else {
      Serial.println("[TESTE] Formato invalido. Use: N V  (ex: 2 150)");
    }
    return;
  }

  char c0 = tolower(texto[0]);
  char c1 = tolower(texto[1]);

  if (c0 == 'u' && c1 == 's') {
    imprimirUltrassonicos(true);
  } else if (c0 == 'u') {
    imprimirUltrassomContinuo = !imprimirUltrassomContinuo;
    Serial.print("[US] Leitura continua ");
    Serial.println(imprimirUltrassomContinuo ? "LIGADA" : "DESLIGADA");
  } else if (c0 == 't') {
    alternarModoTesteMotores();
  } else if (c0 == 'p') {
    pararTesteMotores();
    Serial.println("[TESTE] Motores parados.");
  } else if (c0 == 'x') {
    if (modoTesteMotores) alternarModoTesteMotores();
  } else if (c0 == 'z') {
    if (digitalRead(botao) == HIGH && !bnoPrecisaZerar && !modoTesteMotores) {
      zerarOrientacao();
    } else {
      Serial.println("[BNO055] Solte o botao de jogo e saia do modo teste. Se houve erro, reinicie.");
    }
  } else if (c0 == 'h' || c0 == '?') {
    mostrarAjudaSerial();
  } else {
    Serial.println("[SERIAL] Comando desconhecido. Envie 'h' para ajuda.");
  }
}

void verificarComandosSerial() {
  static char buffer[24];
  static uint8_t tamanho = 0;

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      buffer[tamanho] = '\0';
      if (tamanho > 0) tratarComandoSerial(buffer);
      tamanho = 0;
    } else if (tamanho < sizeof(buffer) - 1) {
      buffer[tamanho++] = c;
    }
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Inicializa o ESC com sinal minimo: o dribbler comeca parado.
  esc.attach(PIN_ESC, ESC_MIN, ESC_MAX);
  esc.writeMicroseconds(ESC_MIN);
  pararDribbler();
  Serial.println("[SETUP] Armando ESC com dribbler parado...");
  delay(4000);
  Serial.println("[SETUP] ESC pronto; dribbler permanece parado.");

  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ENDERECO)) {
    oledDisponivel = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Iniciando goleiro...");
    display.display();
    Serial.println("[SETUP] OLED encontrado");
  } else {
    oledDisponivel = false;
    Serial.println("[ERRO] OLED nao encontrado no endereco 0x3C");
  }

  Serial.println();
  Serial.println("================================");
  Serial.println(" INICIANDO SISTEMA DO GOLEIRO");
  Serial.println("================================");

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

  int pwmPins[] = { M1_PWM, M2_PWM, M3_PWM, M4_PWM };

  for (int i = 0; i < 4; i++) {
    pinMode(pwmPins[i], OUTPUT);
    analogWrite(pwmPins[i], 0);
  }

  Serial.println("[SETUP] Pinos dos motores configurados");

  RPC.begin();
  RPC.bind("anel", receberAnel);

  // Sensores ultrassonicos
  for (int d = 0; d < 4; d++) {
    pinMode(PINOS_TRIG[d], OUTPUT);
    pinMode(PINOS_ECHO[d], INPUT);
    digitalWrite(PINOS_TRIG[d], LOW);
  }

  Serial.println("[SETUP] Ultrassonicos configurados");

  // Botoes
  pinMode(botao, INPUT_PULLUP);
  pinMode(botao2, INPUT_PULLUP);
  pinMode(switchsensor, INPUT_PULLUP);

  Serial.println("[SETUP] Botao 1 e botao 2 configurados");
  Serial.println("[SETUP] Botao 2 LOW = Giroscopio | HIGH = Ring TSOP");

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
  Serial.println("[SETUP] TSOPs prontos (11 sensores = 11 direcoes, 1..11)");
  Serial.println("[SETUP] Sistema iniciado!");
  Serial.println("[SETUP] Botao solto = motores parados");
  Serial.println("[SETUP] Botao pressionado = controle ativo");
  Serial.println("[SETUP] Envie 'h' no Serial para ver os comandos de teste");
  Serial.println("===============================s=");
}

// =====================================================
// ESTRATEGIA DO GOLEIRO
// =====================================================

// ---- Velocidades ----
const int V_DEFESA = 200;
const int V_FUGA = 240;
const int V_ALINHAR_LAT_MAX = 160;
const int V_ALINHAR_LONG_MAX = 95;
const int V_ALINHAR_MIN = 65;
const int KP_ALINHAR_LAT = 7;   // PWM por cm de erro lateral
const int KP_ALINHAR_LONG = 5;  // PWM por cm de erro frente/tras
const int KP_LIMITE_LONG = 8;   // PWM por cm de folga ate o limite
const int V_PICO = 240;
const float ANGULO_MAX_TRANSLACAO = 15.0f;

// ---- Ultrassonicos (cm) ----
const long LAT_PERIGO = 30;
const long LAT_LIBERAR = 32;
const long FRENTE_ALVO = 180;
const long TRAS_ALVO = 40;
const long LATERAL_ALVO = 85;
const long FRENTE_LIMITE = 170;
const long TRAS_LIMITE = 23;
const long TOLERANCIA_ALINHAMENTO = 3;

// ---- Defesa pela direcao fisica do TSOP (1..11) ----
// Numeracao anti-horaria:
//   1      : bola de frente -> parado no eixo lateral
//   2..6   : bola a esquerda -> grupo ESQ
//   7..11  : bola a direita -> grupo DIR
//   -1     : bola nao vista -> alinha pela distancia das paredes
// Lacuna traseira entre 6 e 7; nao existe direcao de sensor nessa lacuna.
const int GK_ESQ_MIN = 2;
const int GK_ESQ_MAX = 6;
const int GK_DIR_MIN = 7;
const int GK_DIR_MAX = 11;
// Se o robo se mover para o lado errado, troque para true.
const bool GK_LADO_INVERTIDO = false;

// Os erros dos dois sensores ja estao no mesmo sentido de movimento.
// Com dois ecos, usa a media. Com apenas um, usa sua referencia.
long combinarErros(long erroA, bool validoA, long erroB, bool validoB) {
  if (validoA && validoB) return (erroA + erroB) / 2;
  if (validoA) return erroA;
  return validoB ? erroB : 0;
}

int pwmAlinhamento(long erro, int ganho, int maximo) {
  if (abs(erro) <= TOLERANCIA_ALINHAMENTO) return 0;
  int pwm = constrain((int)(abs(erro) * ganho), V_ALINHAR_MIN, maximo);
  return erro > 0 ? pwm : -pwm;
}

void defenderComCorrecao() {
  static int sensorFugaLat = -1;
  const long frente = lerFrente();
  const long tras = lerTras();
  const long esquerda = lerEsquerda();
  const long direita = lerDireita();

  float erro = calcularErro(lerOrientacao(), 0.0f);
  if (!bnoProntoParaJogar()) {
    correcaoOrientacaoAtiva = false;
    parar();
    return;
  }

  // Fuga lateral tem prioridade sobre a bola e o alinhamento.
  const long limiarEsq = sensorFugaLat == US_ESQUERDA ? LAT_LIBERAR : LAT_PERIGO;
  const long limiarDir = sensorFugaLat == US_DIREITA ? LAT_LIBERAR : LAT_PERIGO;
  if (esquerda > 0 && esquerda < limiarEsq && direita >= LAT_LIBERAR) {
    sensorFugaLat = US_ESQUERDA;
  } else if (direita > 0 && direita < limiarDir && esquerda >= LAT_LIBERAR) {
    sensorFugaLat = US_DIREITA;
  } else {
    sensorFugaLat = -1;
  }

  // x positivo = direita; y positivo = frente.
  int x = 0;
  int y = 0;
  const int direcao = direcaoFinalTSOP;

  if (sensorFugaLat == US_ESQUERDA) {
    x = V_FUGA;
  } else if (sensorFugaLat == US_DIREITA) {
    x = -V_FUGA;
  } else if (direcao == -1) {
    const long erroLateral = combinarErros(LATERAL_ALVO - esquerda, esquerda > 0,
                                           direita - LATERAL_ALVO, direita > 0);
    x = pwmAlinhamento(erroLateral, KP_ALINHAR_LAT, V_ALINHAR_LAT_MAX);
  } else {
    if (direcao >= GK_ESQ_MIN && direcao <= GK_ESQ_MAX) x = -V_DEFESA;
    if (direcao >= GK_DIR_MIN && direcao <= GK_DIR_MAX) x = V_DEFESA;
    if (GK_LADO_INVERTIDO) x = -x;
  }

  // Com bola, so acompanha no eixo lateral. Sem bola, corrige profundidade.
  // Nao corrige profundidade durante uma fuga lateral.
  if (direcao == -1 && sensorFugaLat == -1) {
    const long erroLongitudinal = combinarErros(frente - FRENTE_ALVO, frente > 0,
                                                TRAS_ALVO - tras, tras > 0);
    y = pwmAlinhamento(erroLongitudinal, KP_ALINHAR_LONG, V_ALINHAR_LONG_MAX);
  }

  // So desloca na direcao de um sensor com eco e espaco disponivel.
  if ((x < 0 && esquerda < LAT_LIBERAR) || (x > 0 && direita < LAT_LIBERAR)) x = 0;
  if (y > 0) {
    const long folga = max(0L, frente - FRENTE_LIMITE);
    y = (int)min((long)y, folga * KP_LIMITE_LONG);
  } else if (y < 0) {
    const long folga = max(0L, tras - TRAS_LIMITE);
    y = -(int)min((long)-y, folga * KP_LIMITE_LONG);
  }

  // Com muito desvio de rumo, primeiro gira para voltar a medir o campo de frente.
  if (fabsf(erro) > ANGULO_MAX_TRANSLACAO) {
    x = 0;
    y = 0;
  }

  int m1 = -y - x;
  int m2 =  y - x;
  int m3 = -y + x;
  int m4 =  y + x;

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
    if (erro > 0.0f) correcao = -correcao;
  }

  // Prioriza a orientacao: reserva espaco no PWM para o giro.
  int picoTranslacao = max(max(abs(m1), abs(m2)), max(abs(m3), abs(m4)));
  int limiteTranslacao = min(V_PICO, 255 - abs(correcao));
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

  // Saturacao final de seguranca.
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

// =====================================================
// LOOP
// =====================================================

void loop() {
  atualizarOrientacaoBNO();
  verificarComandosSerial();

  // Um ultrassonico por vez, com leituras armazenadas.
  atualizarUltrassonicos();
  atualizarImpressaoUltrassom();

  atualizarCacheAnel();
  imprimirDiagnosticoTSOP();
  atualizarDisplayOLED();

  //diagnosticoBNO();

  // Modo teste de motores: ignora o botao de jogo e a estrategia.
  if (modoTesteMotores) {
    atualizarTesteMotores();
    return;
  }

  // Botao solto = parado; somente o botao 2 do setup define o zero.
  if (digitalRead(botao) == HIGH) {
    correcaoOrientacaoAtiva = false;
    parar();
    pararDribbler();
    return;
  }

  // Se o BNO falhar ou perder a referencia, bloquear os motores.
 // if (!bnoProntoParaJogar()) {
  //  correcaoOrientacaoAtiva = false;
  //  parar();
   // pararDribbler();
   // return;
 // }
  //Serial.println(lerTras());
  defenderComCorrecao();
  // O dribbler permanece desligado no goleiro.
 

}
