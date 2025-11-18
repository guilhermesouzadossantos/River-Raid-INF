#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include "menu.h"

#define RIVER_RAID_BLUE Color{39, 43, 174, 255}
#define LARGURA 960
#define ALTURA 800
#define TILE_SIZE 40
#define LARGURA_MAPA 48
#define ALTURA_MAPA 40

// Menu 
int opcao_menu = 0;
int op_saida_menu = 0;

// Estrutura para representar o mapa do jogo
typedef struct {
    char quadradinhos[ALTURA_MAPA][LARGURA_MAPA]; // Desenha o mapa com base nas entradas 'T', 'N', 'X' etc.
    int num_navios;
    int num_helicopteros;
    int num_postos_gasolina;
    int tem_ponte;
} Mapa;

//Define um novo tipo de vari?vel e o que ela pode representar para usarmos como seletora
typedef enum TelaJogo {
    TELA_INICIAL = 0,
    MENU,
    NOVO_JOGO,
    CARREGAR_JOGO,
    RANKING,
    PAUSE,
    SAIR,
    SALVARESAIR,
    SALVAR,
    SAIRDEFINITIVO,
    GAME_OVER
} TelaJogo;

//struct pra iniciar o jogo e pra salvar e carregar o jogo
typedef struct {
    char nome[50];
    int vidas;
    int combustivel;
    int nivel;
    int score;
} Jogador;

//struct pra salvar no final do jogo e usar o ranking
typedef struct {
    char nome[50];
    int score;
} JogadorFinal;

// Variaveis globais do jogo
Mapa mapa_atual;
int fase_atual = 1;
int total_fases = 10;
int jogo_completo = 0;
int velocidade_nave = 3; // Velocidade de movimento autom?tico da nave (talvez aumentar a cada fase?)
int score_atual = 0;

// Estado (agora global para permitir reset)
int POSICAOX_NAVE;
int POSICAOY_NAVE;
int POSICAOX_MISSIL;
int POSICAOY_MISSIL;
char texto[128]; // maior para segurança

// Declarações das funções auxiliares
void resetar_jogo(void);



int colisao_nave(Mapa* mapa, int nave_x, int nave_y, int tamanho) {

    // Lista de tiles que contam como colisão
    char objetos_colisao[] = { 'T', 'N', 'X', 'P', 'G' };
    int total_objetos = sizeof(objetos_colisao) / sizeof(objetos_colisao[0]);

    // Verifica os 4 cantos da nave
    int pontos_x[4] = { nave_x, nave_x + tamanho - 1, nave_x, nave_x + tamanho - 1 };
    int pontos_y[4] = { nave_y, nave_y, nave_y + tamanho - 1, nave_y + tamanho - 1 };

    for (int i = 0; i < 4; i++) {
        int tile_x = pontos_x[i] / TILE_SIZE;
        int tile_y = pontos_y[i] / TILE_SIZE;

        // Verifica se está dentro do mapa
        if (tile_x < 0 || tile_x >= LARGURA_MAPA || tile_y < 0 || tile_y >= ALTURA_MAPA)
            return 1; // bateu fora do mapa ? game over

        char tile = mapa->quadradinhos[tile_y][tile_x];

        // Compara com todos os objetos sólidos
        for (int j = 0; j < total_objetos; j++) {
            if (tile == objetos_colisao[j]) {
                return 1; // Colisão detectada
            }
        }
    }

    return 0;
}

// Colisão do míssil com o mapa; retorna 1 se acertou e remove o tile correspondente e adiciona pontos via ponteiro
int colisao_missil(Mapa* mapa, int mx, int my, int* pontos) {
    // Ponto de checagem no centro aproximado do míssil
    int check_x = mx + (TILE_SIZE / 4);
    int check_y = my + (TILE_SIZE / 2);

    int tx = check_x / TILE_SIZE;
    int ty = check_y / TILE_SIZE;

    if (tx < 0 || tx >= LARGURA_MAPA || ty < 0 || ty >= ALTURA_MAPA) return 0;

    char tile = mapa->quadradinhos[ty][tx];

    switch (tile) {
    case 'N':
        *pontos += 50;
        mapa->quadradinhos[ty][tx] = ' ';
        return 1;
    case 'X':
        *pontos += 100;
        mapa->quadradinhos[ty][tx] = ' ';
        return 1;
    case 'G':
        *pontos += 25;
        mapa->quadradinhos[ty][tx] = ' ';
        return 1;
    case 'P':
        *pontos += 200;
        mapa->quadradinhos[ty][tx] = ' ';
        return 1;
    default:
        return 0;
    }
}


