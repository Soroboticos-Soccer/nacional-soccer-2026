# Goleiro

**Acompanhamento lateral e posicionamento defensivo · Arduino GIGA R1 WiFi**

[Início](../../README.md) · [Firmware](../README.md) · [Atacante](../atacante/README.md)

O goleiro acompanha a bola lateralmente, mantém uma faixa de distância do fundo e corrige sua orientação durante a defesa.

## Sketches

| Núcleo | Arquivo | Responsabilidade |
| --- | --- | --- |
| M4 | [goleiro_m4.ino](goleiro_m4/goleiro_m4.ino) | Leitura dos 11 TSOPs e diagnóstico Serial |
| M7 | [goleiro_m7.ino](goleiro_m7/goleiro_m7.ino) | Defesa, orientação, motores e interface |

## Percepção da bola

Os sensores são numerados de **1 a 11**, em sentido anti-horário, com o sensor 1 à frente. Os sensores 11 e 1 são vizinhos; a lacuna traseira separa os sensores 6 e 7.

O M4 lê janelas de **50 ms** e considera ativo o sensor com pelo menos cinco mudanças. A direção vem do maior grupo de vizinhos, com desempate pela soma de mudanças. A média vetorial ponderada seleciona o sensor físico mais próximo.

O RPC transmite direção **1–11**, ou **-1** sem detecção, e máscara de bits dos sensores ativos. O Serial em **115200 baud** mostra sensor, pino, contagens e direção final.

[Pinagem dos TSOPs](../pinagem-tsop.csv) · [Protocolo RPC](../protocolo-rpc.md)

## Estratégia de defesa

| Recurso | Comportamento |
| --- | --- |
| Bola à esquerda ou à direita | Deslocamento lateral para acompanhamento |
| Bola à frente | Sem deslocamento lateral de acompanhamento |
| Proteção lateral | Afastamento abaixo de 30 cm; liberação em 32 cm |
| Posicionamento traseiro | Faixa de referência entre 60 e 70 cm do fundo |
| Ausência de eco traseiro | Suspensão da translação |
| Orientação | Correção pelo BNO055 em IMUPLUS |

A função de defesa verifica o estado do BNO055 antes de acionar os motores. O dribbler permanece desligado. O OLED apresenta orientação, distâncias, sensores IR e diagnóstico dos motores.

## Diagnóstico pelo Serial

Use **115200 baud** e finalize os comandos com ENTER ou nova linha.

| Comando | Função |
| --- | --- |
| `t` | Entrar ou sair do teste de motores |
| `N V` | Motor 1–4 ou 0 para todos; velocidade de -255 a 255. Exemplo: `2 150` |
| `p` | Parar os motores em teste |
| `x` | Encerrar o teste |
| `u` | Alternar impressão contínua dos ultrassônicos |
| `us` | Medir os quatro ultrassônicos |
| `z` | Redefinir o zero nas condições previstas pelo código |
| `h` ou `?` | Mostrar ajuda |

Execute o teste de motores com o robô suspenso: esse modo controla a tração independentemente do botão de jogo e interrompe o comando após três segundos sem atualização.

## Preparação

Siga as [instruções de dependências, gravação e calibração](../README.md). As constantes do M7 concentram as velocidades, distâncias e ganhos de correção.

[Eletrônica](../../electronics/README.md) · [TDP](../../tdp/TDP.md) · [Licença MIT](../../LICENSE)
