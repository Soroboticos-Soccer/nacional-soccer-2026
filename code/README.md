# Firmware dos robôs

**Arduino GIGA R1 WiFi · C++ · M4 e M7**

[Início](../README.md) · [Atacante](atacante/README.md) · [Goleiro](goleiro/README.md) · [Pinagem](pinagem-tsop.csv)

O firmware divide a aquisição dos sensores infravermelhos e o controle do robô entre os dois núcleos do GIGA. Cada robô utiliza um sketch **M4** para os TSOPs e um **M7** para movimentação, orientação e interface.

## Organização

| Robô | Aquisição M4 | Controle M7 |
| --- | --- | --- |
| Atacante | [atacante_m4.ino](atacante/atacante_m4/atacante_m4.ino) | [atacante_m7.ino](atacante/atacante_m7/atacante_m7.ino) |
| Goleiro | [goleiro_m4.ino](goleiro/goleiro_m4/goleiro_m4.ino) | [goleiro_m7.ino](goleiro/goleiro_m7/goleiro_m7.ino) |

A comunicação usa RPC dentro da mesma placa. O M4 envia direção e máscara de sensores ativos; o M7 recebe o evento `anel`. Consulte o [protocolo M4 → M7](protocolo-rpc.md).

## Ambiente e bibliotecas

No Arduino IDE, instale o pacote **Arduino Mbed OS Giga Boards** e selecione **Arduino GIGA R1 WiFi**.

| Biblioteca | Uso |
| --- | --- |
| Adafruit Unified Sensor | Interface de sensores |
| Adafruit BNO055 | Orientação e utilitários de IMU |
| Adafruit GFX Library | Elementos gráficos do display |
| Adafruit SSD1306 | OLED I2C |
| [HTInfraredSeeker](https://github.com/psycomp/RambamRoboticsClub/tree/master/Robotics%202016/libraries/HTInfraredSeeker) | Interface HiTechnic, com `InfraredSeeker::ReadAC()` |
| Servo | Controle do ESC; use implementação compatível com GIGA |

Aceite as dependências indicadas pelo gerenciador de bibliotecas. `Arduino.h`, `Wire.h`, `RPC.h` e o ambiente RTOS são fornecidos pelo suporte da placa.

## Gravação

1. Confira a [pinagem TSOP](pinagem-tsop.csv) e os pinos definidos no M7 do robô.
2. Abra o sketch M4, selecione o núcleo **M4**, compile e grave.
3. Abra o sketch M7 do mesmo robô, selecione o núcleo **M7**, compile e grave.
4. Abra o Monitor Serial em **115200 baud**.

Mantenha cada sketch na pasta de mesmo nome. Os dois núcleos são gravados separadamente.

## Inicialização e calibração

O setup inicializa o ESC no pulso mínimo durante quatro segundos e prepara os sensores. Com `calibrar = true`, mantenha o robô parado para calibrar o BNO055 e siga as mensagens no Serial: botão 2 em **HIGH** inicia a calibração; depois, **LOW** define o zero.

No modo de jogo, o botão principal em **HIGH** mantém a tração parada e em **LOW** ativa a estratégia. Verifique o sentido dos motores com o robô suspenso antes do teste em campo.

## Referências de operação

| Recurso | Configuração |
| --- | --- |
| BNO055 | Modo IMUPLUS, referência angular relativa |
| Display | OLED 128 × 64, I2C `0x3C` |
| RPC | Evento `anel`, direção e máscara de bits |
| Validade da amostra IR | 100 ms no receptor M7 |
| TSOPs do atacante | 16 sensores, direção 0–15 |
| TSOPs do goleiro | 11 sensores, direção 1–11 |

As estratégias, parâmetros e comandos de diagnóstico estão nas páginas do [atacante](atacante/README.md) e do [goleiro](goleiro/README.md).

[Eletrônica](../electronics/README.md) · [TDP](../tdp/TDP.md) · [Licença MIT](../LICENSE)
