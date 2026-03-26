#ifndef CANVAS_H
#define CANVAS_H

#include <stdbool.h>

#include "raylib.h"

#include "pane_manager.h"

typedef struct Canvas {
	Camera2D camera;
	bool isPanning;
	PaneManager paneManager;
} Canvas;

void Canvas_Init(Canvas *canvas);
void Canvas_Update(Canvas *canvas);
void Canvas_Draw(const Canvas *canvas);
void Canvas_DrawOverlay(const Canvas *canvas);

#endif
