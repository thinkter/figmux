#ifndef TERMINAL_ADAPTER_H
#define TERMINAL_ADAPTER_H

#include <stdbool.h>
#include <stddef.h>

#include "raylib.h"

#define TERMINAL_MAX_LINES 256
#define TERMINAL_MAX_LINE_LENGTH 256

typedef struct TerminalSurface {
	char lines[TERMINAL_MAX_LINES][TERMINAL_MAX_LINE_LENGTH];
	int lineCount;
} TerminalSurface;

typedef struct TerminalDrawParams {
	Rectangle bounds;
	Camera2D camera;
	float cellWidth;
	float cellHeight;
	int fontSize;
	Color textColor;
	Color mutedTextColor;
	Color accentColor;
} TerminalDrawParams;

typedef struct TerminalAdapterVTable {
	bool (*init)(void *state, TerminalSurface *surface, int ptyFd, int columns, int rows, float cellWidth, float cellHeight);
	void (*shutdown)(void *state);
	void (*feed_output)(void *state, TerminalSurface *surface, const char *buffer, size_t length);
	void (*resize)(void *state, TerminalSurface *surface, int columns, int rows, float cellWidth, float cellHeight);
	void (*handle_input)(void *state, int ptyFd);
	void (*prepare_draw)(void *state, const TerminalSurface *surface, const TerminalDrawParams *params);
	void (*draw)(void *state, const TerminalSurface *surface, const TerminalDrawParams *params);
	const char *(*backend_name)(void);
	bool (*is_real_terminal_core)(void);
} TerminalAdapterVTable;

#endif
