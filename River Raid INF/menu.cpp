#include "raylib.h"
#include "menu.h"

GameState RunMenu() {
    const char* opcoes[] = {
        "Novo Jogo",
        "Carregar Jogo",
        "Opções",
        "Sair"
    };

    Rectangle botoes[4];
    int selecionado = -1;

    // Define posição e tamanho dos botões
    for (int i = 0; i < 4; i++) {
        botoes[i] = (Rectangle){ 320, 150 + 70 * i, 300, 60 };
    }

    while (!WindowShouldClose()) {
        selecionado = -1;

        // Verifica hover / clique
        for (int i = 0; i < 4; i++) {
            if (CheckCollisionPointRec(GetMousePosition(), botoes[i])) {
                selecionado = i;
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    return (GameState)(i + 1);
            }
        }

        BeginDrawing();
        ClearBackground(DARKBLUE);

        DrawText("MENU PRINCIPAL", 300, 60, 40, RAYWHITE);

        // Desenha botões
        for (int i = 0; i < 4; i++) {
            Color cor = (i == selecionado ? SKYBLUE : LIGHTGRAY);
            DrawRectangleRec(botoes[i], cor);
            DrawRectangleLinesEx(botoes[i], 3, WHITE);
            DrawText(opcoes[i], botoes[i].x + 40, botoes[i].y + 15, 30, BLACK);
        }

        EndDrawing();
    }

    return SAIR; // caso feche a janela
}