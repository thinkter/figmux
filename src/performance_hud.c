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
static const float kMemorySampleIntervalSeconds = 0.25f;
static const float kMsPerSecond = 1000.0f;
static const size_t kBytesPerMegabyte = 1024 * 1024;

typedef struct MemorySnapshot {
	size_t residentBytes;
	size_t proportionalBytes;
	size_t privateDirtyBytes;
} MemorySnapshot;

static MemorySnapshot PerformanceHud_ReadMemorySnapshot(void)
{
	MemorySnapshot snapshot = {0};

	FILE *statusFile = fopen("/proc/self/status", "r");
	if (statusFile != NULL)
	{
		char line[256];
		size_t residentKilobytes = 0;
		while (fgets(line, sizeof(line), statusFile) != NULL)
		{
			if (sscanf(line, "VmRSS: %zu kB", &residentKilobytes) == 1)
			{
				snapshot.residentBytes = residentKilobytes * 1024;
				break;
			}
		}

		fclose(statusFile);
	}

	FILE *rollupFile = fopen("/proc/self/smaps_rollup", "r");
	if (rollupFile != NULL)
	{
		char line[256];
		size_t proportionalKilobytes = 0;
		size_t privateDirtyKilobytes = 0;
		while (fgets(line, sizeof(line), rollupFile) != NULL)
		{
			if (sscanf(line, "Pss: %zu kB", &proportionalKilobytes) == 1)
			{
				snapshot.proportionalBytes = proportionalKilobytes * 1024;
				continue;
			}

			if (sscanf(line, "Private_Dirty: %zu kB", &privateDirtyKilobytes) == 1)
			{
				snapshot.privateDirtyBytes = privateDirtyKilobytes * 1024;
			}
		}

		fclose(rollupFile);
	}

	return snapshot;
}

void PerformanceHud_Init(PerformanceHud *hud)
{
	hud->averageFrameTimeMs = 0.0f;
	hud->peakFrameTimeMs = 0.0f;
	hud->elapsedSecondsSincePeak = 0.0f;
	hud->elapsedSecondsSinceMemorySample = kMemorySampleIntervalSeconds;
	hud->residentMemoryBytes = 0;
	hud->proportionalMemoryBytes = 0;
	hud->privateDirtyMemoryBytes = 0;
}

void PerformanceHud_Update(PerformanceHud *hud, const Canvas *canvas)
{
	(void)canvas;

	float frameTime = GetFrameTime();
	float frameTimeMs = frameTime * kMsPerSecond;
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
		hud->elapsedSecondsSincePeak += frameTime;
	}

	hud->elapsedSecondsSinceMemorySample += frameTime;
	if (hud->elapsedSecondsSinceMemorySample >= kMemorySampleIntervalSeconds)
	{
		MemorySnapshot snapshot = PerformanceHud_ReadMemorySnapshot();
		hud->residentMemoryBytes = snapshot.residentBytes;
		hud->proportionalMemoryBytes = snapshot.proportionalBytes;
		hud->privateDirtyMemoryBytes = snapshot.privateDirtyBytes;
		hud->elapsedSecondsSinceMemorySample = 0.0f;
	}
}

void PerformanceHud_Draw(const PerformanceHud *hud, const Canvas *canvas)
{
	int x = GetScreenWidth() - kHudWidth - kHudPadding;
	int y = kHudPadding;
	int height = 12 * kHudLineHeight + (kHudPadding * 2);

	DrawRectangle(x, y, kHudWidth, height, kHudBackgroundColor);

	DrawText("Performance", x + kHudPadding, y + kHudPadding, kHudFontSize, kHudValueColor);
	DrawText(TextFormat("FPS: %d", GetFPS()), x + kHudPadding, y + kHudPadding + (1 * kHudLineHeight), kHudFontSize, kHudValueColor);
	DrawText(TextFormat("Frame: %.2f ms", GetFrameTime() * kMsPerSecond), x + kHudPadding, y + kHudPadding + (2 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Avg frame: %.2f ms", hud->averageFrameTimeMs), x + kHudPadding, y + kHudPadding + (3 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Peak frame: %.2f ms", hud->peakFrameTimeMs), x + kHudPadding, y + kHudPadding + (4 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("RSS: %.2f MB", (double)hud->residentMemoryBytes / (double)kBytesPerMegabyte), x + kHudPadding, y + kHudPadding + (5 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("PSS: %.2f MB", (double)hud->proportionalMemoryBytes / (double)kBytesPerMegabyte), x + kHudPadding, y + kHudPadding + (6 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Priv dirty: %.2f MB", (double)hud->privateDirtyMemoryBytes / (double)kBytesPerMegabyte), x + kHudPadding, y + kHudPadding + (7 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Resolution: %d x %d", GetScreenWidth(), GetScreenHeight()), x + kHudPadding, y + kHudPadding + (8 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Canvas zoom: %.2fx", canvas->camera.zoom), x + kHudPadding, y + kHudPadding + (9 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText(TextFormat("Panes: %d", PaneManager_GetPaneCount(&canvas->paneManager)), x + kHudPadding, y + kHudPadding + (10 * kHudLineHeight), kHudFontSize, kHudLabelColor);
	DrawText("Present: VSync", x + kHudPadding, y + kHudPadding + (11 * kHudLineHeight), kHudFontSize, kHudLabelColor);
}
