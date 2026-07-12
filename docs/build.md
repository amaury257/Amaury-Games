# Build — Theos/WSL2 → .ipa → AltStore

> Todo o build fecha no WSL2 (Ubuntu), sem Mac e sem Xcode. Passos que exigiriam
> Xcode não existem neste fluxo; se algum surgir, será marcado `⚠️ REQUER MAC`
> com alternativa Theos.

## Pré-requisitos (uma vez só)

### Instalando o Theos no WSL2

```bash
sudo apt update
sudo apt install -y build-essential fakeroot git perl zip unzip curl libplist-utils
bash -c "$(curl -fsSL https://raw.githubusercontent.com/theos/theos/master/bin/install-theos)"
# Ao final, garanta no ~/.bashrc ou ~/.profile:
#   export THEOS=~/theos
```

O instalador oficial baixa o toolchain (clang + ldid) para Linux. Detalhes:
<https://theos.dev/docs/installation-linux>

### SDK iOS

O Theos precisa de um SDK iPhoneOS em `$THEOS/sdks`. Os SDKs patcheados para
uso com Theos ficam em <https://github.com/theos/sdks>:

```bash
cd $THEOS/sdks
curl -LO https://github.com/theos/sdks/archive/master.zip
unzip -q master.zip 'sdks-master/iPhoneOS*.sdk/*'
mv sdks-master/*.sdk . && rm -rf sdks-master master.zip
ls $THEOS/sdks   # deve listar iPhoneOSxx.x.sdk
```

O `Makefile` usa `TARGET := iphone:clang:latest:26.0` — "latest" escolhe
automaticamente o SDK mais novo instalado.

> Downloads acima são **build-time, na sua máquina**. O app final continua
> offline absoluto: zero rede em runtime.

### Toolchain Swift (opcional — decide D7)

O gate testa se existe `swiftc` capaz de gerar arm64-apple-ios
(`scripts/probe-swift.sh`). Se não existir ou falhar, o projeto segue em
ObjC/UIKit + C++ conforme D7 — não insistir mais de uma sessão nisso.

## Rodando o gate da Fase 0

```bash
./scripts/fase0-gate.sh
```

O script verifica `$THEOS` e SDKs, roda o probe Swift, compila o app "Olá"
com a dylib dummy embarcada, gera `build/RiverRaid.ipa` e grava
`docs/fase0-resultados.md`.

Build manual, sem o gate:

```bash
make package FINALPACKAGE=1   # compila e faz staging em .theos/_/
./scripts/package-ipa.sh      # monta build/RiverRaid.ipa
```

## Instalação via AltStore

1. Transfira `build/RiverRaid.ipa` para onde o AltStore o alcance (iCloud
   Drive não — use cabo/AltServer ou o compartilhamento local do AltStore).
2. No iPhone: AltStore → My Apps → **+** → selecionar `RiverRaid.ipa`.
3. O AltStore reassina o app **e os frameworks embarcados** com o certificado
   do Apple ID. Limites da conta gratuita: **3 apps ativos**, revalidação a
   cada **7 dias**.
4. Abra o app. A tela da Fase 0 mostra o resultado do teste de dlopen:
   - **Verde** — dylib carregada ⇒ D3-A confirmado (fechar ADR 0001).
   - **Âmbar** — falhou ⇒ ver troubleshooting abaixo; se persistir, D3-B.

## Troubleshooting (§11.5 do prompt mestre)

| Sintoma | Causa provável | Ação |
|---|---|---|
| `make` reclama de SDK/`TARGET` | Nenhum SDK em `$THEOS/sdks` ou versão incompatível | Instalar SDK (seção acima); conferir `ls $THEOS/sdks` |
| App instala mas dlopen retorna NULL | Reassinatura do framework falhou, ou path errado | Conferir mensagem de `dlerror` na tela; se for assinatura (`code signature invalid`), testar AltStore atualizado; persistindo, adotar **D3-B** (link estático + renomeação de símbolos por core) via novo ADR |
| AltStore recusa instalar | Limite de 3 apps ativos do Apple ID gratuito | Remover um app sideloaded e repetir |
| App expira/para de abrir após dias | Revalidação de 7 dias do certificado | Abrir o AltStore e atualizar (refresh) o app |
| Áudio estalando (fases futuras) | Buffer alvo baixo / DRC mal calibrado | Aumentar alvo (~64 ms) e revisar o controle dinâmico de taxa (D5) |
| `ldid` não encontrado no build | Toolchain do Theos incompleto | Reinstalar Theos; a assinatura local é opcional (o AltStore reassina), o Makefile tolera a falha |

## Nota sobre a estrutura

O prompt mestre (§10) lista `RiverRaid.plist` na raiz; no template
**application** do Theos o Info.plist vive em `Resources/Info.plist` e é
copiado para dentro do `.app` no staging — é o que este repositório usa.
