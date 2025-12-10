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
#define MAX_EXPLOSOES 20 //  Máximo de explosões simultâneas
#define MAX_RANK 10

// Menu 
int opcao_menu = 0;
int op_saida_menu = 0;

// Estrutura para representar o mapa do jogo
typedef struct {
    char quadradinhos[ALTURA_MAPA][LARGURA_MAPA];
    int num_navios;
    int num_helicopteros;
    int num_postos_gasolina;
    int tem_ponte;
} Mapa;

// Enum de Telas
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
    GAME_OVER,
    NOVO_HIGH_SCORE
} TelaJogo;

// Structs jogador
typedef struct {
    char nome[50];
    int vidas;
    int combustivel;
    int nivel;
    int score;
} Jogador;

typedef struct {
    char nome[50];
    int score;
} JogadorFinal;

// Estrutura para Explosão
typedef struct {
    float x, y;
    int ativo;
    float tempo_animacao;
    int frame_atual;
} ObjExplosao;

// Variaveis globais do jogo
Mapa mapa_atual;
int fase_atual = 1;
int total_fases = 10;
int jogo_completo = 0;
int velocidade_nave = 3;
int score_atual = 0;
double ultimo_movimento_helicopteros = 0.0;
double intervalo_movimento_helicopteros = 0.8;
double ultimo_movimento_barcos = 0.0;
double intervalo_movimento_barcos = 0.8;
JogadorFinal ranking[MAX_RANK];
int novo_high_score = 0; // 1 se for um high score, 0 caso contrário
char nome_novo_score[50] = { 0 }; // Buffer para entrada de nome
int letra_atual_nome = 0; // Contador de caracteres do nome

// Variáveis Globais de Combustível e Explosão
float combustivel = 100.0f;
const float MAX_COMBUSTIVEL = 100.0f;
const float CONSUMO_GASOLINA = 3.0f; // Consumo por segundo considerando um gasto de pouco mais de 10 por mapa
ObjExplosao lista_explosoes[MAX_EXPLOSOES];
int player_explodindo = 0; // Estado para travar o jogo enquanto a nave explode
float timer_gameover = 0.0f;

// Estado
int POSICAOX_NAVE;
int POSICAOY_NAVE;
int POSICAOX_MISSIL;
int POSICAOY_MISSIL;
char texto[128];

// Declarações das funções auxiliares
void resetar_jogo(void);
void adicionar_explosao(float x, float y); 
void atualizar_explosoes(float dt);        

// Função para criar explosão 
void adicionar_explosao(float x, float y) {
    for (int i = 0; i < MAX_EXPLOSOES; i++) {
        if (!lista_explosoes[i].ativo) {
            lista_explosoes[i].x = x;
            lista_explosoes[i].y = y;
            lista_explosoes[i].ativo = 1;
            lista_explosoes[i].tempo_animacao = 0.0f;
            lista_explosoes[i].frame_atual = 0;
            break;
        }
    }
}

// Função para atualizar animação das explosões
void atualizar_explosoes(float dt) {
    for (int i = 0; i < MAX_EXPLOSOES; i++) {
        if (lista_explosoes[i].ativo) {
            lista_explosoes[i].tempo_animacao += dt;
            // Muda de frame a cada 0.1 segundos
            if (lista_explosoes[i].tempo_animacao > 0.1f) {
                lista_explosoes[i].frame_atual++;
                lista_explosoes[i].tempo_animacao = 0.0f;
                // Se passou do frame 4 (0 a 4), desativa
                if (lista_explosoes[i].frame_atual > 4) {
                    lista_explosoes[i].ativo = 0;
                }
            }
        }
    }
}

int colisao_nave(Mapa* mapa, int nave_x, int nave_y, int tamanho) {
    // Verifica se qualquer parte da nave está acima da tela
    if (nave_y + tamanho <= 0) {
        return 0; // Nave completamente acima da tela - transição de fase
    }

    int verificar_colisao = 1;
    int y_inicio = 0;

    if (nave_y < 0) {
        // Nave está parcialmente acima da tela - só verifica a parte visível
        y_inicio = -nave_y;
    }

    char objetos_colisao[] = { 'T', 'N', 'X' };
    int total_objetos = sizeof(objetos_colisao) / sizeof(objetos_colisao[0]);

    // Verifica os 4 cantos da nave
    int pontos_x[4] = { nave_x, nave_x + tamanho - 1, nave_x, nave_x + tamanho - 1 };
    int pontos_y[4] = { nave_y + y_inicio, nave_y + y_inicio, nave_y + tamanho - 1, nave_y + tamanho - 1 };

    for (int i = 0; i < 4; i++) {
        int tile_x = pontos_x[i] / TILE_SIZE;
        int tile_y = pontos_y[i] / TILE_SIZE;

        if (tile_x < 0 || tile_x >= LARGURA_MAPA || tile_y < 0 || tile_y >= ALTURA_MAPA) {
            if (tile_y >= ALTURA_MAPA || tile_x < 0 || tile_x >= LARGURA_MAPA) {
                return 1; // bateu fora do mapa
            }
            continue;
        }

        char tile = mapa->quadradinhos[tile_y][tile_x];
        for (int j = 0; j < total_objetos; j++) {
            if (tile == objetos_colisao[j]) {
                return 1; // Colisão detectada
            }
        }
    }
    return 0;
}

