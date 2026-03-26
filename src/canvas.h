#ifndef CANVAS_H
#define CANVAS_H

#include "raylib.h"

#define MAX_DEBUG_SQUARES 1024
#define DEBUG_SQUARE_SIZE 48.0f

typedef struct DebugSquare {
	Vector2 position;
	float size;
	Color color;
} DebugSquare;

typedef struct Canvas {
	Camera2D camera;
	DebugSquare squares[MAX_DEBUG_SQUARES];
	int squareCount;
	Vector2 hoverWorldPosition;
} Canvas;

void Canvas_Init(Canvas *canvas);
void Canvas_Update(Canvas *canvas);
void Canvas_Draw(const Canvas *canvas);
void Canvas_DrawOverlay(const Canvas *canvas);

#endif
