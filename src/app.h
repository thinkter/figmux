#ifndef APP_H
#define APP_H

#include "canvas.h"
#include "performance_hud.h"

typedef struct App {
	Canvas canvas;
	PerformanceHud performanceHud;
} App;

void App_Init(App *app);
void App_Shutdown(App *app);
void App_Update(App *app);
void App_Draw(App *app);

#endif
