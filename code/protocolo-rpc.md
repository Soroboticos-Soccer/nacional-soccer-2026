# Comunicação M4 → M7

Ambos os sketches iniciam `RPC.begin()` e enviam:

```cpp
RPC.send("anel", direcao, mascara);
```

| Campo | Goleiro | Atacante |
| --- | --- | --- |
| Nome do envio | `anel` | `anel` |
| `direcao` (`int`) | Sensor físico 1 a 11 | Índice 0 a 15 |
| Ausência de detecção | -1 | -1 |
| `mascara` (`unsigned int`) | Bits 0 a 10 | Bits 0 a 15 |
| Bit k ativo | Sensor físico k + 1 detectando | Sensor físico k + 1 detectando |

A máscara contém todos os sensores ativos, incluindo os que não pertencem ao grupo escolhido para a direção final.

Exemplos: apenas o sensor físico 1 ativo produz direção 1 e máscara 1 no goleiro, mas direção 0 e máscara 1 no atacante. Nenhum sensor ativo produz direção -1 e máscara 0 em ambos.

A janela de leitura é de 50 ms. O processamento e, no goleiro, a impressão Serial acrescentam tempo ao ciclo; o envio não tem período fixo garantido de 50 ms.

Os receptores estão publicados no [M7 do atacante](atacante/atacante_m7/atacante_m7.ino) e no [M7 do goleiro](goleiro/goleiro_m7/goleiro_m7.ino). Ambos registram `RPC.bind("anel", receberAnel)` e copiam os dados sob um mutex para o cache do loop. Amostras com mais de `2 * TSOP_TEMPO_JANELA` (100 ms) são convertidas para direção -1 e máscara 0. O goleiro também valida o intervalo 1..11. A integração respeita a diferença de numeração entre os robôs. Esse RPC é comunicação entre núcleos da mesma placa, não entre os dois robôs.

[Voltar às programações](README.md)
