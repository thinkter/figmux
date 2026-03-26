#include "raylib.h"

#include "app.h"
#include "resource_dir.h"

int main(void)
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 800, "Hello Raylib");
	SetExitKey(KEY_NULL);
	// SetTargetFPS(144);

	SearchAndSetResourceDir("resources");

	App app = {0};
	App_Init(&app);

	while (!WindowShouldClose())
	{
		App_Update(&app);

		BeginDrawing();
		App_Draw(&app);
		EndDrawing();
	}

	App_Shutdown(&app);
	CloseWindow();
	return 0;
}
