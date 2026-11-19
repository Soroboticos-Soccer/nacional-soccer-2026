# Mecânica e CAD

**Chassi, rodas e suportes · Sorobóticos 2026**

[Início](../README.md) · [TDP](../tdp/TDP.md) · [Materiais](../electronics/README.md)

Os projetos mecânicos organizam os componentes dos robôs e facilitam o acesso para manutenção. O desenvolvimento combina modelagem no **Fusion 360**, prototipagem e fabricação por impressão 3D.

## Arquivos do projeto

| Arquivo | Aplicação | Formato |
| --- | --- | --- |
| [Chassi inferior](chassi%20inferior%20v8.2.f3d) | Base e estrutura inferior | Fusion 360 |
| [Chassi inferior para impressão](chassi%20inferior%20v8.2.stl) | Malha do chassi inferior | STL |
| [Chassi superior e encaixes](chassi%20superior%20encaixes%202.3.f3d) | Estrutura superior | Fusion 360 |
| [Estrutura intermediária](meio%20soccer.f3d) | Parte central do conjunto | Fusion 360 |
| [Topo](Topo_Soccer_v1.f3d) | Estrutura de fechamento | Fusion 360 |
| [Suporte TSOP do atacante](TSOP%20new.f3d) | Posicionamento dos sensores IR | Fusion 360 |
| [Suporte TSOP do goleiro](TSOP%20new_goleiro.f3d) | Posicionamento dos sensores IR | Fusion 360 |
| [Montagem da roda](Montagem_roda_soccer.step) | Roda omnidirecional | STEP |
| [Suporte ultrassônico](suporte%20Ultrassonico%20pronto.stl) | Fixação do sensor de distância | STL |

## Desenvolvimento e fabricação

O chassi utiliza encaixes tipo **cauda de andorinha** para facilitar a manutenção. As rodas duplas combinam estrutura em ABS, roletes deslizantes em resina ABS Like, roletes flexíveis e eixos de aço trefilado.

Para reproduzir uma peça:

1. Abra o arquivo Fusion 360 ou STEP em um CAD compatível.
2. Confira dimensões, encaixes e orientação da montagem.
3. Exporte a malha e prepare o fatiamento no OrcaSlicer.
4. Valide o encaixe com uma impressão parcial antes de fabricar o conjunto.

A descrição dos materiais, espessuras, protótipos e testes está na seção de [design mecânico do TDP](../tdp/TDP.md#design-mecânico).

[Início](../README.md) · [Licença MIT](../LICENSE)
