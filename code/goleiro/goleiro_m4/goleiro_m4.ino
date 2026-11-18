// =====================================================
// GOLEIRO - RoboCup Junior (nucleo M4)
// Diagnostico individual dos 11 TSOPs pelo Serial
//
// O robo tem 11 TSOPs, numerados de 1 a 11 em sentido anti-horario.
// TSOP 1: frente; 2..6: lado esquerdo; 7..11: lado direito.
// A traseira nao tem sensores: a lacuna fica entre 6 e 7.
// Sensores 11 e 1 sao vizinhos na frente; 6 e 7 NAO sao vizinhos.
//
// O M7 recebe:
// - Direcao: 1..11 ou -1 (bola nao vista)
// - Mascara: 11 bits, bit k = TSOP fisico k + 1 detectando
// =====================================================

#include <Arduino.h>
#include <RPC.h>
#include <math.h>

const int NUM_TSOPS = 11;      // 11 sensores = direcoes fisicas 1..11

const int TSOP_LIMIAR_MUDANCAS = 5;
const int TSOP_TEMPO_JANELA = 50;

// Pino de cada TSOP fisico 1..11; indice interno 0..10.
const int pinosTSOP[NUM_TSOPS] = {
  31, 33, 35, 39, 41, 43,  // TSOPs 1..6
  29, 37, 25, 23, 27       // TSOPs 7..11
};

// Angulos em graus a partir da frente, positivos no sentido horario.
// Numeracao fisica 1..11 em sentido ANTI-HORARIO; indice = numero - 1.
// A lacuna traseira fica entre os sensores 6 e 7.
const float ANGULOS_TSOP_GRAUS[NUM_TSOPS] = {
  0.0f, 337.5f, 315.0f, 292.5f, 270.0f, 247.5f,
  112.5f, 90.0f, 67.5f, 45.0f, 22.5f
};

// Estado por TSOP: indice k corresponde ao sensor fisico k + 1.
bool tsopDetectado[NUM_TSOPS] = {false};

int direcaoFinalTSOP = -1;


// =====================================================
// VERIFICA SE DOIS TSOPS (INDICES) SAO VIZINHOS
// =====================================================
// Indices internos: consecutivos, com dobra 10 <-> 0 (sensores 11 e 1).
// Indices 5 e 6 (sensores 6 e 7): lacuna traseira, NAO sao vizinhos.

bool tsopsVizinhos(int a, int b) {

  int d = (b - a + NUM_TSOPS) % NUM_TSOPS;

  if (d != 1 && d != NUM_TSOPS - 1) {
    return false;
  }

  // Identifica quem vem antes na ordem da numeracao anti-horaria.
  int anterior = (d == 1) ? a : b;

  return anterior != 5;
}


// =====================================================
// IMPRESSAO INDIVIDUAL DOS TSOPS NO SERIAL
// =====================================================

void imprimirLeituraTSOP(const int mudancas[]) {

  Serial.println();
  Serial.println("========== LEITURA DOS TSOPS ==========");

  for (int k = 0; k < NUM_TSOPS; k++) {

    Serial.print("TSOP ");
    Serial.print(k + 1);

    Serial.print(" | Pino: ");
    Serial.print(pinosTSOP[k]);

    Serial.print(" | Direcao: ");
    Serial.print(k + 1);

    Serial.print(" | Mudancas: ");
    Serial.print(mudancas[k]);

    Serial.print(" | Estado: ");

    if (tsopDetectado[k]) {
      Serial.println("DETECTANDO");
    } else {
      Serial.println("NAO DETECTANDO");
    }
  }

  Serial.print("DIRECAO FINAL (1..11): ");
  Serial.println(direcaoFinalTSOP);

  Serial.println("=======================================");
}


// =====================================================
// LEITURA E CALCULO DA DIRECAO DOS TSOPS
// =====================================================