// Colisão do Míssil com lógica de Ponte e Explosões
int colisao_missil(Mapa* mapa, int mx, int my, int* pontos) {
    int check_x = mx + (TILE_SIZE / 4);
    int check_y = my + (TILE_SIZE / 2);
    int tx = check_x / TILE_SIZE;
    int ty = check_y / TILE_SIZE;

    if (tx < 0 || tx >= LARGURA_MAPA || ty < 0 || ty >= ALTURA_MAPA) return 0;

    char tile = mapa->quadradinhos[ty][tx];

    switch (tile) {
    case 'N':
        *pontos += 30;
        mapa->quadradinhos[ty][tx] = ' ';
        adicionar_explosao((float)(tx * TILE_SIZE), (float)(ty * TILE_SIZE)); 
        return 1;
    case 'X':
        *pontos += 60;
        mapa->quadradinhos[ty][tx] = ' ';
        adicionar_explosao((float)(tx * TILE_SIZE), (float)(ty * TILE_SIZE));
        return 1;
    case 'G': // Gasolina explode e dá pontos se atirar nela
        *pontos += 80;
        mapa->quadradinhos[ty][tx] = ' ';
        adicionar_explosao((float)(tx * TILE_SIZE), (float)(ty * TILE_SIZE));
        return 1;
    case 'P': // Destruição da Ponte
        *pontos += 200;
        // Limpa a linha inteira da ponte
        for (int x = 0; x < LARGURA_MAPA; x++) {
            if (mapa->quadradinhos[ty][x] == 'P') {
                mapa->quadradinhos[ty][x] = ' ';
                if (x % 2 == 0) adicionar_explosao((float)(x * TILE_SIZE), (float)(ty * TILE_SIZE));
            }
        }
        mapa->tem_ponte = 0;
        return 1;
    default:
        return 0;
    }
}

TelaJogo TelaGameOver(void) {

    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("GAME OVER", 275, 275, 60, RED);
    DrawText(TextFormat("SCORE: %d", score_atual), 380, 370, 40, WHITE);
    DrawText("Pressione ENTER para ver o RANKING", 240, 440, 25, WHITE);
    EndDrawing();

    if (IsKeyPressed(KEY_ENTER)) {
        return RANKING;
    } // Vai para o ranking
    return GAME_OVER;
}

TelaJogo TelaIni(void) {
    Texture2D SPRITES = LoadTexture("assets/sprites.png");
    Rectangle NAVE = { 103, 70,  56, 52 };
    TelaJogo Tela = TELA_INICIAL;

    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("RiverINF", 500, 375, 90, YELLOW);
    DrawText("Pressione ENTER para iniciar", 500, 465, 15, WHITE);
    Rectangle destNave = { 90, 275, 300, 300 };
    DrawTexturePro(SPRITES, NAVE, destNave, Vector2{ 0, 0 }, 0, WHITE);
    if (IsKeyPressed(KEY_ENTER)) Tela = MENU;
    EndDrawing();

    return Tela;
}


TelaJogo TelaMenu(void) {
    TelaJogo Tela = MENU;
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
    if (IsKeyPressed(KEY_ENTER)) {
        switch (opcao_menu) {
        case 0:
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
    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("RiverINF", 500, 375, 90, YELLOW);
    DrawText("Pressione ENTER para selecionar", 500, 465, 15, WHITE);
    DrawText(opcoes[0], 90, 325, 40, YELLOW);
    DrawText(opcoes[1], 90, 325 + 60, 40, YELLOW);
    DrawText(opcoes[2], 90, 325 + 120, 40, YELLOW);
    DrawText(opcoes[3], 90, 325 + 180, 40, YELLOW);
    for (int i = 0; i < total_opcoes; i++) {
        if (i == opcao_menu) DrawRectangle(60, (320 + i * 60) + 20, 10, 10, YELLOW);
    }
    EndDrawing();
    return Tela;
}

TelaJogo TelaSaida(void) {
    TelaJogo Tela = SAIR;
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
        case 0: Tela = SAIRDEFINITIVO; break;
        case 1: Tela = MENU; break;
        }
    }
    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("Tem certeza?", 265, 275, 60, YELLOW);
    DrawText("Sim, sair", 380, 400, 30, YELLOW);
    DrawText("Voltar ao menu", 380, 400 + 60, 30, YELLOW);
    for (int i = 0; i < ops_saida; i++) {
        if (i == op_saida_menu) DrawRectangle(360, (400 + i * 60) + 10, 8, 8, YELLOW);
    }
    EndDrawing();
    return Tela;
}

