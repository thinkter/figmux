#ifndef PERFORMANCE_HUD_H
#define PERFORMANCE_HUD_H

#include <stddef.h>

#include "canvas.h"

typedef struct PerformanceHud {
	float averageFrameTimeMs;
	float peakFrameTimeMs;
	float elapsedSecondsSincePeak;
	size_t residentMemoryBytes;
} PerformanceHud;

void PerformanceHud_Init(PerformanceHud *hud);
void PerformanceHud_Update(PerformanceHud *hud, const Canvas *canvas);
void PerformanceHud_Draw(const PerformanceHud *hud, const Canvas *canvas);

#endif
