#ifndef GHOSTTY_ADAPTER_H
#define GHOSTTY_ADAPTER_H

#include <stdbool.h>

#include <ghostty/vt.h>

#include "terminal_adapter.h"

typedef struct GhosttyEffectsContext {
	int ptyFd;
	float cellWidth;
	float cellHeight;
	uint16_t columns;
	uint16_t rows;
} GhosttyEffectsContext;

typedef struct GhosttyAdapterState {
	bool available;
	GhosttyTerminal terminal;
	GhosttyKeyEncoder keyEncoder;
	GhosttyKeyEvent keyEvent;
	GhosttyRenderState renderState;
	GhosttyRenderStateRowIterator rowIterator;
	GhosttyRenderStateRowCells rowCells;
	GhosttyEffectsContext effectsContext;
} GhosttyAdapterState;

extern const TerminalAdapterVTable kGhosttyAdapterVTable;

#endif