TelaJogo TelaSalvareSair(void) {
    TelaJogo Tela = SALVARESAIR;
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
        case 0: Tela = NOVO_JOGO; break;
        case 1: Tela = SALVAR; break;
        }
    }
    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("O que deseja?", 265, 275, 60, YELLOW);
    DrawText("Voltar ao jogo", 380, 400, 30, YELLOW);
    DrawText("Salvar e sair", 380, 400 + 60, 30, YELLOW);
    for (int i = 0; i < ops_saida; i++) {
        if (i == op_saida_menu) DrawRectangle(360, (400 + i * 60) + 10, 8, 8, YELLOW);
    }
    EndDrawing();
    return Tela;
}


// Função para salvar o ranking no arquivo binário
void salvar_ranking(JogadorFinal ranking[]) {
    FILE* arquivo = NULL;
    errno_t err = fopen_s(&arquivo, "assets/highscore.bin", "wb+");

    if (err != 0 || !arquivo) {
        printf("ERRO: Nao foi possivel salvar highscore.bin\n");
        return;
    }

    // Escreve o array de ranking no arquivo
    fwrite(ranking, sizeof(JogadorFinal), MAX_RANK, arquivo);
    fclose(arquivo);
}

// Função para inserir a nova pontuação no ranking (se for um high score)
void inserir_no_ranking(JogadorFinal ranking[], int score, const char* nome) {
    int i, j;
    // Verifica se a pontuação é maior que a menor pontuação do ranking
    if (score > ranking[MAX_RANK - 1].score) {
        // Encontra a posição correta para inserção
        for (i = 0; i < MAX_RANK; i++) {
            if (score > ranking[i].score) {
                // Desloca os elementos menores para baixo
                for (j = MAX_RANK - 1; j > i; j--) {
                    ranking[j] = ranking[j - 1];
                }
                // Insere a nova pontuação
                ranking[i].score = score;
                // Copia o nome (truncando((ensinamentos de arq0 valendo a pena)) se necessário)
                strncpy_s(ranking[i].nome, sizeof(ranking[i].nome), nome, _TRUNCATE);
                return;
            }
        }
    }
}


// Tela para o jogador digitar o nome ao conseguir um High Score 
TelaJogo TelaNovoHighScore(void) {
    int key = GetCharPressed();
    // Limite de 20 caracteres para o nome.
    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (letra_atual_nome < 20)) {
            nome_novo_score[letra_atual_nome] = (char)key;
            letra_atual_nome++;
        }
        key = GetCharPressed();
    }
    nome_novo_score[letra_atual_nome] = '\0';

    // Permite apagar com Backspace
    if (IsKeyPressed(KEY_BACKSPACE)) {
        letra_atual_nome--;
        if (letra_atual_nome < 0) letra_atual_nome = 0;
        nome_novo_score[letra_atual_nome] = '\0';
    }

    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("NOVO HIGH SCORE!", LARGURA / 2 - MeasureText("NOVO HIGH SCORE!", 50) / 2, 100, 50, GOLD);
    DrawText(TextFormat("SCORE: %d", score_atual), LARGURA / 2 - MeasureText(TextFormat("SCORE: %d", score_atual), 30) / 2, 180, 30, WHITE);

    DrawText("DIGITE SEU NOME (MAX 20):", LARGURA / 2 - MeasureText("DIGITE SEU NOME (MAX 20):", 30) / 2, 300, 30, YELLOW);

    // Desenha a caixa de texto
    DrawRectangle(LARGURA / 2 - 200, 350, 400, 50, WHITE);
    DrawText(nome_novo_score, LARGURA / 2 - 190, 360, 30, BLACK);

    // Desenha o cursor piscando
    if (((int)(GetTime() * 2)) % 2 == 0) DrawText("_", LARGURA / 2 - 190 + MeasureText(nome_novo_score, 30), 360, 30, BLACK);

    DrawText("Pressione ENTER para SALVAR", LARGURA / 2 - MeasureText("Pressione ENTER para SALVAR", 25) / 2, 450, 25, WHITE);
    EndDrawing();

    if (IsKeyPressed(KEY_ENTER) && letra_atual_nome > 0) {
        nome_novo_score[letra_atual_nome] = '\0';

        inserir_no_ranking(ranking, score_atual, nome_novo_score);
        salvar_ranking(ranking);

        // Reseta estados
        novo_high_score = 0;
        letra_atual_nome = 0;
        nome_novo_score[0] = '\0';

        return TELA_INICIAL; // Volta para o Menu Principal
    }

    return NOVO_HIGH_SCORE; // Permanece nesta tela
}

