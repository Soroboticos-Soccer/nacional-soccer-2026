#include <Arduino.h>
#include <RPC.h>
const int NUM_TSOPS = 16;

const int TSOP_LIMIAR_MUDANCAS = 5; //talvez mudar
const int TSOP_TEMPO_JANELA = 50;

int pinosTSOP[NUM_TSOPS] = {
  22, 24, 26, 28, 30,
  32, 34, 36, 38, 40,
  42, 44, 46, 48, 50,
  52
};

// Estado mais recente de cada TSOP e decisao calculada pelo conjunto.
bool tsopDetectado[NUM_TSOPS] = {false};
int direcaoFinalTSOP = -1;
bool tsopsVizinhos(int a, int b) {
  if (b == a + 1) return true;

  // TSOPs 5 e 7: pula o TSOP 6 defeituoso.
  if (a == 4 && b == 6) return true;

  return false;
}
int lerDirecaoTSOP() {

  int mudancas[NUM_TSOPS] = {0};
  int ultimo[NUM_TSOPS];

  for (int i = 0; i < NUM_TSOPS; i++) {
    ultimo[i] = digitalRead(pinosTSOP[i]);
  }

  unsigned long inicio = millis();

  while (millis() - inicio < TSOP_TEMPO_JANELA) {
    for (int i = 0; i < NUM_TSOPS; i++) {
      // Mantem a leitura dos 16 TSOPs durante a janela de amostragem.
      int atual = digitalRead(pinosTSOP[i]);

      if (atual != ultimo[i]) {
        mudancas[i]++;
        ultimo[i] = atual;
      }
    }
  }

  // Identifica quais sensores detectaram a bola.
  // A mesma informacao sera usada no desenho do OLED.
  for (int i = 0; i < NUM_TSOPS; i++) {
    tsopDetectado[i] = (mudancas[i] >= TSOP_LIMIAR_MUDANCAS);
  }


  // Procura o maior grupo coerente de sensores ativos.
  int melhorTamanho = 0;
  int melhorSomaMudancas = -1;
  int melhorDirecao = -1;

  int i = 0;

  while (i < NUM_TSOPS) {
    if (!tsopDetectado[i]) {
      i++;
      continue;
    }

    int inicioGrupo = i;
    int fimGrupo = i;
    int quantidade = 1;
    int somaMudancas = mudancas[i];

    int j = i + 1;

    while (j < NUM_TSOPS) {
      if (!tsopDetectado[j]) {
        j++;
        continue;
      }

      // Se o próximo sensor ativo não for vizinho,
      // encerra este grupo.
      if (!tsopsVizinhos(fimGrupo, j)) {
        break;
      }

      fimGrupo = j;
      quantidade++;
      somaMudancas += mudancas[j];
      j++;
    }

    // Calcula o centro do grupo usando a numeracao 1-based.
    int somaPosicoes = 0;

    for (int k = inicioGrupo; k <= fimGrupo; k++) {
      if (tsopDetectado[k]) {
        somaPosicoes += k + 1;
      }
    }

    // Arredonda o centro para a direcao mais proxima.
    int centro = (somaPosicoes + quantidade / 2) / quantidade;

    // Prioriza o grupo maior; em empate, mais mudancas.
    if (quantidade > melhorTamanho ||
        (quantidade == melhorTamanho &&
         somaMudancas > melhorSomaMudancas)) {
      melhorTamanho = quantidade;
      melhorSomaMudancas = somaMudancas;
      melhorDirecao = centro;
    }

    i = j;
  }

  if (melhorDirecao != -1) {

    // Mantem o retorno zero-based da funcao original.
    direcaoFinalTSOP = melhorDirecao - 1;
    return direcaoFinalTSOP;
  }


  direcaoFinalTSOP = -1;
  return direcaoFinalTSOP;
}
void setup() {
  for (int pino : pinosTSOP) pinMode(pino, INPUT);
  RPC.begin();
}
void loop() {
  int direcao = lerDirecaoTSOP();
  unsigned int mascara = 0;
  for (int i = 0; i < NUM_TSOPS; ++i) {
    if (tsopDetectado[i]) mascara |= 1u << i;
  }
  RPC.send("anel", direcao, mascara);
}
