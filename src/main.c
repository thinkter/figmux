#include "raylib.h"
#include "raymath.h"

#include "resource_dir.h"	// utility header for SearchAndSetResourceDir

#define MAX_DEBUG_SQUARES 1024
#define DEBUG_SQUARE_SIZE 48.0f

typedef struct DebugSquare {
	Vector2 position;
	float size;
	Color color;
} DebugSquare;

int main ()
{
	// Tell the window to use vsync, allow resizing, and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);

	// Create the window and OpenGL context
	InitWindow(1280, 800, "Hello Raylib");

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	Camera2D camera = {0};
	camera.target = (Vector2){ 0.0f, 0.0f };
	camera.offset = (Vector2){ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	DebugSquare squares[MAX_DEBUG_SQUARES] = {0};
	int squareCount = 0;
	
	// game loop
	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		camera.offset = (Vector2){ GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };

		bool isAltDown = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
		bool isPanningWithLeftMouse = isAltDown && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
		bool isPanningWithMiddleMouse = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);

		if (isPanningWithMiddleMouse || isPanningWithLeftMouse)
		{
			Vector2 delta = GetMouseDelta();
			delta = Vector2Scale(delta, -1.0f / camera.zoom);
			camera.target = Vector2Add(camera.target, delta);
		}

		float wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			Vector2 mouseWorldBeforeZoom = GetScreenToWorld2D(GetMousePosition(), camera);
			float zoomStep = 0.1f * wheel;
			camera.zoom += zoomStep;
			if (camera.zoom < 0.1f)
			{
				camera.zoom = 0.1f;
			}
			if (camera.zoom > 8.0f)
			{
				camera.zoom = 8.0f;
			}
			Vector2 mouseWorldAfterZoom = GetScreenToWorld2D(GetMousePosition(), camera);
			camera.target = Vector2Add(camera.target, Vector2Subtract(mouseWorldBeforeZoom, mouseWorldAfterZoom));
		}

		Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
		if (!isAltDown && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && squareCount < MAX_DEBUG_SQUARES)
		{
			squares[squareCount++] = (DebugSquare) {
				.position = {
					mouseWorld.x - (DEBUG_SQUARE_SIZE * 0.5f),
					mouseWorld.y - (DEBUG_SQUARE_SIZE * 0.5f)
				},
				.size = DEBUG_SQUARE_SIZE,
				.color = ORANGE
			};
		}

		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground((Color){ 18, 18, 24, 255 });

		BeginMode2D(camera);

		const int gridExtent = 4000;
		const int gridStep = 64;
		for (int x = -gridExtent; x <= gridExtent; x += gridStep)
		{
			Color lineColor = (x == 0) ? (Color){ 70, 120, 220, 255 } : (Color){ 46, 46, 58, 255 };
			DrawLine(x, -gridExtent, x, gridExtent, lineColor);
		}
		for (int y = -gridExtent; y <= gridExtent; y += gridStep)
		{
			Color lineColor = (y == 0) ? (Color){ 220, 90, 90, 255 } : (Color){ 46, 46, 58, 255 };
			DrawLine(-gridExtent, y, gridExtent, y, lineColor);
		}

		for (int i = 0; i < squareCount; i++)
		{
			DrawRectangleV(squares[i].position, (Vector2){ squares[i].size, squares[i].size }, squares[i].color);
			DrawRectangleLinesEx(
				(Rectangle){ squares[i].position.x, squares[i].position.y, squares[i].size, squares[i].size },
				2.0f / camera.zoom,
				RAYWHITE
			);
		}

		DrawRectangleV(
			(Vector2){ mouseWorld.x - (DEBUG_SQUARE_SIZE * 0.5f), mouseWorld.y - (DEBUG_SQUARE_SIZE * 0.5f) },
			(Vector2){ DEBUG_SQUARE_SIZE, DEBUG_SQUARE_SIZE },
			(Color){ 255, 255, 255, 40 }
		);
		DrawRectangleLinesEx(
			(Rectangle){ mouseWorld.x - (DEBUG_SQUARE_SIZE * 0.5f), mouseWorld.y - (DEBUG_SQUARE_SIZE * 0.5f), DEBUG_SQUARE_SIZE, DEBUG_SQUARE_SIZE },
			2.0f / camera.zoom,
			(Color){ 255, 255, 255, 190 }
		);

		EndMode2D();

		DrawRectangle(16, 16, 400, 96, (Color){ 0, 0, 0, 140 });
		DrawText("Middle mouse or Alt+Left: drag canvas", 32, 28, 20, RAYWHITE);
		DrawText("Mouse wheel: zoom to cursor", 32, 54, 20, RAYWHITE);
		DrawText(TextFormat("Left click: place square | Count: %d", squareCount), 32, 80, 20, RAYWHITE);
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