// Tela de Ranking
TelaJogo TelaRanking(void) {
    TelaJogo Tela = RANKING;
    BeginDrawing();
    ClearBackground(RIVER_RAID_BLUE);
    DrawText("TOP 10 PILOTOS", LARGURA / 2 - MeasureText("TOP 10 PILOTOS", 40) / 2, 50, 40, YELLOW);
    DrawText("Pressione ENTER para voltar", LARGURA / 2 - MeasureText("Pressione ENTER para voltar", 20) / 2, ALTURA - 30, 20, WHITE);

    for (int i = 0; i < MAX_RANK; i++) {
        Color cor = WHITE;
        if (i == 0) cor = GOLD;
        else if (i == 1) cor = GRAY;
        else if (i == 2) cor = DARKBROWN;

        // Formato: 01. NOME (SCORE)
        DrawText(TextFormat("%02d. %s", i + 1, ranking[i].nome), 100, 150 + i * 45, 30, cor);
        DrawText(TextFormat("%d", ranking[i].score), LARGURA - 200, 150 + i * 45, 30, cor);
    }

    EndDrawing();
    
	if (IsKeyPressed(KEY_ENTER)) {
		return TELA_INICIAL;
        }
   
    return Tela;
}



// Função para carregar o ranking a partir do arquivo binário
void carregar_ranking(JogadorFinal ranking[]) {
    FILE* arquivo = NULL;
    errno_t err = fopen_s(&arquivo, "assets/highscore.bin", "rb+");

    // Verifica se o arquivo foi aberto corretamente
    if (err != 0 || !arquivo) {
        printf("AVISO: Nao foi possivel carregar highscore.bin. Inicializando ranking padrao.\n");
        // Inicializa o ranking com valores padrão caso o arquivo não exista
        for (int i = 0; i < MAX_RANK; i++) {
            sprintf_s(ranking[i].nome, sizeof(ranking[i].nome), "---- %d", MAX_RANK - i);
            ranking[i].score = 000;
        }
    }
    else {
        // Lê os dados do arquivo para a estrutura de ranking
        size_t lidos = fread(ranking, sizeof(JogadorFinal), MAX_RANK, arquivo);
        if (lidos < MAX_RANK) {
            printf("AVISO: highscore.bin com dados incompletos. Inicializando o restante.\n");
            // Preenche os restantes com valores padrão se o arquivo for menor
            for (size_t i = lidos; i < MAX_RANK; i++) {
                sprintf_s(ranking[i].nome, sizeof(ranking[i].nome), "---- %d", MAX_RANK - i);
                ranking[i].score = 000;
            }
        }
        fclose(arquivo);
    }
}



