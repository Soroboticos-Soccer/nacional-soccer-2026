# Goleiro — leitura de TSOPs no M4

[Sketch](goleiro_m4/goleiro_m4.ino) para Arduino GIGA R1 WiFi, núcleo M4.

O código lê 11 TSOPs durante janelas de 50 ms. Um sensor fica ativo quando registra pelo menos 5 mudanças. O maior grupo de sensores vizinhos determina a direção; em empate, vence a maior soma de mudanças. O centro usa média vetorial ponderada pelas mudanças e escolhe o sensor físico mais próximo.

A numeração física é de 1 a 11 em sentido anti-horário. O sensor 1 fica à frente. Os sensores 11 e 1 são vizinhos; os sensores 6 e 7 não são vizinhos devido à lacuna traseira. Os ângulos físicos estão declarados em `ANGULOS_TSOP_GRAUS`.

O Monitor Serial, a 115200 baud, informa sensor, pino, direção, contagem de mudanças, estado e direção final a cada leitura. Essa impressão acrescenta tempo ao ciclo, além da janela de amostragem.

O envio usa `RPC.send("anel", direcao, mascara)`. A direção vai de 1 a 11, ou -1 sem detecção. Os bits 0 a 10 identificam os sensores físicos 1 a 11. Consulte o [protocolo](../protocolo-rpc.md) e a [pinagem](../pinagem-tsop.csv).

O sketch não contém o receptor M7 nem a lógica de movimentação. A lógica recebida foi preservada.
