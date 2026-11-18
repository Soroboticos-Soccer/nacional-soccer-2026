# Atacante — núcleos M4 e M7

[Sketch](atacante_m4/atacante_m4.ino) para Arduino GIGA R1 WiFi, núcleo M4.

O código lê 16 TSOPs durante janelas de 50 ms. Um sensor fica ativo quando registra pelo menos 5 mudanças. O maior grupo coerente determina a direção; em empate, vence a maior soma de mudanças. O centro é a média das posições físicas dos sensores ativos do grupo, arredondada e convertida para índice de 0 a 15.

O envio usa `RPC.send("anel", direcao, mascara)`. A direção vai de 0 a 15, ou -1 sem detecção. Os bits 0 a 15 representam os sensores físicos 1 a 16. Consulte o [protocolo](../protocolo-rpc.md) e a [pinagem](../pinagem-tsop.csv).

## Comportamentos da versão recebida

- A função de vizinhança permite o salto do TSOP 5 ao 7, comentado como tratamento do TSOP 6 defeituoso.
- O TSOP 6 continua sendo lido e pode ser marcado como ativo; ele não é desabilitado pelo código.
- O agrupamento percorre o vetor de forma linear. Os sensores 16 e 1 não são unidos em um único grupo.
- O sketch não inicializa Serial nem desenha no OLED. O comentário sobre OLED não corresponde a uma implementação de display neste arquivo.

Esses comportamentos foram documentados sem alterar a lógica fornecida.

## M7 — movimentação e sensores

O [atacante_m7.ino](atacante_m7/atacante_m7.ino) recebe direção e máscara do M4 por `RPC.bind("anel", receberAnel)`. Controla quatro motores, lê quatro ultrassônicos, acompanha a orientação pelo BNO055 e apresenta diagnóstico no OLED. A estratégia usa direção TSOP, leituras do IR Seeker e afastamento das paredes, somando correção angular aos motores.

O IR Seeker é lido com `InfraredSeeker::ReadAC()`. O ESC é inicializado no pino 8 com pulso mínimo; o anexo contém a função de parada do dribbler, sem comando ativo de aceleração. O botão principal é o pino 55, o botão 2 é o 57 e o seletor de sensor é o 13.

As chamadas `verificarComandosSerial()` e a verificação de BNO no início do modo de jogo estão comentadas no `loop()` recebido. Não há modo de teste Serial equivalente ao do goleiro habilitado neste anexo. O código foi preservado, sem afirmar que essas verificações estejam ativas.

Veja [dependências, gravação e calibração](../README.md). Compilação e testes físicos pendentes.

[Pôster](../../poster/README.md) · [Licença MIT](../../LICENSE) · [Início](../../README.md)
