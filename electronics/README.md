# Eletrônica e lista de materiais

A [BOM completa em CSV](bom.csv) contém os 24 itens da aba **BOM completa** da planilha `BOM_Soroboticos_RoboCupJunior_2026(1).xlsx`, baseada na relação enviada pela equipe em 16/11/2026.

O CSV usa UTF-8, vírgula como delimitador, ponto decimal e aspas em campos que contêm vírgulas. Os preços e subtotais estão em reais. Quantidades e preços não informados permanecem vazios. O arquivo contém somente o cabeçalho e os itens, sem linhas decorativas ou total que duplicaria a soma dos registros.

## Totais e escopo

São 21 itens precificados, somando **R$ 3.397,10**, e 3 itens pendentes de cotação. A soma confere com o total parcial da planilha. A aba de resumo para banner apresenta uma seleção desses mesmos itens e não deve ser somada novamente.

O [TDP](../tdp/TDP.md) declara R$ 3.197,10 por robô. A diferença de **R$ 200,00** já está apontada na planilha. Os valores foram preservados; ainda é necessário confirmar o escopo da compra, a resina Flex e os itens pendentes.

Robocore é o fornecedor informado pela equipe, sem verificação de nota fiscal ou orçamento. A lista não confirma, por si só, quais peças estão instaladas em cada robô.

## Conferências registradas na planilha

| Prioridade | Conferência | Origem | Estado |
| --- | --- | --- | --- |
| Alta | Validar se R$ 3.397,10 é por robô ou por aquisição/lote. | Feedback nº 3 e nº 17; lista atual | Pendente |
| Alta | Justificar diferença de R$ 200,00 frente aos R$ 3.197,10 citados anteriormente. | Feedback nº 17; lista atual | Pendente |
| Alta | Conferir se HiTechnic IR Seeker permanece instalado ou é legado. | Desenho atual x contexto de sensores | Pendente |
| Alta | Conferir se 4 ultrassônicos estão na configuração final. | Desenho atual x contexto anterior | Pendente |
| Alta | Incluir preço e quantidade da resina ABS Like. | Feedback nº 16 | Pendente |
| Alta | Incluir suporte/montante para spinner (com custo). | Feedback nº 16 | Pendente |
| Alta | Incluir arruelas para fixar roletes (com custo). | Feedback nº 16 | Pendente |
| Média | Conferir quantidade comprada x quantidade consumida (ABS, resina, arame). | Boa prática de BOM reproduzível | Pendente |
| Média | Conferir preços unitários e fornecedores por nota fiscal ou orçamento. | Feedback nº 15 | Pendente |
| Média | Explicar função dos principais subsistemas no banner. | Feedback do pôster | Em elaboração |
| Média | Referenciar tabela completa por QR code e legenda/citação. | Feedback nº 3 e nº 5 | Em elaboração |

A planilha recomenda registrar data da cotação, moeda, quantidades por robô e distinguir peças de protótipo das peças instaladas. Os comentários específicos por item estão preservados no CSV.

## Programações e conexões

- [Programações dos robôs](../code/README.md)
- [Pinagem dos TSOPs](../code/pinagem-tsop.csv)
- [Protocolo M4 → M7](../code/protocolo-rpc.md)

O goleiro fornecido usa 11 TSOPs e o atacante usa 16. A quantidade de 16 na BOM foi mantida como consta na planilha.

## Pôster e controle M7

O [pôster original](../poster/README.md) foi publicado junto com os [M7s do atacante e do goleiro](../code/README.md). Ambos os M7s declaram quatro ultrassônicos, BNO055, OLED e controle de quatro motores, com pinagens próprias. Isso documenta a configuração do software recebido, sem confirmar a instalação física dos componentes.

No atacante, o IR Seeker é lido por `InfraredSeeker::ReadAC()`; no goleiro, essa leitura está comentada. A pendência sobre sua presença física permanece aberta. O pôster apresenta R$ 3.439,10, R$ 42,00 acima do total parcial da BOM; esse terceiro valor também precisa ser conciliado com o escopo da compra. Os valores originais do CSV e do TDP foram preservados.

As referências ao banner como “Em elaboração” na tabela acima registram o estado da planilha de origem. O pôster agora está publicado; essa publicação não resolve as conferências financeiras pendentes.

[Licença MIT](../LICENSE) · [Início](../README.md)
