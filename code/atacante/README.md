# Atacante — leitura de TSOPs no M4

[Sketch](atacante_m4/atacante_m4.ino) para Arduino GIGA R1 WiFi, núcleo M4.

O código lê 16 TSOPs durante janelas de 50 ms. Um sensor fica ativo quando registra pelo menos 5 mudanças. O maior grupo coerente determina a direção; em empate, vence a maior soma de mudanças. O centro é a média das posições físicas dos sensores ativos do grupo, arredondada e convertida para índice de 0 a 15.

O envio usa `RPC.send("anel", direcao, mascara)`. A direção vai de 0 a 15, ou -1 sem detecção. Os bits 0 a 15 representam os sensores físicos 1 a 16. Consulte o [protocolo](../protocolo-rpc.md) e a [pinagem](../pinagem-tsop.csv).

## Comportamentos da versão recebida

- A função de vizinhança permite o salto do TSOP 5 ao 7, comentado como tratamento do TSOP 6 defeituoso.
- O TSOP 6 continua sendo lido e pode ser marcado como ativo; ele não é desabilitado pelo código.
- O agrupamento percorre o vetor de forma linear. Os sensores 16 e 1 não são unidos em um único grupo.
- O sketch não inicializa Serial nem desenha no OLED. O comentário sobre OLED não corresponde a uma implementação de display neste arquivo.

Esses comportamentos foram documentados sem alterar a lógica fornecida. O receptor M7 e o controle de movimentação não foram incluídos.
