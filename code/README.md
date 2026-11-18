# Programações dos robôs

Os dois sketches abaixo foram fornecidos pela equipe e mantêm a lógica recebida.

| Robô | Sketch | Núcleo | TSOPs | Direção enviada |
| --- | --- | --- | --- | --- |
| Goleiro | [goleiro_m4.ino](goleiro/goleiro_m4/goleiro_m4.ino) | M4 | 11 | 1 a 11; -1 sem detecção |
| Atacante | [atacante_m4.ino](atacante/atacante_m4/atacante_m4.ino) | M4 | 16 | 0 a 15; -1 sem detecção |

## Uso

1. Abra o arquivo `.ino` correspondente no Arduino IDE. Cada sketch está em uma pasta com o mesmo nome do arquivo.
2. Use a configuração da placa Arduino GIGA R1 WiFi com o núcleo M4 selecionado.
3. Confira a [pinagem](pinagem-tsop.csv) com as conexões do robô.
4. Compile e grave o sketch correspondente.
5. No goleiro, abra o Monitor Serial a 115200 baud para o diagnóstico individual.

Os arquivos incluem `Arduino.h` e `RPC.h`; o goleiro também inclui `math.h`. Use o suporte da placa GIGA que disponibiliza esses cabeçalhos. Estes sketches não incluem BNO055, OLED nem acionamento dos motores.

O receptor do M7 e o controle de movimentação não foram fornecidos nesta publicação. A leitura dos sensores e o envio RPC não constituem a programação completa de partida. Consulte o [protocolo RPC](protocolo-rpc.md) e as notas do [goleiro](goleiro/README.md) e do [atacante](atacante/README.md).

Compilação para a placa e testes no hardware ainda não foram executados nesta publicação.
