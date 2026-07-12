# Controles

## Touch (§7.12)

- **Manche virtual flutuante** (preferência do usuário): toque em qualquer
  ponto da metade **esquerda** da tela — o manche nasce ali. Arraste até
  44 pt de raio; solte para centrar.
  - **Eixo vertical = acelerador** (mecânica-assinatura do original):
    cima acelera até 2,0×, baixo desacelera até 0,5×.
  - Eixo horizontal: deslocamento lateral, resposta imediata, sem inércia.
- **Fogo**: qualquer toque na metade **direita** (há um círculo-guia no canto
  inferior). Segurar dispara em cadência fixa; máx. 2 mísseis na tela.
- **Pausa**: botão `II` no canto superior direito.
- Haptics leves (Core Haptics): tiro, explosão e vida extra.

## Gamepad Bluetooth (GCController)

Suporte a MFi/Xbox/DualSense via perfil estendido:

| Controle | Ação |
|---|---|
| Analógico esquerdo / D-pad | Manche (Y = acelerador) |
| A | Fogo |
| Menu | Pausa |

Touch e gamepad são mesclados a cada quadro (vence o maior deslocamento);
o polling acontece imediatamente antes de cada `executarQuadro`.
A tela de remapeamento chega na Fase 5.

## Tutorial de 1 tela

Pendente (§7.3 exige comunicar o acelerador no manche) — entra no polimento
da Fase 1 junto com a tela de Configurações.