int carregar_mapa(const char* nome_arquivo, Mapa* mapa) {
    FILE* arquivo = NULL;
    errno_t err = fopen_s(&arquivo, nome_arquivo, "r");
    if (err != 0 || !arquivo) {
        printf("ERRO: Nao foi possivel abrir o arquivo: %s\n", nome_arquivo);
        return 0;
    }
    mapa->num_navios = 0;
    mapa->num_helicopteros = 0;
    mapa->num_postos_gasolina = 0;
    mapa->tem_ponte = 0;
    char linha[LARGURA_MAPA + 8];
    for (int y = 0; y < ALTURA_MAPA; y++) {
        if (fgets(linha, sizeof(linha), arquivo) == NULL) {
            for (int x = 0; x < LARGURA_MAPA; x++) mapa->quadradinhos[y][x] = 'T';
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
    return 1;
}

int carregar_proxima_fase() {
    if (fase_atual >= total_fases) {
        jogo_completo = 1;
        return 0;
    }
    int proxima = fase_atual + 1;
    char nome_arquivo[50];
    sprintf_s(nome_arquivo, sizeof(nome_arquivo), "assets/fase%d.txt", proxima);
    if (carregar_mapa(nome_arquivo, &mapa_atual)) {
        fase_atual = proxima;
        return 1;
    }
    return 0;
}

void desenhar_mapa(Mapa* mapa, Texture2D sprites) {
    Rectangle NAVE = { 103, 70, 56, 52 };
    Rectangle MISSIL = { 0, 70, 40, 50 };
    Rectangle HELICOPTERO = { 8, 184, 65, 44 };
    Rectangle NAVIO = { 12, 232, 133, 60 };
    Rectangle GAS = { 609, 60, 60, 100 };
    Rectangle PONTE = { 685, 64, 255, 95 };

    for (int y = 0; y < ALTURA_MAPA; y++) {
        for (int x = 0; x < LARGURA_MAPA; x++) {
            char tile = mapa->quadradinhos[y][x];
            int screen_x = x * TILE_SIZE;
            int screen_y = y * TILE_SIZE;
            Rectangle destino = { (float)screen_x, (float)screen_y, TILE_SIZE, TILE_SIZE };
            Rectangle NAVIO_destino = { (float)screen_x, (float)screen_y, TILE_SIZE, TILE_SIZE };

            switch (tile) {
            case 'T': DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, DARKGREEN); break;
            case 'N':
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, RIVER_RAID_BLUE); // Fundo
                DrawTexturePro(sprites, NAVIO, NAVIO_destino, Vector2{ 0, 0 }, 0, WHITE);
                break;
            case 'X':
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, RIVER_RAID_BLUE);
                DrawTexturePro(sprites, HELICOPTERO, destino, Vector2{ 0, 0 }, 0, WHITE);
                break;
            case 'G':
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, RIVER_RAID_BLUE);
                DrawTexturePro(sprites, GAS, destino, Vector2{ 0, 0 }, 0, WHITE);
                break;
            case 'P':
                DrawRectangle(screen_x, screen_y, TILE_SIZE, TILE_SIZE, RIVER_RAID_BLUE);
                DrawTexturePro(sprites, PONTE, destino, Vector2{ 0, 0 }, 0, WHITE);
                break;
            case ' ':
            default: DrawRectangle(destino.x, destino.y, TILE_SIZE, TILE_SIZE, RIVER_RAID_BLUE); break;
            }
        }
    }
}

 int posicao_valida_nave(int pos_x, int pos_y, Mapa* mapa) {
    int tile_x = pos_x / TILE_SIZE;
    int tile_y = pos_y / TILE_SIZE;
    if (tile_x < 0 || tile_x >= LARGURA_MAPA || tile_y < 0 || tile_y >= ALTURA_MAPA) return 0;
    return mapa->quadradinhos[tile_y][tile_x] != 'T';
}

int colisao_ponte(Mapa* mapa, int nave_x, int nave_y, int tamanho) {
    int pontos_x[4] = { nave_x, nave_x + tamanho - 1, nave_x, nave_x + tamanho - 1 };
    int pontos_y[4] = { nave_y, nave_y, nave_y + tamanho - 1, nave_y + tamanho - 1 };
    int ponte_encontrada = 0;
    int ponte_y = -1;
    for (int i = 0; i < 4; i++) {
        int tile_x = pontos_x[i] / TILE_SIZE;
        int tile_y = pontos_y[i] / TILE_SIZE;
        if (tile_x < 0 || tile_x >= LARGURA_MAPA || tile_y < 0 || tile_y >= ALTURA_MAPA) continue;
        if (mapa->quadradinhos[tile_y][tile_x] == 'P') {
            ponte_encontrada = 1;
            ponte_y = tile_y;
            break;
        }
    }
    if (ponte_encontrada && ponte_y != -1) {
        // Colisão física com ponte (sem ser míssil)
        return 1;
    }
    return 0;
}

int encontrar_pos_spawn(Mapa* mapa, int* out_x, int* out_y) {
    int try_y = ALTURA - TILE_SIZE - 1;
    if (try_y < 0) try_y = 0;
    for (int tx = 0; tx < LARGURA_MAPA; tx++) {
        int px = tx * TILE_SIZE;
        if (posicao_valida_nave(px, try_y, mapa) &&
            posicao_valida_nave(px + TILE_SIZE - 1, try_y, mapa) &&
            posicao_valida_nave(px, try_y + TILE_SIZE - 1, mapa) &&
            posicao_valida_nave(px + TILE_SIZE - 1, try_y + TILE_SIZE - 1, mapa)) {
            *out_x = px;
            *out_y = try_y;
            return 1;
        }
    }
    for (int dy = 1; dy <= 3; dy++) {
        int ty = try_y - dy * TILE_SIZE;
        if (ty < 0) break;
        for (int tx = 0; tx < LARGURA_MAPA; tx++) {
            int px = tx * TILE_SIZE;
            if (posicao_valida_nave(px, ty, mapa) &&
                posicao_valida_nave(px + TILE_SIZE - 1, ty, mapa)) {
                *out_x = px;
                *out_y = ty;
                return 1;
            }
        }
    }
    return 0;
}