// Tela de Game Over — exibe score atual e instrução para ver o ranking
TelaJogo TelaGameOver(void) {
    BeginDrawing();
    ClearBackground(BLACK);

    DrawText("GAME OVER", 350, 300, 60, RED);
    DrawText(TextFormat("SCORE: %d", score_atual), 380, 370, 40, WHITE);
    DrawText("Pressione ENTER para ver o RANKING", 240, 440, 25, WHITE);

    EndDrawing();

    if (IsKeyPressed(KEY_ENTER)) {
        return RANKING;
    }
    return GAME_OVER;
}


//Função para carregar a Tela inicial antes do menu
TelaJogo TelaIni(void) {
    Texture2D SPRITES = LoadTexture("assets/sprites.png");
    Rectangle NAVE = { 103, 70,  56, 52 };
    TelaJogo Tela = TELA_INICIAL;
    int posy = 275;
    //desenha a tela
    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("RiverINF", 500, 375, 90, YELLOW);
    DrawText("Pressione ENTER para iniciar", 500, 465, 15, WHITE);

    Rectangle destNave = { 90, 275, 300, 300 };

    DrawTexturePro(SPRITES, NAVE, destNave, Vector2{ 0, 0 }, 0, WHITE);
    //atualiza a mesma
    if (IsKeyPressed(KEY_ENTER)) {
        Tela = MENU;
    }

    EndDrawing();

    return Tela;
}

//Funcao para fazer o menu
TelaJogo TelaMenu(void) {
    TelaJogo Tela;
    Tela = MENU;
    const char* opcoes[4] = { "Novo Jogo", "Carregar Jogo", "Ranking", "Sair" };
    int total_opcoes = 4;

    if (IsKeyPressed(KEY_DOWN)) {
        opcao_menu++;
        if (opcao_menu >= total_opcoes) opcao_menu = 0;
    }
    if (IsKeyPressed(KEY_UP)) {
        opcao_menu--;
        if (opcao_menu < 0) opcao_menu = total_opcoes - 1;
    }
    //Nesse caso como a funcao ? tipada fiz um switch case pra retornar a nova tela que o jogo vai entrar
    if (IsKeyPressed(KEY_ENTER)) {
        switch (opcao_menu) {
        case 0:
            // chama resetar_jogo() para garantir estado limpo sempre que escolher Novo Jogo
            resetar_jogo();
            Tela = NOVO_JOGO;
            break;
        case 1:
            Tela = CARREGAR_JOGO;
            break;
        case 2:
            Tela = RANKING;
            break;
        case 3:
            Tela = SAIR;
            break;
        }
    }

    //Desenho
    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("RiverINF", 500, 375, 90, YELLOW);
    DrawText("Pressione ENTER para selecionar", 500, 465, 15, WHITE);

    DrawText(opcoes[0], 90, 325, 40, YELLOW);
    DrawText(opcoes[1], 90, 325 + 60, 40, YELLOW);
    DrawText(opcoes[2], 90, 325 + 120, 40, YELLOW);
    DrawText(opcoes[3], 90, 325 + 180, 40, YELLOW);

    for (int i = 0; i < total_opcoes; i++) {
        if (i == opcao_menu) {
            DrawRectangle(60, (320 + i * 60) + 20, 10, 10, YELLOW);
        }
    }
    EndDrawing();

    return Tela;
}

//Funcao para tela de saida no menu
TelaJogo TelaSaida(void) {
    TelaJogo Tela;
    Tela = SAIR;
    int ops_saida = 2;
    if (IsKeyPressed(KEY_UP)) {
        op_saida_menu--;
        if (op_saida_menu < 0) op_saida_menu = ops_saida - 1;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        op_saida_menu++;
        if (op_saida_menu >= ops_saida) op_saida_menu = 0;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        switch (op_saida_menu) {
        case 0:
            Tela = SAIRDEFINITIVO;
            break;
        case 1:
            Tela = MENU;
            break;
        }
    }

    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("Tem certeza?", 265, 275, 60, YELLOW);
    DrawText("Sim, sair", 380, 400, 30, YELLOW);
    DrawText("Voltar ao menu", 380, 400 + 60, 30, YELLOW);

    for (int i = 0; i < ops_saida; i++) {
        if (i == op_saida_menu) {
            DrawRectangle(360, (400 + i * 60) + 10, 8, 8, YELLOW);
        }
    }
    EndDrawing();

    return Tela;
}

