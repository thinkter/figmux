#include "pane_manager.h"

#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

static const float kPaneMinWidth = 240.0f;
static const float kPaneMinHeight = 160.0f;
static const float kPaneDefaultWidth = 420.0f;
static const float kPaneDefaultHeight = 260.0f;
static const float kPaneTitlebarHeight = 34.0f;
static const float kPanePadding = 14.0f;

static const Color kPaneBodyColor = { 26, 29, 36, 235 };
static const Color kPaneHeaderColor = { 38, 44, 56, 255 };
static const Color kPaneOutlineColor = { 114, 129, 166, 255 };
static const Color kPaneBlurredOutlineColor = { 70, 78, 96, 220 };
static const Color kPaneTextColor = { 232, 236, 245, 255 };
static const Color kPaneMutedTextColor = { 160, 170, 188, 255 };
static const Color kPaneAccentColor = { 236, 178, 79, 255 };

static Rectangle Pane_TitlebarBounds(Rectangle bounds)
{
	return (Rectangle){ bounds.x, bounds.y, bounds.width, kPaneTitlebarHeight };
}

static Pane *PaneManager_GetFocusedPane(PaneManager *manager)
{
	if (manager->focusedPaneIndex < 0 || manager->focusedPaneIndex >= manager->paneCount)
	{
		return NULL;
	}

	return &manager->panes[manager->focusedPaneIndex];
}

static void PaneManager_UpdateFocusState(PaneManager *manager)
{
	for (int i = 0; i < manager->paneCount; i++)
	{
		manager->panes[i].focused = (i == manager->focusedPaneIndex);
	}
}

static void PaneManager_BringToFront(PaneManager *manager, int index)
{
	if (index < 0 || index >= manager->paneCount)
	{
		return;
	}

	if (index == (manager->paneCount - 1))
	{
		manager->focusedPaneIndex = index;
		manager->draggingPaneIndex = index;
		PaneManager_UpdateFocusState(manager);
		return;
	}

	Pane pane = manager->panes[index];
	for (int i = index; i < manager->paneCount - 1; i++)
	{
		manager->panes[i] = manager->panes[i + 1];
	}

	manager->panes[manager->paneCount - 1] = pane;
	manager->focusedPaneIndex = manager->paneCount - 1;
	manager->draggingPaneIndex = manager->focusedPaneIndex;
	PaneManager_UpdateFocusState(manager);
}

static void PaneManager_AddPane(PaneManager *manager, Vector2 worldPosition)
{
	if (manager->paneCount >= MAX_PANES)
	{
		return;
	}

	Pane *pane = &manager->panes[manager->paneCount];
	pane->id = manager->nextPaneId++;
	pane->bounds = (Rectangle){
		worldPosition.x - (kPaneDefaultWidth * 0.5f),
		worldPosition.y - (kPaneDefaultHeight * 0.5f),
		kPaneDefaultWidth,
		kPaneDefaultHeight
	};
	pane->bounds.width = fmaxf(pane->bounds.width, kPaneMinWidth);
	pane->bounds.height = fmaxf(pane->bounds.height, kPaneMinHeight);
	snprintf(pane->title, sizeof(pane->title), "Terminal %d", pane->id);
	pane->focused = false;

	manager->paneCount++;
	manager->focusedPaneIndex = manager->paneCount - 1;
	manager->draggingPaneIndex = -1;
	PaneManager_UpdateFocusState(manager);
}

static int PaneManager_FindTopPaneAt(const PaneManager *manager, Vector2 worldPosition)
{
	for (int i = manager->paneCount - 1; i >= 0; i--)
	{
		if (CheckCollisionPointRec(worldPosition, manager->panes[i].bounds))
		{
			return i;
		}
	}

	return -1;
}

void PaneManager_Init(PaneManager *manager)
{
	manager->paneCount = 0;
	manager->nextPaneId = 1;
	manager->focusedPaneIndex = -1;
	manager->draggingPaneIndex = -1;
	manager->dragOffset = (Vector2){ 0.0f, 0.0f };
	manager->isPointerOverPane = false;

	PaneManager_AddPane(manager, (Vector2){ 0.0f, 0.0f });
}

