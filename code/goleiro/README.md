# Goleiro — núcleos M4 e M7

[Sketch](goleiro_m4/goleiro_m4.ino) para Arduino GIGA R1 WiFi, núcleo M4.

O código lê 11 TSOPs durante janelas de 50 ms. Um sensor fica ativo quando registra pelo menos 5 mudanças. O maior grupo de sensores vizinhos determina a direção; em empate, vence a maior soma de mudanças. O centro usa média vetorial ponderada pelas mudanças e escolhe o sensor físico mais próximo.

A numeração física é de 1 a 11 em sentido anti-horário. O sensor 1 fica à frente. Os sensores 11 e 1 são vizinhos; os sensores 6 e 7 não são vizinhos devido à lacuna traseira. Os ângulos físicos estão declarados em `ANGULOS_TSOP_GRAUS`.

O Monitor Serial, a 115200 baud, informa sensor, pino, direção, contagem de mudanças, estado e direção final a cada leitura. Essa impressão acrescenta tempo ao ciclo, além da janela de amostragem.

O envio usa `RPC.send("anel", direcao, mascara)`. A direção vai de 1 a 11, ou -1 sem detecção. Os bits 0 a 10 identificam os sensores físicos 1 a 11. Consulte o [protocolo](../protocolo-rpc.md) e a [pinagem](../pinagem-tsop.csv).

O sketch M4 mantém a lógica recebida. O receptor e a movimentação estão no M7 abaixo.

## M7 — defesa e controle

O [goleiro_m7.ino](goleiro_m7/goleiro_m7.ino) registra `RPC.bind("anel", receberAnel)` e recebe direções 1 a 11. Segue a bola lateralmente, usa limites laterais de 30 cm (liberação em 32 cm) e busca manter 60 a 70 cm do fundo. Sem leitura traseira válida, suspende a translação; a correção angular continua. A função `defenderComCorrecao()` verifica o estado do BNO antes de acionar os motores.

O BNO055 opera em IMUPLUS. O OLED apresenta orientação, distâncias, anel TSOP e teste de motores. A leitura real do IR Seeker está comentada em `lerSeeker()`; os valores 10 impressos nessa função são diagnóstico fixo, não medições. O dribbler permanece desligado. O bloqueio adicional de BNO no `loop()` está comentado, embora exista a verificação dentro da função de defesa.

## Monitor Serial — 115200 baud

Finalize cada comando com ENTER / nova linha.

| Comando | Função |
| --- | --- |
| `t` | Entrar/sair do teste de motores |
| `N V` | Em teste, motor 1..4 (0 = todos), velocidade -255..255; exemplo: `2 150` |
| `p` | Parar motores do teste |
| `x` | Sair do teste |
| `u` | Alternar impressão contínua dos ultrassônicos |
| `us` | Medir os quatro ultrassônicos imediatamente |
| `z` | Redefinir zero, com botão de jogo solto e fora do teste, conforme condições do código |
| `h` ou `?` | Mostrar ajuda |

O teste de motores ignora o botão de jogo e a estratégia, com parada automática após 3 segundos sem novo comando. Execute-o com o robô suspenso.

Veja [dependências, gravação e calibração](../README.md). Compilação e testes físicos pendentes.

[Pôster](../../poster/README.md) · [Licença MIT](../../LICENSE) · [Início](../../README.md)