void resetar_jogo(void) {
    fase_atual = 1;
    score_atual = 0;
    jogo_completo = 0;
    velocidade_nave = 3;

    // Inicializar combustível e estados de morte
    combustivel = MAX_COMBUSTIVEL;
    player_explodindo = 0;
    timer_gameover = 0.0f;
    for (int i = 0; i < MAX_EXPLOSOES; i++) lista_explosoes[i].ativo = 0;

    POSICAOX_NAVE = LARGURA / 2;
    POSICAOY_NAVE = ALTURA - 100;
    POSICAOX_MISSIL = -100;
    POSICAOY_MISSIL = -100;
    sprintf_s(texto, sizeof(texto), "River Raid INF - Fase %d", fase_atual);
    carregar_mapa("assets/fase1.txt", &mapa_atual);
    int sx, sy;
    if (encontrar_pos_spawn(&mapa_atual, &sx, &sy)) {
        POSICAOX_NAVE = sx;
        POSICAOY_NAVE = sy;
    }
}
void mover_barcos(Mapa* mapa) {
    double tempo_atual = GetTime();
    if (tempo_atual - ultimo_movimento_barcos < intervalo_movimento_barcos) 
        return;
    ultimo_movimento_barcos = tempo_atual;
    for (int y = 0; y < ALTURA_MAPA; y++) {
        for (int x = 0; x < LARGURA_MAPA; x++) {
            if (mapa->quadradinhos[y][x] == 'N') {
                int direcao = GetRandomValue(-1, 1);
                if (direcao == 0) continue;
                int novo_x = x + direcao;
                if (novo_x >= 0 && novo_x < LARGURA_MAPA && mapa->quadradinhos[y][novo_x] == ' ') {
                    mapa->quadradinhos[y][x] = ' ';
                    mapa->quadradinhos[y][novo_x] = 'N';
                }
            }
        }
    }
}
void mover_helicopteros(Mapa* mapa) {
    double tempo_atual = GetTime();
    if (tempo_atual - ultimo_movimento_helicopteros < intervalo_movimento_helicopteros) 
        return;
    ultimo_movimento_helicopteros = tempo_atual;
    for (int y = 0; y < ALTURA_MAPA; y++) {
        for (int x = 0; x < LARGURA_MAPA; x++) {
            if (mapa->quadradinhos[y][x] == 'X') {
                int direcao = GetRandomValue(-1, 1);
                if (direcao == 0) continue;
                int novo_x = x + direcao;
                if (novo_x >= 0 && novo_x < LARGURA_MAPA && mapa->quadradinhos[y][novo_x] == ' ') {
                    mapa->quadradinhos[y][x] = ' ';
                    mapa->quadradinhos[y][novo_x] = 'X';
                }
            }
        }
    }
}

