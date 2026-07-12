#!/usr/bin/env python3
# gerar-icone.py — Gera o ícone do app (arte própria, pixel-art minimalista:
# jato amarelo sobre rio azul entre margens verdes). Sem dependências além
# da stdlib: escreve o PNG na mão (zlib + struct).
#
# Saídas:
#   assets/icone.png                                        (1024×1024, p/ source AltStore)
#   Resources/Assets.xcassets/AppIcon.appiconset/icon1024.png
import struct, zlib, os

GRADE = 32          # desenho em grade 32×32, escalado ×32 → 1024
ESCALA = 32
TAM = GRADE * ESCALA

CARVAO   = (10, 10, 12)
VERDE    = (44, 136, 48)
VERDE_2  = (36, 112, 36)
AZUL     = (32, 56, 200)
AZUL_2   = (28, 48, 176)
AMARELO  = (232, 216, 80)
BRANCO   = (240, 240, 240)

def cor_pixel(x, y):
    # margens verdes com faixas sutis; rio ao centro com leve variação
    faixa = (y // 4) % 2
    if 11 <= x <= 20:
        cor = AZUL if faixa else AZUL_2
    else:
        cor = VERDE if faixa else VERDE_2
    # moldura externa escura (respiro do ícone)
    if x < 2 or x > 29 or y < 2 or y > 29:
        cor = CARVAO
    return cor

# sprite do jato (8×10), desenhado por linhas de bits — mesmo traço do jogo
JATO = [
    0b00011000, 0b00011000, 0b00111100, 0b00111100, 0b01111110,
    0b11111111, 0b11111111, 0b01100110, 0b00100100, 0b00100100,
]
JATO_X, JATO_Y = 12, 16     # topo-esquerda na grade
MISSil_X, MISSIL_Y = 15, 11 # rastro do tiro à frente

def pixels():
    grade = [[cor_pixel(x, y) for x in range(GRADE)] for y in range(GRADE)]
    for r, bits in enumerate(JATO):
        for c in range(8):
            if bits & (0x80 >> c):
                grade[JATO_Y + r][JATO_X + c] = AMARELO
    for r in range(3):
        grade[MISSIL_Y + r][MISSil_X] = BRANCO
        grade[MISSIL_Y + r][MISSil_X + 1] = BRANCO
    return grade

def escrever_png(caminho, grade):
    linhas = bytearray()
    for gy in range(TAM):
        linhas.append(0)   # filtro None
        y = gy // ESCALA
        for gx in range(TAM):
            r, g, b = grade[y][gx // ESCALA]
            linhas += bytes((r, g, b))
    def bloco(tipo, dados):
        return (struct.pack(">I", len(dados)) + tipo + dados +
                struct.pack(">I", zlib.crc32(tipo + dados) & 0xFFFFFFFF))
    ihdr = struct.pack(">IIBBBBB", TAM, TAM, 8, 2, 0, 0, 0)   # RGB 8 bits, sem alfa
    png = (b"\x89PNG\r\n\x1a\n" + bloco(b"IHDR", ihdr) +
           bloco(b"IDAT", zlib.compress(bytes(linhas), 9)) + bloco(b"IEND", b""))
    os.makedirs(os.path.dirname(caminho), exist_ok=True)
    with open(caminho, "wb") as f:
        f.write(png)
    print(f"gerado: {caminho} ({len(png)} bytes)")

if __name__ == "__main__":
    raiz = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    grade = pixels()
    escrever_png(os.path.join(raiz, "assets/icone.png"), grade)
    escrever_png(os.path.join(
        raiz, "Resources/Assets.xcassets/AppIcon.appiconset/icon1024.png"), grade)
