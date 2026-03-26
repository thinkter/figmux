#include "performance_hud.h"

#include <stdio.h>
#include <unistd.h>

#include "raylib.h"

static const Color kHudBackgroundColor = { 0, 0, 0, 140 };
static const Color kHudLabelColor = { 210, 210, 220, 255 };
static const Color kHudValueColor = { 255, 255, 255, 255 };
static const int kHudFontSize = 20;
static const int kHudPadding = 16;
static const int kHudLineHeight = 24;
static const int kHudWidth = 320;
static const float kPeakHoldSeconds = 2.0f;
static const float kFrameSmoothing = 0.1f;
static const float kMsPerSecond = 1000.0f;
static const size_t kBytesPerMegabyte = 1024 * 1024;

static size_t PerformanceHud_ReadResidentMemoryBytes(void)
{
	FILE *statusFile = fopen("/proc/self/status", "r");
	if (statusFile == NULL)
	{
		return 0;
	}

	char line[256];
	size_t residentKilobytes = 0;
	while (fgets(line, sizeof(line), statusFile) != NULL)
	{
		if (sscanf(line, "VmRSS: %zu kB", &residentKilobytes) == 1)
		{
			break;
		}
	}

	fclose(statusFile);
	return residentKilobytes * 1024;
}

void PerformanceHud_Init(PerformanceHud *hud)
{
	hud->averageFrameTimeMs = 0.0f;
	hud->peakFrameTimeMs = 0.0f;
	hud->elapsedSecondsSincePeak = 0.0f;
	hud->residentMemoryBytes = 0;
}

void PerformanceHud_Update(PerformanceHud *hud, const Canvas *canvas)
{
	(void)canvas;

	float frameTimeMs = GetFrameTime() * kMsPerSecond;
	if (hud->averageFrameTimeMs <= 0.0f)
	{
		hud->averageFrameTimeMs = frameTimeMs;
	}
	else
	{
		hud->averageFrameTimeMs += (frameTimeMs - hud->averageFrameTimeMs) * kFrameSmoothing;
	}

	if (frameTimeMs >= hud->peakFrameTimeMs || hud->elapsedSecondsSincePeak >= kPeakHoldSeconds)
	{
		hud->peakFrameTimeMs = frameTimeMs;
		hud->elapsedSecondsSincePeak = 0.0f;
	}
	else
	{
		hud->elapsedSecondsSincePeak += GetFrameTime();
	}

	hud->residentMemoryBytes = PerformanceHud_ReadResidentMemoryBytes();
}

void PerformanceHud_Draw(const PerformanceHud *hud, const Canvas *canvas)
{
	int x = GetScreenWidth() - kHudWidth - kHudPadding;
	int y = kHudPadding;
	int height = 10 * kHudLineHeight + (kHudPadding * 2);

	DrawRectangle(x, y, kHudWidth, height, kHudBackgroundColor);

	DrawText("Performance", x + kHudPadding, y + kHudPadding, kHudFontSize, kHudValueColor);
	DrawText(TextFormat("FPS: %d", GetFPS()), x + kHudPadding, y + kHudPadding + (1 * kHudLineHeight), kHudFontSize, kHudValueColor);
	DrawText(TextFormat("Frame: %.2f ms", GetFrameTime() * kMsPerSecond), x + kHudPadding, y + kHudPadding + (2 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Avg frame: %.2f ms", hud->averageFrameTimeMs), x + kHudPadding, y + kHudPadding + (3 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Peak frame: %.2f ms", hud->peakFrameTimeMs), x + kHudPadding, y + kHudPadding + (4 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("RSS: %.2f MB", (double)hud->residentMemoryBytes / (double)kBytesPerMegabyte), x + kHudPadding, y + kHudPadding + (5 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Resolution: %d x %d", GetScreenWidth(), GetScreenHeight()), x + kHudPadding, y + kHudPadding + (6 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Canvas zoom: %.2fx", canvas->camera.zoom), x + kHudPadding, y + kHudPadding + (7 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Panes: %d", PaneManager_GetPaneCount(&canvas->paneManager)), x + kHudPadding, y + kHudPadding + (8 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText("Present: VSync", x + kHudPadding, y + kHudPadding + (9 * kHudLineHeight), kHudFontSize, kHudLabelColor);
}
