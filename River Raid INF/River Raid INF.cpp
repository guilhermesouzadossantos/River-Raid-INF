#include "raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define RIVER_RAID_BLUE Color{39, 43, 174, 255}
#define LARGURA 800
#define ALTURA 450

int main() {
	char texto[50] = "Pressione uma seta"; //Texto inicial
	int POSICAOX_NAVE = 400;
	int POSICAOY_NAVE = 450;
	int POSICAOX_MISSIL = -100;
	int POSICAOY_MISSIL = -100;
	int POSICAOX_INIMIGO_BARCO = 200;
	int POSICAOY_INIMIGO_BARCO = 120;


	InitWindow(LARGURA, ALTURA, "River Raid INF"); //Inicializa janela, com certo tamanho e titulo
	SetTargetFPS(30);

	// SPRITES
	Texture2D SPRITES = LoadTexture("assets/sprites.png");
	Rectangle NAVE = { 103, 70,  56, 52 };
	Rectangle NAVE_ESQUERDA = { 51,  74,  40, 56 };
	Rectangle NAVE_DIREITA = { 171, 74,  40, 56 };
	Rectangle MISSIL = { 0, 70, 40, 50 };
	Rectangle INIMIGO_BARCO = { 15,  234, 128, 32 };
	Rectangle INIMIGO_AVIAO1_1 = { 11,  186, 64, 40 };
	Rectangle INIMIGO_AVIAO1_2 = { 83,  186, 64, 40 };


	//Este laco repete enquanto a janela nao for fechada
	while (!WindowShouldClose())
	{
		Vector2 position_nave = {POSICAOX_NAVE, POSICAOY_NAVE}; // Movimento da nave
		Vector2 position_missil = {POSICAOX_MISSIL, POSICAOY_MISSIL}; // Movimento da nave
		Vector2 position_inimigo_barco = { POSICAOX_INIMIGO_BARCO, POSICAOY_INIMIGO_BARCO };

		// cria retângulos de colisão em coordenadas de tela (world rects)
		Rectangle colisao_nave = { POSICAOX_NAVE, POSICAOY_NAVE, 56, 52 };
		Rectangle colisao_missil = { POSICAOX_MISSIL, POSICAOY_MISSIL, 40, 50 };
		Rectangle colisao_inimigo_barco = { POSICAOX_INIMIGO_BARCO, POSICAOY_INIMIGO_BARCO, 128, 32 };



		if (POSICAOX_NAVE < 0)   {POSICAOX_NAVE = 0;} // Barreira da tela (esquerda)
		if (POSICAOX_NAVE > 800) {POSICAOX_NAVE = 0;} // Barreira da tela (direita)

		POSICAOY_NAVE -= 5; // Velocidade da nave do jogador
		POSICAOY_MISSIL -= 15; // Velocidade da nave do missil

		
		


		// Trata entrada do usuario e atualiza estado do jogo
		if (IsKeyPressed(KEY_RIGHT)) {
			POSICAOX_NAVE += 40;
		}
		if (IsKeyPressed(KEY_LEFT)) {
			POSICAOX_NAVE -= 40;
		}
		if (IsKeyPressed(KEY_DOWN)) {
			POSICAOY_NAVE += 15;
		}
		if (POSICAOY_NAVE == 0) {
			POSICAOY_NAVE = 450;
		}
		if (IsKeyPressed(KEY_SPACE) && POSICAOY_MISSIL < -50) {
			POSICAOX_MISSIL = POSICAOX_NAVE;
			POSICAOY_MISSIL = POSICAOY_NAVE - 60;
		}

		// Atualiza o que eh mostrado na tela a partir do estado do jogo
		BeginDrawing(); //Inicia o ambiente de desenho na tela
		ClearBackground(RIVER_RAID_BLUE); //Limpa a tela e define cor de fundo

		// COLLISION
		if (CheckCollisionRecs(colisao_missil, colisao_inimigo_barco) == true)
		{
			POSICAOX_INIMIGO_BARCO = -500;
			POSICAOY_MISSIL = -50;
		}
		//tested de git

		// DRAW
		DrawTextureRec(SPRITES, NAVE, position_nave, WHITE); // Desenha a nave do jogador
		DrawTextureRec(SPRITES, MISSIL, position_missil, WHITE); // Desenha o missil
		DrawTextureRec(SPRITES, INIMIGO_BARCO, position_inimigo_barco, WHITE);

		EndDrawing(); //Finaliza o ambiente de desenho na tela
	}

	UnloadTexture(SPRITES);
	CloseWindow(); // Fecha a janela
	return 0;
}
