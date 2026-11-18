# Programações dos robôs

Códigos fornecidos pela equipe para Arduino GIGA R1 WiFi, organizados em sketches separados por núcleo. Os M7s foram adicionados integralmente a partir dos anexos, sem alterar sua lógica; os M4s existentes foram mantidos.

| Robô | M4 — leitura | M7 — controle | Direção RPC |
| --- | --- | --- | --- |
| Atacante | [atacante_m4.ino](atacante/atacante_m4/atacante_m4.ino) | [atacante_m7.ino](atacante/atacante_m7/atacante_m7.ino) | 0 a 15; -1 sem detecção |
| Goleiro | [goleiro_m4.ino](goleiro/goleiro_m4/goleiro_m4.ino) | [goleiro_m7.ino](goleiro/goleiro_m7/goleiro_m7.ino) | 1 a 11; -1 sem detecção |

## Dependências

Use o suporte de placa Arduino GIGA R1 WiFi (Arduino Mbed OS Giga Boards), com `Arduino.h`, `RPC.h`, `Wire.h` e o ambiente RTOS utilizado pelo RPC. Para os M7s, instale as bibliotecas que fornecem:

- `Adafruit_Sensor.h` — Adafruit Unified Sensor;
- `Adafruit_BNO055.h` e `utility/imumaths.h` — Adafruit BNO055;
- `Adafruit_GFX.h` — Adafruit GFX Library;
- `Adafruit_SSD1306.h` — Adafruit SSD1306;
- `HTInfraredSeeker.h` — biblioteca HiTechnic IR Seeker compatível com `InfraredSeeker::ReadAC()`;
- `Servo.h` — implementação compatível com o GIGA.

Instale também as dependências solicitadas pelo gerenciador de bibliotecas. As versões das bibliotecas não foram informadas nos anexos; a compilação ainda não foi validada.

## Gravação e inicialização

1. Confira a [pinagem dos TSOPs](pinagem-tsop.csv) e as definições de motores, botões e ultrassônicos no M7 correspondente. A pinagem difere entre atacante e goleiro.
2. Abra o `.ino` do M4 no Arduino IDE, selecione Arduino GIGA R1 WiFi e o núcleo M4, compile e grave.
3. Abra o `.ino` do M7 do mesmo robô em sua própria pasta, selecione o núcleo M7, compile e grave. Cada pasta tem o mesmo nome de seu sketch; não una M4 e M7 em um único arquivo.
4. Abra o Monitor Serial a 115200 baud. A inicialização mantém o ESC no mínimo durante 4 segundos e depois inicializa os sensores.
5. Com `calibrar = true`, mantenha o robô parado para calibrar o BNO055. O setup aguarda o botão 2 em HIGH para iniciar e depois em LOW para definir o zero, conforme as mensagens do programa.
6. No modo de jogo, botão principal em HIGH mantém os motores parados; LOW ativa a estratégia. Confira o comportamento dos motores com o robô suspenso antes dos testes em campo.

O BNO055 usa IMUPLUS, com referência relativa e possibilidade de drift. O OLED é 128 × 64, endereço I2C `0x3C`. O M7 recebe o RPC `anel` e invalida amostras com mais de 100 ms. Consulte o [protocolo](protocolo-rpc.md).

As particularidades dos anexos, inclusive trechos de diagnóstico e verificações comentadas, estão descritas nos READMEs do [atacante](atacante/README.md) e do [goleiro](goleiro/README.md). A presença dos quatro arquivos não representa validação de partida: compilação e testes físicos permanecem pendentes.

[Pôster](../poster/README.md) · [Eletrônica](../electronics/README.md) · [Licença MIT](../LICENSE) · [Início](../README.md)
