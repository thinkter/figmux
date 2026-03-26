#ifndef APP_H
#define APP_H

#include "canvas.h"

typedef struct App {
	Canvas canvas;
} App;

void App_Init(App *app);
void App_Update(App *app);
void App_Draw(const App *app);

#endif
