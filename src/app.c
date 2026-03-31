#include "app.h"

void App_Init(App *app)
{
	Canvas_Init(&app->canvas);
	PerformanceHud_Init(&app->performanceHud);
}

void App_Shutdown(App *app)
{
	Canvas_Shutdown(&app->canvas);
}

void App_Update(App *app)
{
	Canvas_Update(&app->canvas);
	PerformanceHud_Update(&app->performanceHud, &app->canvas);
}

void App_Draw(App *app)
{
	Canvas_PrepareDraw(&app->canvas);
	Canvas_Draw(&app->canvas);
	Canvas_DrawOverlay(&app->canvas);
	PerformanceHud_Draw(&app->performanceHud, &app->canvas);
}