void PaneManager_Update(PaneManager *manager, Camera2D camera, bool canvasPanActive)
{
	Vector2 mouseScreen = GetMousePosition();
	Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);

	if (IsKeyPressed(KEY_N))
	{
		PaneManager_AddPane(manager, mouseWorld);
	}

	manager->isPointerOverPane = (PaneManager_FindTopPaneAt(manager, mouseWorld) >= 0);

	if (canvasPanActive)
	{
		manager->draggingPaneIndex = -1;
		return;
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		int hitIndex = PaneManager_FindTopPaneAt(manager, mouseWorld);
		if (hitIndex >= 0)
		{
			bool hitTitlebar = CheckCollisionPointRec(mouseWorld, Pane_TitlebarBounds(manager->panes[hitIndex].bounds));
			PaneManager_BringToFront(manager, hitIndex);

			Pane *focusedPane = PaneManager_GetFocusedPane(manager);
			if (focusedPane != NULL && hitTitlebar)
			{
				manager->dragOffset = Vector2Subtract(mouseWorld, (Vector2){ focusedPane->bounds.x, focusedPane->bounds.y });
			}
			else
			{
				manager->draggingPaneIndex = -1;
			}
		}
		else
		{
			manager->focusedPaneIndex = -1;
			manager->draggingPaneIndex = -1;
			PaneManager_UpdateFocusState(manager);
		}
	}

	if (manager->draggingPaneIndex >= 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		Pane *pane = PaneManager_GetFocusedPane(manager);
		if (pane != NULL)
		{
			pane->bounds.x = mouseWorld.x - manager->dragOffset.x;
			pane->bounds.y = mouseWorld.y - manager->dragOffset.y;
		}
	}

	if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
	{
		manager->draggingPaneIndex = -1;
	}
}

void PaneManager_Draw(const PaneManager *manager, Camera2D camera)
{
	(void)camera;

	for (int i = 0; i < manager->paneCount; i++)
	{
		const Pane *pane = &manager->panes[i];
		Rectangle titlebar = Pane_TitlebarBounds(pane->bounds);
		Color outlineColor = pane->focused ? kPaneOutlineColor : kPaneBlurredOutlineColor;

		DrawRectangleRounded(pane->bounds, 0.05f, 8, kPaneBodyColor);
		DrawRectangleRounded(titlebar, 0.08f, 8, kPaneHeaderColor);
		DrawRectangleLinesEx(pane->bounds, 2.0f / camera.zoom, outlineColor);

		DrawCircleV(
			(Vector2){ pane->bounds.x + kPanePadding, pane->bounds.y + (kPaneTitlebarHeight * 0.5f) },
			5.0f / camera.zoom,
			kPaneAccentColor
		);

		int fontSize = (int)(20.0f / camera.zoom);
		if (fontSize < 12)
		{
			fontSize = 12;
		}

		DrawText(
			pane->title,
			(int)(pane->bounds.x + 28.0f),
			(int)(pane->bounds.y + 8.0f),
			fontSize,
			kPaneTextColor
		);

		int bodyFontSize = (int)(18.0f / camera.zoom);
		if (bodyFontSize < 10)
		{
			bodyFontSize = 10;
		}

		DrawText(
			"Terminal host placeholder",
			(int)(pane->bounds.x + kPanePadding),
			(int)(pane->bounds.y + kPaneTitlebarHeight + 20.0f),
			bodyFontSize,
			kPaneTextColor
		);
		DrawText(
			"Drag the titlebar. Press N to spawn panes.",
			(int)(pane->bounds.x + kPanePadding),
			(int)(pane->bounds.y + kPaneTitlebarHeight + 48.0f),
			bodyFontSize,
			kPaneMutedTextColor
		);
		DrawText(
			TextFormat("Pane id: %d", pane->id),
			(int)(pane->bounds.x + kPanePadding),
			(int)(pane->bounds.y + kPaneTitlebarHeight + 88.0f),
			bodyFontSize,
			kPaneMutedTextColor
		);
	}
}

int PaneManager_GetPaneCount(const PaneManager *manager)
{
	return manager->paneCount;
}

bool PaneManager_IsPointerOverPane(const PaneManager *manager)
{
	return manager->isPointerOverPane;
}