int main() {
    FILE* arquivo;
    POSICAOX_NAVE = LARGURA / 2;
    POSICAOY_NAVE = ALTURA - 100;
    POSICAOX_MISSIL = -100;
    POSICAOY_MISSIL = -100;
    sprintf_s(texto, sizeof(texto), "River Raid INF - Fase %d", fase_atual);


    TelaJogo TelaAgora = TELA_INICIAL;
    InitWindow(LARGURA, ALTURA, "River Raid INF");
    SetTargetFPS(60);

    carregar_ranking(ranking);

    if (!carregar_mapa("assets/fase1.txt", &mapa_atual)) {
        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(RED);
            DrawText("ERRO: Nao foi possivel carregar fase1.txt", 50, ALTURA / 2, 20, WHITE);
            EndDrawing();
        }
        return 1;
    }

    Texture2D SPRITES = LoadTexture("assets/sprites.png");
    if (SPRITES.id == 0) printf("AVISO: Nao foi possivel carregar sprites.png.\n");

    Rectangle NAVE_CENTRO = { 103, 70,  56, 52 };
    // Sprites para lados
    Rectangle NAVE_ESQ = { 45, 70, 56, 52 };
    Rectangle NAVE_DIR = { 161, 70, 56, 52 };
    Rectangle MISSIL = { 0, 70, 40, 50 };

    // Frames da explosão
    Rectangle FRAMES_EXPLOSAO[5] = {
        { 220, 60, 45, 45 }, { 270, 60, 45, 45 }, { 320, 60, 45, 45 },
        { 370, 60, 45, 45 }, { 420, 60, 45, 45 }
    };

    TelaJogo TelaAnterior = (TelaJogo)-1;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (TelaAgora != TelaAnterior) {
            if (TelaAgora == NOVO_JOGO) {
                sprintf_s(texto, sizeof(texto), "River Raid INF - Fase %d", fase_atual);
                int sx, sy;
                if (encontrar_pos_spawn(&mapa_atual, &sx, &sy)) {
                    POSICAOX_NAVE = sx; POSICAOY_NAVE = sy;
                }
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
        case RANKING:
            TelaAgora = TelaRanking();
            break;
        case NOVO_JOGO:
            if (!jogo_completo) {

                // Se estiver explodindo, roda animação e espera
                if (player_explodindo) {
                    atualizar_explosoes(dt);
                    timer_gameover += dt;
                    if (timer_gameover > 1.5f) { // 1.5s de delay antes do game over
                        TelaAgora = GAME_OVER;
                    }
                }
                else {

                    // Combustível
                    combustivel -= CONSUMO_GASOLINA * dt;
                    if (combustivel <= 0) {
                        combustivel = 0;
                        adicionar_explosao((float)POSICAOX_NAVE, (float)POSICAOY_NAVE);
                        player_explodindo = 1;
                    }

                    // ADIÇÃO: Recarga (passar por cima do 'G')
                    int cx = (POSICAOX_NAVE + TILE_SIZE / 2) / TILE_SIZE;
                    int cy = (POSICAOY_NAVE + TILE_SIZE / 2) / TILE_SIZE;
                    if (cx >= 0 && cx < LARGURA_MAPA && cy >= 0 && cy < ALTURA_MAPA) {
                        if (mapa_atual.quadradinhos[cy][cx] == 'G') {
                            combustivel += 40.0f * dt;
                            if (combustivel > MAX_COMBUSTIVEL) combustivel = MAX_COMBUSTIVEL;
                        }
                    }

                    POSICAOY_NAVE -= velocidade_nave;
                    mover_helicopteros(&mapa_atual);
                    mover_barcos(&mapa_atual);

                    // Atualiza explosões dos inimigos
                    atualizar_explosoes(dt);

                    if (POSICAOY_NAVE + TILE_SIZE <= 0) {
                        if (carregar_proxima_fase()) {
                            int sx, sy;
                            if(encontrar_pos_spawn(&mapa_atual, &sx, &sy)) {
                                // POSICAOX_NAVE = sx; luiza: comentei essa linha pq era ela que tava dando aquele problema de quando ela mudava de fase. nao quis tirar a função toda pois tive medo
                                POSICAOY_NAVE = sy;
                            }
                        else {
                            POSICAOY_NAVE = ALTURA - 100;
                        }
                            sprintf_s(texto, sizeof(texto), "River Raid INF - Fase %d", fase_atual);
                            break;
                        }
                        else {
                            jogo_completo = 1;
                        }
                    }

                    // Controles
                    Rectangle nave_source = NAVE_CENTRO; // Sprite padrão
                    {
                        int nova_pos_x = POSICAOX_NAVE;
                        if (IsKeyDown(KEY_RIGHT)) {
                            nova_pos_x += 5;
                            nave_source = NAVE_DIR; // Sprite Direita
                        }
                        if (IsKeyDown(KEY_LEFT)) {
                            nova_pos_x -= 5;
                            nave_source = NAVE_ESQ; //Sprite Esquerda
                        }
                        if (IsKeyDown(KEY_DOWN)) POSICAOY_NAVE += 2;
                        if (IsKeyDown(KEY_UP)) POSICAOY_NAVE -= 5;
                        if (IsKeyPressed(KEY_BACKSPACE)) TelaAgora = SALVARESAIR;

                        if (posicao_valida_nave(nova_pos_x, POSICAOY_NAVE, &mapa_atual) &&
                            posicao_valida_nave(nova_pos_x + TILE_SIZE - 1, POSICAOY_NAVE, &mapa_atual) &&
                            posicao_valida_nave(nova_pos_x, POSICAOY_NAVE + TILE_SIZE - 1, &mapa_atual) &&
                            posicao_valida_nave(nova_pos_x + TILE_SIZE - 1, POSICAOY_NAVE + TILE_SIZE - 1, &mapa_atual)) {
                            POSICAOX_NAVE = nova_pos_x;
                        }

                        // Colisão física com ponte (morte)
                        if (colisao_ponte(&mapa_atual, POSICAOX_NAVE, POSICAOY_NAVE, TILE_SIZE)) {
                            adicionar_explosao((float)POSICAOX_NAVE, (float)POSICAOY_NAVE);
                            player_explodindo = 1;
                        }

                        if (POSICAOX_NAVE < 0) POSICAOX_NAVE = 0;
                        if (POSICAOX_NAVE > LARGURA - TILE_SIZE) POSICAOX_NAVE = LARGURA - TILE_SIZE;
                    }

                    if (IsKeyPressed(KEY_SPACE) && POSICAOY_MISSIL < -50) {
                        POSICAOX_MISSIL = POSICAOX_NAVE + (TILE_SIZE / 4);
                        POSICAOY_MISSIL = POSICAOY_NAVE - 40;
                    }

                    if (POSICAOY_MISSIL > -50) {
                        POSICAOY_MISSIL -= 10;
                        if (POSICAOY_MISSIL < -TILE_SIZE) {
                            POSICAOY_MISSIL = -100; POSICAOX_MISSIL = -100;
                        }
                    }

                    if (POSICAOY_MISSIL > -50) {
                        if (colisao_missil(&mapa_atual, POSICAOX_MISSIL, POSICAOY_MISSIL, &score_atual)) {
                            POSICAOY_MISSIL = -100; POSICAOX_MISSIL = -100;
                        }
                    }

                    // Colisão física com inimigos/terreno
                    if (colisao_nave(&mapa_atual, POSICAOX_NAVE, POSICAOY_NAVE, TILE_SIZE)) {
                        adicionar_explosao((float)POSICAOX_NAVE, (float)POSICAOY_NAVE);
                        player_explodindo = 1;
                    }

                    // DESENHO
                    BeginDrawing();
                    ClearBackground(RIVER_RAID_BLUE);
                    desenhar_mapa(&mapa_atual, SPRITES);

                    if (SPRITES.id != 0) {
                        // Só desenha a nave se ela não estiver explodindo
                        if (!player_explodindo) {
                            Rectangle destNave = { (float)POSICAOX_NAVE, (float)POSICAOY_NAVE, TILE_SIZE, TILE_SIZE };
                            DrawTexturePro(SPRITES, nave_source, destNave, Vector2{ 0, 0 }, 0, WHITE);
                        }
                        if (POSICAOY_MISSIL > -50) {
                            Rectangle destMissil = { (float)POSICAOX_MISSIL, (float)POSICAOY_MISSIL, TILE_SIZE / 2, TILE_SIZE };
                            DrawTexturePro(SPRITES, MISSIL, destMissil, Vector2{ 0, 0 }, 0, WHITE);
                        }
                    }
                    else {
                        DrawRectangle(POSICAOX_NAVE, POSICAOY_NAVE, TILE_SIZE, TILE_SIZE, WHITE);
                    }

                    // Desenhar Explosões
                    for (int i = 0; i < MAX_EXPLOSOES; i++) {
                        if (lista_explosoes[i].ativo) {
                            Rectangle destExp = { lista_explosoes[i].x, lista_explosoes[i].y, TILE_SIZE, TILE_SIZE };
                            DrawTexturePro(SPRITES, FRAMES_EXPLOSAO[lista_explosoes[i].frame_atual], destExp, Vector2{ 0,0 }, 0, WHITE);
                        }
                    }

                    DrawRectangle(0, 0, LARGURA, 30, Fade(BLACK, 0.7f));
                    DrawText(texto, 10, 5, 20, WHITE);
                    DrawText(TextFormat("Score: %d", score_atual), LARGURA / 2 - 40, 5, 20, WHITE);

                    // HUD Combustível
                    DrawText("Combustivel", LARGURA - 280, 5, 20, WHITE);
                    DrawRectangle(LARGURA - 160, 5, 150, 20, WHITE);
                    Color corComb = GREEN;
                    if (combustivel < 30) corComb = RED;
                    DrawRectangle(LARGURA - 160, 5, (int)(150 * (combustivel / MAX_COMBUSTIVEL)), 20, corComb);

                    if (jogo_completo) {
                        DrawRectangle(LARGURA / 2 - 150, ALTURA / 2 - 50, 300, 100, Fade(BLACK, 0.8f));
                        DrawText("JOGO COMPLETO!", LARGURA / 2 - 120, ALTURA / 2 - 30, 30, GREEN);
                        TelaAgora = TelaNovoHighScore();
                    }
                    EndDrawing();
                }
            }
            break;

        case GAME_OVER: 
            TelaAgora = TelaGameOver();
            // Lógica de transição para a tela de Novo High Score
           
            break;
        case NOVO_HIGH_SCORE:
            TelaAgora = TelaNovoHighScore();
            break;
        default: break;
        }
        TelaAnterior = TelaAgora;
    }

    if (SPRITES.id != 0) UnloadTexture(SPRITES);
    CloseWindow();
    return 0;
}