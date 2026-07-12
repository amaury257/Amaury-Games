// Dylib dummy da Fase 0: prova que frameworks embarcados em .app/Frameworks/
// sobrevivem à reassinatura do AltStore e que dlopen funciona no app
// sideloaded — o mesmo mecanismo que carregará os cores libretro (D3-A).
__attribute__((visibility("default")))
const char *fase0_mensagem(void) {
    return "dylib dummy carregada e executada com sucesso";
}
