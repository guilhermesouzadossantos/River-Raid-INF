#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define RIVER_RAID_BLUE Color{39, 43, 174, 255}
#define LARGURA 960
#define ALTURA 800
#define TILE_SIZE 40
#define LARGURA_MAPA 24
#define ALTURA_MAPA 20

// Estrutura para representar o mapa do jogo
typedef struct {
    char quadradinhos[ALTURA_MAPA][LARGURA_MAPA]; // Desenha o mapa com base nas entradas 'T', 'N', 'X' etc.
    int num_navios;
    int num_helicopteros;
    int num_postos_gasolina;
    int tem_ponte;
} Mapa;

// Variáveis globais
Mapa mapa_atual;
int fase_atual = 1;
int total_fases = 3;
int jogo_completo = 0;
int velocidade_nave = 3; // Velocidade de movimento automático da nave (talvez aumentar a cada fase?)

// Função para carregar mapa do arquivo (errno_t para não dar erro ...)
int carregar_mapa(const char* nome_arquivo, Mapa* mapa) {
    FILE* arquivo = NULL;
    errno_t err = fopen_s(&arquivo, nome_arquivo, "r");
    if (err != 0 || !arquivo) {
        printf("ERRO: Nao foi possivel abrir o arquivo: %s\n", nome_arquivo);
        return 0;
    }

    // Inicializar contadores (começa em zero para nova contagem)
    mapa->num_navios = 0; // Usar "->" porque Mapa é um ponteiro, não a struct em si
    mapa->num_helicopteros = 0;
    mapa->num_postos_gasolina = 0;
    mapa->tem_ponte = 0;

    char linha[LARGURA_MAPA];
    
    for (int y = 0; y < ALTURA_MAPA; y++) {
        if (fgets(linha, sizeof(linha), arquivo) == NULL) {
            for (int x = 0; x < LARGURA_MAPA; x++) {
                mapa->quadradinhos[y][x] = 'T';
            }
            continue;
        }

        for (int x = 0; x < LARGURA_MAPA; x++) {
            if (x < strlen(linha) && linha[x] != '\n' && linha[x] != '\r') {
                mapa->quadradinhos[y][x] = linha[x];

                switch (linha[x]) {
                case 'N': mapa->num_navios++; break;
                case 'X': mapa->num_helicopteros++; break;
                case 'G': mapa->num_postos_gasolina++; break;
                case 'P': mapa->tem_ponte = 1; break;
                }
            }
            else {
                mapa->quadradinhos[y][x] = ' ';
            }
        }
    }

    fclose(arquivo);
    printf("Mapa %s carregado: Navios=%d, Helicopteros=%d, Postos=%d, Ponte=%d\n",
        nome_arquivo, mapa->num_navios, mapa->num_helicopteros,
        mapa->num_postos_gasolina, mapa->tem_ponte);
    return 1;
}

// Função para carregar a próxima fase
int carregar_proxima_fase() {
    fase_atual++;

    if (fase_atual > total_fases) {
        printf("Todas as fases completadas!\n");
        jogo_completo = 1;
        return 0;
    }

    char nome_arquivo[50];
    sprintf_s(nome_arquivo, sizeof(nome_arquivo), "assets/fase%d.txt", fase_atual);

    printf("Carregando proxima fase: %s\n", nome_arquivo);
    return carregar_mapa(nome_arquivo, &mapa_atual);
}

