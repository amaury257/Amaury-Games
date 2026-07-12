# ADR 0002 — D7: Linguagem principal do app

- **Status:** Proposto — aguardando probe da Fase 0 no WSL2.
- **Data:** 2026-07-12

## Contexto

Preferência da spec: Swift 6. Porém o build fecha por Theos no Linux (WSL2),
e compilar Swift→iOS a partir do Linux exige toolchain específica que pode
não estar disponível ou não funcionar com o SDK instalado. A spec proíbe
insistir mais de uma sessão nisso.

## Decisão (proposta)

- `scripts/probe-swift.sh` tenta compilar um `hello.swift` para
  `arm64-apple-ios26.0` com a toolchain do Theos (ou `swiftc` do PATH).
- **Probe OK** ⇒ D7 = **Swift 6** (UI SwiftUI/UIKit conforme a fase);
  status vira **Aceito (Swift)**.
- **Probe falha** ⇒ D7 = **ObjC/UIKit + C++** para todo o app;
  status vira **Aceito (ObjC)**.

Em ambos os casos a ponte com os cores segue em Objective-C++ (D8), e o app
"Olá" da Fase 0 permanece ObjC — é o caminho garantido para validar o gate
independentemente do resultado.

## Consequências

- Swift: código de jogo/UI em Swift tipado, interop com `LibretroCore.mm`
  via header bridging do Theos.
- ObjC: disciplina extra (nullability, generics leves, ARC); nenhuma perda
  funcional — todo o produto é viável em ObjC.