int lerDirecaoTSOP() {

  int mudancas[NUM_TSOPS] = {0};
  int ultimo[NUM_TSOPS];

  // Leitura inicial
  for (int k = 0; k < NUM_TSOPS; k++) {
    ultimo[k] = digitalRead(pinosTSOP[k]);
  }

  unsigned long inicio = millis();

  // Observa os sensores durante a janela de leitura
  while (millis() - inicio < TSOP_TEMPO_JANELA) {

    for (int k = 0; k < NUM_TSOPS; k++) {

      int atual = digitalRead(pinosTSOP[k]);

      if (atual != ultimo[k]) {

        mudancas[k]++;

        ultimo[k] = atual;
      }
    }
  }

  // Determina quais sensores estao ativos
  for (int k = 0; k < NUM_TSOPS; k++) {
    tsopDetectado[k] = (mudancas[k] >= TSOP_LIMIAR_MUDANCAS);
  }


  // ===================================================
  // AGRUPAMENTO DOS SENSORES ATIVOS VIZINHOS
  // ===================================================

  int grupo[NUM_TSOPS];

  for (int k = 0; k < NUM_TSOPS; k++) {
    grupo[k] = -1;
  }

  int numGrupos = 0;
  int melhorTamanho = 0;
  int melhorSomaMudancas = -1;
  int melhorDirecao = -1;

  for (int p = 0; p < NUM_TSOPS; p++) {

    if (!tsopDetectado[p] || grupo[p] != -1) {
      continue;
    }

    int id = numGrupos++;

    grupo[p] = id;

    // Expande o grupo ate nao encontrar mais vizinhos
    bool mudou = true;

    while (mudou) {

      mudou = false;

      for (int q = 0; q < NUM_TSOPS; q++) {

        if (!tsopDetectado[q] || grupo[q] != -1) {
          continue;
        }

        for (int r = 0; r < NUM_TSOPS; r++) {

          if (grupo[r] == id && tsopsVizinhos(r, q)) {

            grupo[q] = id;
            mudou = true;

            break;
          }
        }
      }
    }


    // =================================================
    // CALCULO DO CENTRO DO GRUPO
    // =================================================
    // Media vetorial usando o angulo FISICO de cada sensor; o
    // resultado e convertido para o sensor mais proximo (1..11).

    int quantidade = 0;
    int somaMudancas = 0;

    float sx = 0.0f;
    float sy = 0.0f;

    for (int q = 0; q < NUM_TSOPS; q++) {

      if (grupo[q] != id) {
        continue;
      }

      quantidade++;
      somaMudancas += mudancas[q];

      float ang = ANGULOS_TSOP_GRAUS[q] * PI / 180.0f;

      sx += mudancas[q] * sinf(ang);
      sy += mudancas[q] * cosf(ang);
    }

    float angMedio = atan2f(sx, sy);
    if (angMedio < 0.0f) angMedio += 2.0f * PI;

    int centro = 0;
    float menorDif = 1000.0f;

    for (int k = 0; k < NUM_TSOPS; k++) {

      float angK = ANGULOS_TSOP_GRAUS[k] * PI / 180.0f;
      float dif = fabsf(angMedio - angK);
      if (dif > PI) dif = 2.0f * PI - dif;

      if (dif < menorDif) {
        menorDif = dif;
        centro = k;
      }
    }


    // Prioriza o grupo maior.
    // Em empate, escolhe o grupo com mais mudancas.

    if (
      quantidade > melhorTamanho ||
      (quantidade == melhorTamanho &&
       somaMudancas > melhorSomaMudancas)
    ) {

      melhorTamanho = quantidade;
      melhorSomaMudancas = somaMudancas;
      melhorDirecao = centro + 1;  // indice interno -> numero fisico 1..11
    }
  }


  // ===================================================
  // DIRECAO FINAL (1..11 ou -1)
  // ===================================================

  direcaoFinalTSOP = melhorDirecao;

  // Imprime diagnostico individual
  imprimirLeituraTSOP(mudancas);

  return direcaoFinalTSOP;
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  for (int k = 0; k < NUM_TSOPS; k++) {
    pinMode(pinosTSOP[k], INPUT);
  }

  RPC.begin();

  Serial.println();
  Serial.println("=======================================");
  Serial.println("   DIAGNOSTICO DOS TSOPS INICIADO");
  Serial.println("   11 sensores = 11 direcoes (1..11)");
  Serial.println("   Monitor Serial: 115200 baud");
  Serial.println("=======================================");
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  int direcao = lerDirecaoTSOP();

  // Monta mascara de 11 bits
  // Bit k representa o TSOP fisico k + 1 (bits 0..10).

  unsigned int mascara = 0;

  for (int k = 0; k < NUM_TSOPS; k++) {

    if (tsopDetectado[k]) {
      mascara |= 1u << k;
    }
  }

  // Envia dados ao M7
  RPC.send("anel", direcao, mascara);
}
