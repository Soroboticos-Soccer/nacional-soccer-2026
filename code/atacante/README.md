# Atacante

**Busca, condução e finalização · Arduino GIGA R1 WiFi**

[Início](../../README.md) · [Firmware](../README.md) · [Goleiro](../goleiro/README.md)

O atacante combina a direção da bola com orientação angular e medição de distâncias para executar sua movimentação.

## Sketches

| Núcleo | Arquivo | Responsabilidade |
| --- | --- | --- |
| M4 | [atacante_m4.ino](atacante_m4/atacante_m4.ino) | Leitura dos 16 TSOPs e envio por RPC |
| M7 | [atacante_m7.ino](atacante_m7/atacante_m7.ino) | Estratégia, motores, BNO055, ultrassônicos e OLED |

## Percepção da bola

O M4 lê os sensores em janelas de **50 ms**. Um TSOP é considerado ativo a partir de cinco mudanças de estado. O maior grupo coerente define a direção; em empate, prevalece a maior soma de mudanças.

A direção é calculada pela média das posições físicas do grupo e convertida para **0–15**. O valor **-1** indica ausência de detecção. A máscara usa os bits 0–15 para representar os sensores físicos 1–16.

O agrupamento percorre o vetor linearmente, com tratamento de vizinhança entre os sensores 5 e 7. O sensor 6 participa da leitura; os sensores 16 e 1 são tratados em grupos separados.

[Pinagem dos TSOPs](../pinagem-tsop.csv) · [Protocolo RPC](../protocolo-rpc.md)

## Controle e interface

O M7 recebe o evento `anel` e combina a direção TSOP, o HiTechnic IR Seeker e as distâncias dos quatro ultrassônicos. A correção angular pelo BNO055 é somada ao comando dos quatro motores. O OLED apresenta informações de operação.

| Entrada ou saída | Pino |
| --- | --- |
| Botão principal | 55 |
| Botão 2 | 57 |
| Seletor de sensor | 13 |
| ESC | 8 |

O ESC inicia com pulso mínimo. A configuração de jogo utiliza a movimentação do robô para conduzir a bola.

## Preparação

Siga as [instruções de dependências, gravação e calibração](../README.md). Consulte as constantes no M7 para ajustar velocidades, limites de distância e orientação à montagem.

[Eletrônica](../../electronics/README.md) · [TDP](../../tdp/TDP.md) · [Licença MIT](../../LICENSE)