// Função para desenhar o mapa na tela com base no arquivo .txt da fase
void desenhar_mapa(Mapa* mapa) {
    for (int y = 0; y < ALTURA_MAPA; y++) {
        for (int x = 0; x < LARGURA_MAPA; x++) {
            char tile = mapa->quadradinhos[y][x];
            int screen_x = x * TILE_SIZE;
            int screen_y = y * TILE_SIZE;

            switch (tile) {
            case 'T': // Terra
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, GREEN);
                DrawRectangleLines(screen_x, screen_y, TILE_SIZE, TILE_SIZE, DARKGREEN);
                break;
            case 'N': // Navio
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, DARKGRAY);
                DrawRectangleLines(screen_x, screen_y, TILE_SIZE, TILE_SIZE, BLACK);
                break;
            case 'X': // Helicóptero
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, RED);
                DrawRectangleLines(screen_x, screen_y, TILE_SIZE, TILE_SIZE, BLACK);
                break;
            case 'G': // Posto de gasolina
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, YELLOW);
                DrawRectangleLines(screen_x, screen_y, TILE_SIZE, TILE_SIZE, BLACK);
                break;
            case 'P': // Ponte
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, BROWN);
                DrawRectangleLines(screen_x, screen_y, TILE_SIZE, TILE_SIZE, BLACK);
                break;
            }
        }
    }
}

// Função para verificar se uma posição é válida para a nave (não é terra)
int posicao_valida_nave(int pos_x, int pos_y, Mapa* mapa) {
    // Converter coordenadas de pixel para coordenadas de tile
    int tile_x = pos_x / TILE_SIZE;
    int tile_y = pos_y / TILE_SIZE;

    // Verificar se está dentro dos limites do mapa
    if (tile_x < 0 || tile_x >= LARGURA_MAPA || tile_y < 0 || tile_y >= ALTURA_MAPA) {
        return 0;
    }

    // A posição é válida se não for terra ('T')
    return mapa->quadradinhos[tile_y][tile_x] != 'T';
}