//Função para quando pausa no meio do jogo
TelaJogo TelaSalvareSair(void) {
    TelaJogo Tela;
    Tela = SALVARESAIR;
    int ops_saida = 2;
    if (IsKeyPressed(KEY_UP)) {
        op_saida_menu--;
        if (op_saida_menu < 0) op_saida_menu = ops_saida - 1;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        op_saida_menu++;
        if (op_saida_menu >= ops_saida) op_saida_menu = 0;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        switch (op_saida_menu) {
        case 0:
            Tela = NOVO_JOGO;
            break;
        case 1:
            Tela = SALVAR;
            break;
        }
    }

    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("O que deseja?", 265, 275, 60, YELLOW);
    DrawText("Voltar ao jogo", 380, 400, 30, YELLOW);
    DrawText("Salvar e sair", 380, 400 + 60, 30, YELLOW);

    for (int i = 0; i < ops_saida; i++) {
        if (i == op_saida_menu) {
            DrawRectangle(360, (400 + i * 60) + 10, 8, 8, YELLOW);
        }
    }
    EndDrawing();

    return Tela;
}

// Funcao para carregar mapa do arquivo (errno_t para n?o dar erro ...)
int carregar_mapa(const char* nome_arquivo, Mapa* mapa) {
    FILE* arquivo = NULL;
    errno_t err = fopen_s(&arquivo, nome_arquivo, "r");
    if (err != 0 || !arquivo) {
        printf("ERRO: Nao foi possivel abrir o arquivo: %s\n", nome_arquivo);
        return 0;
    }

    // Inicializar contadores (comeca em zero para nova contagem)
    mapa->num_navios = 0; // Usar "->" porque Mapa ? um ponteiro, nao a struct em si
    mapa->num_helicopteros = 0;
    mapa->num_postos_gasolina = 0;
    mapa->tem_ponte = 0;

    char linha[LARGURA_MAPA + 8];

    for (int y = 0; y < ALTURA_MAPA; y++) {
        if (fgets(linha, sizeof(linha), arquivo) == NULL) {
            for (int x = 0; x < LARGURA_MAPA; x++) {
                mapa->quadradinhos[y][x] = 'T';
            }
            continue;
        }

        for (int x = 0; x < LARGURA_MAPA; x++) {
            if (x < (int)strlen(linha) && linha[x] != '\n' && linha[x] != '\r') {
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

// Funcao para carregar a proxima fase
int carregar_proxima_fase() {
    // tenta carregar a proxima sem alterar fase_atual ate ter sucesso
    if (fase_atual >= total_fases) {
        printf("Todas as fases completadas!\n");
        jogo_completo = 1;
        return 0;
    }

    int proxima = fase_atual + 1;
    char nome_arquivo[50];
    sprintf_s(nome_arquivo, sizeof(nome_arquivo), "assets/fase%d.txt", proxima);

    printf("Carregando proxima fase: %s\n", nome_arquivo);
    if (carregar_mapa(nome_arquivo, &mapa_atual)) {
        fase_atual = proxima;
        return 1;
    }
    else {
        printf("Falha ao carregar a fase %d\n", proxima);
        return 0;
    }
}

// Funcao para desenhar o mapa na tela com base no arquivo .txt da fase
void desenhar_mapa(Mapa* mapa, Texture2D sprites) {

    Rectangle NAVE = { 103, 70,  56, 52 };

    Texture2D SPRITES = LoadTexture("assets/sprites.png");
    Rectangle SPR_HELI = { 20, 190, 50, 40 };
    Rectangle SPR_NAVIO_INI = { 12, 32, 133, 60 }; 
    Rectangle SPR_GAS = { 200, 74, 32, 80 }; 
    Rectangle SPR_PONTE = { 150, 10, 250, 100 };

    for (int y = 0; y < ALTURA_MAPA; y++) {
        for (int x = 0; x < LARGURA_MAPA; x++) {

            char tile = mapa->quadradinhos[y][x];
            int screen_x = x * TILE_SIZE;
            int screen_y = y * TILE_SIZE;

            Rectangle destino = { screen_x, screen_y, TILE_SIZE, TILE_SIZE };

            switch (tile) {

            case 'T':   // TERRA
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, DARKGREEN);
                break;

            case 'N':   // NAVIO INIMIGO
                DrawTexturePro(SPRITES, SPR_NAVIO_INI, destino, Vector2{ 0, 0 }, 0, WHITE);
                break;

            case 'X':   // HELICÓPTERO
                DrawTexturePro(SPRITES, SPR_HELI, destino, Vector2{ 0, 0 }, 0, WHITE);
                break;

            case 'G':   // GASOLINA
                DrawTexturePro(SPRITES, SPR_GAS, destino, Vector2{ 0, 0 }, 0, WHITE);
                break;

            case 'P':   // PONTE
                DrawTexturePro(SPRITES, SPR_PONTE, destino, Vector2{ 0, 0 }, 0, WHITE);
                break;

            case ' ':   // ÁGUA
            default:
                DrawRectangle(destino.x, destino.y, TILE_SIZE, TILE_SIZE, RIVER_RAID_BLUE);
                break;
            }
        }
    }

    UnloadTexture(SPRITES); 
}

// Funcao para verificar se uma posicao e valida para a nave (nao e terra)
int posicao_valida_nave(int pos_x, int pos_y, Mapa* mapa) {
    // Converter coordenadas de pixel para coordenadas de tile
    int tile_x = pos_x / TILE_SIZE;
    int tile_y = pos_y / TILE_SIZE;

    // Verificar se esta dentro dos limites do mapa
    if (tile_x < 0 || tile_x >= LARGURA_MAPA || tile_y < 0 || tile_y >= ALTURA_MAPA) {
        return 0;
    }

    return mapa->quadradinhos[tile_y][tile_x] != 'T';
}

// Retorna 1 se encontrou e escreve out_x/out_y em pixels; 0 caso contrário (usa fallback)
int encontrar_pos_spawn(Mapa* mapa, int* out_x, int* out_y) {
    // Procurar nas linhas horizontais correspondentes à visual inferior (próximo à tela)
    int try_y = ALTURA - TILE_SIZE - 1;
    if (try_y < 0) try_y = 0;

    for (int tx = 0; tx < LARGURA_MAPA; tx++) {
        int px = tx * TILE_SIZE;
        // verificar os 4 cantos para garantir espaço
        if (posicao_valida_nave(px, try_y, mapa) &&
            posicao_valida_nave(px + TILE_SIZE - 1, try_y, mapa) &&
            posicao_valida_nave(px, try_y + TILE_SIZE - 1, mapa) &&
            posicao_valida_nave(px + TILE_SIZE - 1, try_y + TILE_SIZE - 1, mapa)) {
            *out_x = px;
            *out_y = try_y;
            return 1;
        }
    }

    // não achou: tenta varrer um pouco acima (duas linhas)
    for (int dy = 1; dy <= 3; dy++) {
        int ty = try_y - dy * TILE_SIZE;
        if (ty < 0) break;
        for (int tx = 0; tx < LARGURA_MAPA; tx++) {
            int px = tx * TILE_SIZE;
            if (posicao_valida_nave(px, ty, mapa) &&
                posicao_valida_nave(px + TILE_SIZE - 1, ty, mapa) &&
                posicao_valida_nave(px, ty + TILE_SIZE - 1, mapa) &&
                posicao_valida_nave(px + TILE_SIZE - 1, ty + TILE_SIZE - 1, mapa)) {
                *out_x = px;
                *out_y = ty;
                return 1;
            }
        }
    }

    return 0;
}

// Função que reinicia o estado do jogo para iniciar um Novo Jogo
void resetar_jogo(void) {
    fase_atual = 1;
    score_atual = 0;
    jogo_completo = 0;
    velocidade_nave = 3;

    // Reposiciona a nave e o míssil
    POSICAOX_NAVE = LARGURA / 2;
    POSICAOY_NAVE = ALTURA - 100;
    POSICAOX_MISSIL = -100;
    POSICAOY_MISSIL = -100;

    // Atualiza o texto da fase
    sprintf_s(texto, sizeof(texto), "River Raid INF - Fase %d", fase_atual);

    // Recarrega o mapa da fase 1 (se falhar, a função carregar_mapa já imprime erro)
    carregar_mapa("assets/fase1.txt", &mapa_atual);

    // Tentar achar um spawn seguro e ajustar se encontrado
    int sx, sy;
    if (encontrar_pos_spawn(&mapa_atual, &sx, &sy)) {
        POSICAOX_NAVE = sx;
        POSICAOY_NAVE = sy;
    }
}

// main
int main() {
    // inicializar estado (em caso de entrar direto em NOVO_JOGO)
    POSICAOX_NAVE = LARGURA / 2;
    POSICAOY_NAVE = ALTURA - 100;
    POSICAOX_MISSIL = -100;
    POSICAOY_MISSIL = -100;
    sprintf_s(texto, sizeof(texto), "River Raid INF - Fase %d", fase_atual);

    TelaJogo TelaAgora = TELA_INICIAL;

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
            if (IsKeyPressed(KEY_ESCAPE)) break;
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
    Rectangle NAVE = { 103, 70,  56, 52 }; 
    Rectangle MISSIL = { 0, 70, 40, 50 }; 

    // Variável para detectar transições de tela (para inicializar apenas quando entrar em NOVO_JOGO)
    TelaJogo TelaAnterior = (TelaJogo)-1;

    while (!WindowShouldClose()) {

        // Se houve mudança de tela, realizar inicializações específicas
        if (TelaAgora != TelaAnterior) {
            // Entrando em NOVO_JOGO ja tratamos reset ao escolher "Novo Jogo" no menu,
            // mas se houver outro caminho que setar NOVO_JOGO, garantir estado limpo:
            if (TelaAgora == NOVO_JOGO) {
                // garantia adicional: se o mapa não for da fase atual, recarregar
                // (mantém comportamento seguro)
                // atualizar texto
                sprintf_s(texto, sizeof(texto), "River Raid INF - Fase %d", fase_atual);

                // garantir spawn em posição válida
                int sx, sy;
                if (encontrar_pos_spawn(&mapa_atual, &sx, &sy)) {
                    POSICAOX_NAVE = sx;
                    POSICAOY_NAVE = sy;
                }
                else {
                    POSICAOX_NAVE = LARGURA / 2;
                    POSICAOY_NAVE = ALTURA - 100;
                }

                POSICAOX_MISSIL = -100;
                POSICAOY_MISSIL = -100;
            }
        }

        switch (TelaAgora) {
        case TELA_INICIAL:
            TelaAgora = TelaIni();
            break;
        case MENU:
            TelaAgora = TelaMenu();
            break;
        case CARREGAR_JOGO:
            // implementação futura
            TelaAgora = TelaIni();
            break;
        case RANKING:
            // implementação futura
            TelaAgora = TelaIni();
            break;
        case SAIR:
            TelaAgora = TelaSaida();
            break;
        case SAIRDEFINITIVO:
            CloseWindow();
            return 1;
        case SALVARESAIR:
            TelaAgora = TelaSalvareSair();
            break;
        case SALVAR:
            TelaAgora = TelaIni();
            break;
        case NOVO_JOGO:
            // MOVIMENTO AUTOMATICO DA NAVE (PARA CIMA)
            if (!jogo_completo) {
                POSICAOY_NAVE -= velocidade_nave;

                // Verificar se a nave chegou ao topo (final da fase)
                if (POSICAOY_NAVE <= -TILE_SIZE) {
                    // tentar carregar proxima fase; se sucesso, reposicionar nav e atualizar texto
                    if (carregar_proxima_fase()) {
                        // Reposicionar nave na base para a nova fase (em posição segura)
                        int sx, sy;
                        if (encontrar_pos_spawn(&mapa_atual, &sx, &sy)) {
                            POSICAOX_NAVE = sx;
                            POSICAOY_NAVE = sy;
                        }
                        else {
                            POSICAOX_NAVE = LARGURA / 2;
                            POSICAOY_NAVE = ALTURA - 100;
                        }
                        sprintf_s(texto, sizeof(texto), "River Raid INF - Fase %d", fase_atual);
                    }
                    else {
                        // Se não carregou próxima fase: marca jogo como completo
                        jogo_completo = 1;
                    }
                }
            }

            // Controles da nave 
            {
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
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    TelaAgora = SALVARESAIR;
                }

                // Verificar se a nova posicao horizontal ? valida (nao e terra)
                if (posicao_valida_nave(nova_pos_x, POSICAOY_NAVE, &mapa_atual) &&
                    posicao_valida_nave(nova_pos_x + TILE_SIZE - 1, POSICAOY_NAVE, &mapa_atual) &&
                    posicao_valida_nave(nova_pos_x, POSICAOY_NAVE + TILE_SIZE - 1, &mapa_atual) &&
                    posicao_valida_nave(nova_pos_x + TILE_SIZE - 1, POSICAOY_NAVE + TILE_SIZE - 1, &mapa_atual)) {

                    POSICAOX_NAVE = nova_pos_x;
                }

                // Limites da tela - apenas horizontal
                if (POSICAOX_NAVE < 0) POSICAOX_NAVE = 0;
                if (POSICAOX_NAVE > LARGURA - TILE_SIZE) POSICAOX_NAVE = LARGURA - TILE_SIZE;
            }

            // Disparar missil
            if (IsKeyPressed(KEY_SPACE) && POSICAOY_MISSIL < -50) {
                POSICAOX_MISSIL = POSICAOX_NAVE + (TILE_SIZE / 4);
                POSICAOY_MISSIL = POSICAOY_NAVE - 40;
            }

            // Atualizar missil
            if (POSICAOY_MISSIL > -50) {
                POSICAOY_MISSIL -= 10;

                // Resetar missil se sair da tela
                if (POSICAOY_MISSIL < -TILE_SIZE) {
                    POSICAOY_MISSIL = -100;
                    POSICAOX_MISSIL = -100;
                }
            }

            if (POSICAOY_MISSIL > -50) {
                if (colisao_missil(&mapa_atual, POSICAOX_MISSIL, POSICAOY_MISSIL, &score_atual)) {
                    // resetar missil ao acertar
                    POSICAOY_MISSIL = -100;
                    POSICAOX_MISSIL = -100;
                }
            }

            if (colisao_nave(&mapa_atual, POSICAOX_NAVE, POSICAOY_NAVE, TILE_SIZE)) {
                TelaAgora = GAME_OVER;
                break;
            }

            BeginDrawing();
            ClearBackground(RIVER_RAID_BLUE);

            // Desenhar mapa estatico
            desenhar_mapa(&mapa_atual, SPRITES);

            // Desenhar elementos do jogo com tamanho 40x40
            if (SPRITES.id != 0) {
                // Desenhar nave redimensionada para 40x40
                Rectangle destNave = { POSICAOX_NAVE, POSICAOY_NAVE, TILE_SIZE, TILE_SIZE };
                DrawTexturePro(SPRITES, NAVE, destNave, Vector2{ 0, 0 }, 0, WHITE);

                // Desenhar missil redimensionado para 20x40
                if (POSICAOY_MISSIL > -50) {
                    Rectangle destMissil = { POSICAOX_MISSIL, POSICAOY_MISSIL, TILE_SIZE / 2, TILE_SIZE };
                    DrawTexturePro(SPRITES, MISSIL, destMissil, Vector2{ 0, 0 }, 0, WHITE);
                }
            }
            else {
                // Fallback: usar retangulos coloridos com tamanho 40x40
                DrawRectangle(POSICAOX_NAVE, POSICAOY_NAVE, TILE_SIZE, TILE_SIZE, WHITE);
                if (POSICAOY_MISSIL > -50) {
                    DrawRectangle(POSICAOX_MISSIL, POSICAOY_MISSIL, TILE_SIZE / 2, TILE_SIZE, ORANGE);
                }
            }

            // Desenhar informacoes da fase e HUD
            DrawRectangle(0, 0, LARGURA, 30, Fade(BLACK, 0.7f));
            DrawText(texto, 10, 5, 20, WHITE);
            DrawText(TextFormat("Fase: %d/%d", fase_atual, total_fases), LARGURA - 150, 5, 20, WHITE);
            DrawText(TextFormat("Score: %d", score_atual), LARGURA / 2 - 40, 5, 20, WHITE);

            // Mostrar mensagem de jogo completo
            if (jogo_completo) {
                DrawRectangle(LARGURA / 2 - 150, ALTURA / 2 - 50, 300, 100, Fade(BLACK, 0.8f));
                DrawText("JOGO COMPLETO!", LARGURA / 2 - 120, ALTURA / 2 - 30, 30, GREEN);
                DrawText("Pressione ESC para sair", LARGURA / 2 - 120, ALTURA / 2 + 10, 20, WHITE);
            }

            EndDrawing();
            break;

        case GAME_OVER:
            TelaAgora = TelaGameOver();
            break;

        default:
            break;
        }

        // atualizar TelaAnterior para detectar próxima transição
        TelaAnterior = TelaAgora;
    }

    if (SPRITES.id != 0) {
        UnloadTexture(SPRITES);
    }
    CloseWindow();
    return 0;
}
