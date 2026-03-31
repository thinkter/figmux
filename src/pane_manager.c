#include "pane_manager.h"

#include <stdio.h>

#include "raylib.h"
#include "raymath.h"

static const float kPaneMinWidth = 240.0f;
static const float kPaneMinHeight = 160.0f;
static const float kPaneDefaultWidth = 720.0f;
static const float kPaneDefaultHeight = 720.0f;
static const float kPaneTitlebarHeight = 34.0f;
static const float kPanePadding = 14.0f;
static const float kPaneCharacterWidth = 9.0f;
static const float kPaneLineHeight = 18.0f;

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

static Rectangle Pane_ContentBounds(Rectangle bounds)
{
	return (Rectangle){
		bounds.x + kPanePadding,
		bounds.y + kPaneTitlebarHeight + kPanePadding,
		bounds.width - (kPanePadding * 2.0f),
		bounds.height - kPaneTitlebarHeight - (kPanePadding * 2.0f)
	};
}

static void Pane_CalculateTerminalSize(const Pane *pane, int *columns, int *rows)
{
	Rectangle contentBounds = Pane_ContentBounds(pane->bounds);
	*columns = (int)(contentBounds.width / kPaneCharacterWidth);
	*rows = (int)(contentBounds.height / kPaneLineHeight);

	if (*columns < 20)
	{
		*columns = 20;
	}
	if (*rows < 6)
	{
		*rows = 6;
	}
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
	Pane_CalculateTerminalSize(pane, &pane->columns, &pane->rows);
	TerminalSession_Init(&pane->terminalSession, pane->columns, pane->rows);

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

void PaneManager_Shutdown(PaneManager *manager)
{
	for (int i = 0; i < manager->paneCount; i++)
	{
		TerminalSession_Shutdown(&manager->panes[i].terminalSession);
	}
}

static void PaneManager_HandleFocusedTerminalInput(PaneManager *manager)
{
	Pane *focusedPane = PaneManager_GetFocusedPane(manager);
	if (focusedPane == NULL)
	{
		return;
	}

	TerminalSession_HandleInput(&focusedPane->terminalSession);
}

void PaneManager_Update(PaneManager *manager, Camera2D camera, bool canvasPanActive)
{
	Vector2 mouseScreen = GetMousePosition();
	Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);

	if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_N))
	{
		PaneManager_AddPane(manager, mouseWorld);
	}

	for (int i = 0; i < manager->paneCount; i++)
	{
		TerminalSession_Update(&manager->panes[i].terminalSession);
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

	for (int i = 0; i < manager->paneCount; i++)
	{
		Pane *pane = &manager->panes[i];
		int columns = 0;
		int rows = 0;
		Pane_CalculateTerminalSize(pane, &columns, &rows);
		if (columns != pane->columns || rows != pane->rows)
		{
			pane->columns = columns;
			pane->rows = rows;
			TerminalSession_Resize(&pane->terminalSession, columns, rows, kPaneCharacterWidth, kPaneLineHeight);
		}
	}

	if (!canvasPanActive && manager->draggingPaneIndex < 0)
	{
		PaneManager_HandleFocusedTerminalInput(manager);
	}
}

void PaneManager_PrepareDraw(PaneManager *manager, Camera2D camera)
{
	for (int i = 0; i < manager->paneCount; i++)
	{
		Pane *pane = &manager->panes[i];
		Rectangle contentBounds = Pane_ContentBounds(pane->bounds);
		TerminalDrawParams drawParams = {
			.bounds = contentBounds,
			.camera = camera,
			.cellWidth = kPaneCharacterWidth,
			.cellHeight = kPaneLineHeight,
			.fontSize = 16,
			.textColor = kPaneTextColor,
			.mutedTextColor = kPaneMutedTextColor,
			.accentColor = kPaneAccentColor
		};
		TerminalSession_PrepareDraw(&pane->terminalSession, &drawParams);
	}
}

void PaneManager_Draw(const PaneManager *manager, Camera2D camera)
{
	for (int i = 0; i < manager->paneCount; i++)
	{
		const Pane *pane = &manager->panes[i];
		Rectangle titlebar = Pane_TitlebarBounds(pane->bounds);
		Rectangle contentBounds = Pane_ContentBounds(pane->bounds);
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

		int badgeFontSize = (int)(14.0f / camera.zoom);
		if (badgeFontSize < 9)
		{
			badgeFontSize = 9;
		}

		DrawText(
			TerminalSession_GetBackendName(&pane->terminalSession),
			(int)(pane->bounds.x + pane->bounds.width - 132.0f),
			(int)(pane->bounds.y + 10.0f),
			badgeFontSize,
			TerminalSession_UsingRealTerminalCore(&pane->terminalSession) ? kPaneMutedTextColor : kPaneAccentColor
		);

		TerminalDrawParams drawParams = {
			.bounds = contentBounds,
			.camera = camera,
			.cellWidth = kPaneCharacterWidth,
			.cellHeight = kPaneLineHeight,
			.fontSize = 16,
			.textColor = kPaneTextColor,
			.mutedTextColor = kPaneMutedTextColor,
			.accentColor = kPaneAccentColor
		};
		TerminalSession_Draw(&pane->terminalSession, &drawParams);

		if (!TerminalSession_IsActive(&pane->terminalSession) && TerminalSession_HasExited(&pane->terminalSession))
		{
			DrawText(
				"[shell exited]",
				(int)contentBounds.x,
				(int)(contentBounds.y + contentBounds.height - kPaneLineHeight),
				16,
				kPaneMutedTextColor
			);
		}
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
