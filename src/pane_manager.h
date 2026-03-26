#ifndef PANE_MANAGER_H
#define PANE_MANAGER_H

#include <stdbool.h>

#include "raylib.h"

#define MAX_PANES 64

typedef struct Pane {
	int id;
	Rectangle bounds;
	char title[64];
	bool focused;
} Pane;

typedef struct PaneManager {
	Pane panes[MAX_PANES];
	int paneCount;
	int nextPaneId;
	int focusedPaneIndex;
	int draggingPaneIndex;
	Vector2 dragOffset;
	bool isPointerOverPane;
} PaneManager;

void PaneManager_Init(PaneManager *manager);
void PaneManager_Update(PaneManager *manager, Camera2D camera, bool canvasPanActive);
void PaneManager_Draw(const PaneManager *manager, Camera2D camera);
int PaneManager_GetPaneCount(const PaneManager *manager);
bool PaneManager_IsPointerOverPane(const PaneManager *manager);

#endif
