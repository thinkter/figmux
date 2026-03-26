#include "canvas.h"

#include "raymath.h"

static const Color kBackgroundColor = { 18, 18, 24, 255 };
static const Color kGridColor = { 46, 46, 58, 255 };
static const Color kXAxisColor = { 70, 120, 220, 255 };
static const Color kYAxisColor = { 220, 90, 90, 255 };
static const Color kPreviewFillColor = { 255, 255, 255, 40 };
static const Color kPreviewOutlineColor = { 255, 255, 255, 190 };
static const Color kOverlayColor = { 0, 0, 0, 140 };

static const int kGridExtent = 4000;
static const int kGridStep = 64;
static const float kMinZoom = 0.1f;
static const float kMaxZoom = 8.0f;
static const float kZoomStep = 0.1f;

static bool Canvas_IsAltDown(void)
{
	return IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
}

static void Canvas_UpdateOffset(Canvas *canvas)
{
	canvas->camera.offset = (Vector2){ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
}

static void Canvas_HandlePan(Canvas *canvas)
{
	bool isPanningWithLeftMouse = Canvas_IsAltDown() && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
	bool isPanningWithMiddleMouse = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);

	if (!isPanningWithLeftMouse && !isPanningWithMiddleMouse)
	{
		return;
	}

	Vector2 delta = GetMouseDelta();
	delta = Vector2Scale(delta, -1.0f / canvas->camera.zoom);
	canvas->camera.target = Vector2Add(canvas->camera.target, delta);
}

static void Canvas_HandleZoom(Canvas *canvas)
{
	float wheel = GetMouseWheelMove();
	if (wheel == 0.0f)
	{
		return;
	}

	Vector2 mouseWorldBeforeZoom = GetScreenToWorld2D(GetMousePosition(), canvas->camera);
	canvas->camera.zoom += kZoomStep * wheel;
	canvas->camera.zoom = Clamp(canvas->camera.zoom, kMinZoom, kMaxZoom);

	Vector2 mouseWorldAfterZoom = GetScreenToWorld2D(GetMousePosition(), canvas->camera);
	Vector2 zoomOffset = Vector2Subtract(mouseWorldBeforeZoom, mouseWorldAfterZoom);
	canvas->camera.target = Vector2Add(canvas->camera.target, zoomOffset);
}

static void Canvas_HandleSquarePlacement(Canvas *canvas)
{
	if (Canvas_IsAltDown() || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		return;
	}

	if (canvas->squareCount >= MAX_DEBUG_SQUARES)
	{
		return;
	}

	canvas->squares[canvas->squareCount++] = (DebugSquare) {
		.position = {
			canvas->hoverWorldPosition.x - (DEBUG_SQUARE_SIZE * 0.5f),
			canvas->hoverWorldPosition.y - (DEBUG_SQUARE_SIZE * 0.5f)
		},
		.size = DEBUG_SQUARE_SIZE,
		.color = ORANGE
	};
}

static void Canvas_DrawGrid(void)
{
	for (int x = -kGridExtent; x <= kGridExtent; x += kGridStep)
	{
		Color color = (x == 0) ? kXAxisColor : kGridColor;
		DrawLine(x, -kGridExtent, x, kGridExtent, color);
	}

	for (int y = -kGridExtent; y <= kGridExtent; y += kGridStep)
	{
		Color color = (y == 0) ? kYAxisColor : kGridColor;
		DrawLine(-kGridExtent, y, kGridExtent, y, color);
	}
}

static void Canvas_DrawPlacedSquares(const Canvas *canvas)
{
	for (int i = 0; i < canvas->squareCount; i++)
	{
		const DebugSquare *square = &canvas->squares[i];
		DrawRectangleV(square->position, (Vector2){ square->size, square->size }, square->color);
		DrawRectangleLinesEx(
			(Rectangle){ square->position.x, square->position.y, square->size, square->size },
			2.0f / canvas->camera.zoom,
			RAYWHITE
		);
	}
}

static void Canvas_DrawHoverPreview(const Canvas *canvas)
{
	Rectangle preview = {
		canvas->hoverWorldPosition.x - (DEBUG_SQUARE_SIZE * 0.5f),
		canvas->hoverWorldPosition.y - (DEBUG_SQUARE_SIZE * 0.5f),
		DEBUG_SQUARE_SIZE,
		DEBUG_SQUARE_SIZE
	};

	DrawRectangleRec(preview, kPreviewFillColor);
	DrawRectangleLinesEx(preview, 2.0f / canvas->camera.zoom, kPreviewOutlineColor);
}

void Canvas_Init(Canvas *canvas)
{
	canvas->camera = (Camera2D){
		.offset = { 0.0f, 0.0f },
		.target = { 0.0f, 0.0f },
		.rotation = 0.0f,
		.zoom = 1.0f
	};
	canvas->squareCount = 0;
	canvas->hoverWorldPosition = (Vector2){ 0.0f, 0.0f };

	Canvas_UpdateOffset(canvas);
}

void Canvas_Update(Canvas *canvas)
{
	Canvas_UpdateOffset(canvas);
	Canvas_HandlePan(canvas);
	Canvas_HandleZoom(canvas);

	canvas->hoverWorldPosition = GetScreenToWorld2D(GetMousePosition(), canvas->camera);
	Canvas_HandleSquarePlacement(canvas);
}

void Canvas_Draw(const Canvas *canvas)
{
	ClearBackground(kBackgroundColor);

	BeginMode2D(canvas->camera);
	Canvas_DrawGrid();
	Canvas_DrawPlacedSquares(canvas);
	Canvas_DrawHoverPreview(canvas);
	EndMode2D();
}

void Canvas_DrawOverlay(const Canvas *canvas)
{
	DrawRectangle(16, 16, 400, 96, kOverlayColor);
	DrawText("Middle mouse or Alt+Left: drag canvas", 32, 28, 20, RAYWHITE);
	DrawText("Mouse wheel: zoom to cursor", 32, 54, 20, RAYWHITE);
	DrawText(TextFormat("Left click: place square | Count: %d", canvas->squareCount), 32, 80, 20, RAYWHITE);
}