int main() {
    char texto[50] = "River Raid INF - Fase 1";
    int POSICAOX_NAVE = LARGURA / 2;
    int POSICAOY_NAVE = ALTURA - 100; // Começa na parte inferior
    int POSICAOX_MISSIL = -100;
    int POSICAOY_MISSIL = -100;

    InitWindow(LARGURA, ALTURA, "River Raid INF");
    SetTargetFPS(60);

    // Tentar carregar primeira fase
    if (!carregar_mapa("assets/fase1.txt", &mapa_atual)) {
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(RED);
            DrawText("ERRO: Nao foi possivel carregar fase1.txt", 50, ALTURA / 2 - 20, 30, WHITE);
            DrawText("Certifique-se de que o arquivo existe no diretorio correto", 50, ALTURA / 2 + 20, 20, WHITE);
            DrawText("Pressione ESC para sair", 50, ALTURA / 2 + 50, 20, WHITE);
            EndDrawing();
        }
        CloseWindow();
        return 1;
    }

    // SPRITES - Agora com tamanho 40x40
    Texture2D SPRITES = LoadTexture("assets/sprites.png");
    if (SPRITES.id == 0) {
        printf("AVISO: Nao foi possivel carregar sprites.png. Usando graficos basicos.\n");
    }

    // Ajustando os sprites para 40x40 pixels
    Rectangle NAVE = { 103, 70,  56, 52 }; // Recorte original da nave
    Rectangle MISSIL = { 0, 70, 40, 50 };  // Recorte original do míssil

    while (!WindowShouldClose()) {
        // MOVIMENTO AUTOMÁTICO DA NAVE (PARA CIMA)
        if (!jogo_completo) {
            POSICAOY_NAVE -= velocidade_nave;

            // Verificar se a nave chegou ao topo (final da fase)
            if (POSICAOY_NAVE <= -TILE_SIZE) {
                if (carregar_proxima_fase()) {
                    // Reposicionar nave na base para a nova fase
                    POSICAOY_NAVE = ALTURA;
                    sprintf_s(texto, sizeof(texto), "River Raid INF - Fase %d", fase_atual);
                }
            }
        }

        // Controles da nave 
        int nova_pos_x = POSICAOX_NAVE;

        if (IsKeyDown(KEY_RIGHT)) {
            nova_pos_x += 5;
        }
        if (IsKeyDown(KEY_LEFT)) {
            nova_pos_x -= 5;
        }
        if (IsKeyDown(KEY_DOWN)) {
            POSICAOY_NAVE += 2;
        }
        if (IsKeyDown(KEY_UP)) {
            POSICAOY_NAVE -= 5;
        }

        // Verificar se a nova posição horizontal é válida (não é terra)
        if (posicao_valida_nave(nova_pos_x, POSICAOY_NAVE, &mapa_atual) &&
            posicao_valida_nave(nova_pos_x + TILE_SIZE - 1, POSICAOY_NAVE, &mapa_atual) &&
            posicao_valida_nave(nova_pos_x, POSICAOY_NAVE + TILE_SIZE - 1, &mapa_atual) &&
            posicao_valida_nave(nova_pos_x + TILE_SIZE - 1, POSICAOY_NAVE + TILE_SIZE - 1, &mapa_atual)) {

            POSICAOX_NAVE = nova_pos_x;
        }

        // Limites da tela - apenas horizontal
        if (POSICAOX_NAVE < 0) POSICAOX_NAVE = 0;
        if (POSICAOX_NAVE > LARGURA - TILE_SIZE) POSICAOX_NAVE = LARGURA - TILE_SIZE;

        // Disparar míssil
        if (IsKeyPressed(KEY_SPACE) && POSICAOY_MISSIL < -50) {
            POSICAOX_MISSIL = POSICAOX_NAVE + (TILE_SIZE / 4);
            POSICAOY_MISSIL = POSICAOY_NAVE - 40;
        }

        // Atualizar míssil
        if (POSICAOY_MISSIL > -50) {
            POSICAOY_MISSIL -= 10;

            // Resetar míssil se sair da tela
            if (POSICAOY_MISSIL < -TILE_SIZE) {
                POSICAOY_MISSIL = -100;
                POSICAOX_MISSIL = -100;
            }
        }

        BeginDrawing();
        ClearBackground(RIVER_RAID_BLUE);

        // Desenhar mapa estático
        desenhar_mapa(&mapa_atual);

        // Desenhar elementos do jogo com tamanho 40x40
        if (SPRITES.id != 0) {
            // Desenhar nave redimensionada para 40x40
            Rectangle destNave = { POSICAOX_NAVE, POSICAOY_NAVE, TILE_SIZE, TILE_SIZE };
            DrawTexturePro(SPRITES, NAVE, destNave, Vector2 { 0, 0 }, 0, WHITE);

            // Desenhar míssil redimensionado para 20x40
            if (POSICAOY_MISSIL > -50) {
                Rectangle destMissil = { POSICAOX_MISSIL, POSICAOY_MISSIL, TILE_SIZE / 2, TILE_SIZE };
                DrawTexturePro(SPRITES, MISSIL, destMissil, Vector2 { 0, 0 }, 0, WHITE);
            }
        }
        else {
            // Fallback: usar retângulos coloridos com tamanho 40x40
            DrawRectangle(POSICAOX_NAVE, POSICAOY_NAVE, TILE_SIZE, TILE_SIZE, WHITE);
            if (POSICAOY_MISSIL > -50) {
                DrawRectangle(POSICAOX_MISSIL, POSICAOY_MISSIL, TILE_SIZE / 2, TILE_SIZE, ORANGE);
            }
        }

        // Desenhar informações da fase e HUD
        DrawRectangle(0, 0, LARGURA, 30, Fade(BLACK, 0.7f));
        DrawText(texto, 10, 5, 20, WHITE);
        DrawText(TextFormat("Fase: %d/%d", fase_atual, total_fases), LARGURA - 150, 5, 20, WHITE);

        // Mostrar mensagem de jogo completo
        if (jogo_completo) {
            DrawRectangle(LARGURA / 2 - 150, ALTURA / 2 - 50, 300, 100, Fade(BLACK, 0.8f));
            DrawText("JOGO COMPLETO!", LARGURA / 2 - 120, ALTURA / 2 - 30, 30, GREEN);
            DrawText("Pressione ESC para sair", LARGURA / 2 - 120, ALTURA / 2 + 10, 20, WHITE);
        }

        EndDrawing();
    }

    if (SPRITES.id != 0) {
        UnloadTexture(SPRITES);
    }
    CloseWindow();
    return 0;
}