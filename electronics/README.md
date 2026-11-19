# Eletrônica e materiais

**Sorobóticos · Nacional Soccer 2026**

[Início](../README.md) · [TDP](../tdp/TDP.md) · [Programação](../code/README.md)

A eletrônica integra percepção infravermelha, orientação, medição de distância e acionamento dos motores. Os arquivos abaixo apresentam os registros da planilha de materiais da equipe de **18 de novembro de 2026**.

## Lista de materiais

| Arquivo | Conteúdo |
| --- | --- |
| [bom.csv](bom.csv) | 20 componentes e materiais de hardware, com modelos, fornecedores, quantidades e custos em BRL e USD |
| [ferramentas-software.csv](ferramentas-software.csv) | 9 registros de ferramentas de fabricação e bibliotecas |
| [bom-referencia.csv](bom-referencia.csv) | Equipe, moeda, cotação e data informadas na planilha |

Os CSVs usam **UTF-8**, vírgula como separador e ponto decimal. Quantidades com unidades e valores não preenchidos seguem os registros da planilha. As colunas de custos em dólares preservam os valores fornecidos.

## Subsistemas

| Subsistema | Componentes e função |
| --- | --- |
| Processamento | Arduino GIGA R1 WiFi; aquisição no M4 e controle no M7 |
| Detecção da bola | TSOP4838 e interface HiTechnic IR Seeker |
| Orientação | BNO055 |
| Distâncias | Sensores ultrassônicos |
| Tração | Quatro motores DC e duas pontes H L298N |
| Alimentação | Bateria LiPo, regulador de tensão, conectores XT60 e interruptores |
| Interface | Display OLED I2C |

## Integração

Consulte a [pinagem TSOP](../code/pinagem-tsop.csv), o [protocolo entre núcleos](../code/protocolo-rpc.md) e os sketches M7 para as conexões de motores, ultrassônicos, botões e display de cada robô.

O [TDP](../tdp/TDP.md#diagrama-da-eletrônica) apresenta o diagrama eletrônico e a descrição dos circuitos. Os [projetos mecânicos](../cad/README.md) incluem os suportes dos sensores.

[Início](../README.md) · [Licença MIT](../LICENSE)
