#ifndef MENU_H
#define MENU_H

typedef enum GameState {
    MENU,
    NOVO_JOGO,
    CARREGAR_JOGO,
    OPCOES,
    SAIR
} GameState;

GameState RunMenu();

#endif