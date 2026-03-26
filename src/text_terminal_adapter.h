#ifndef TEXT_TERMINAL_ADAPTER_H
#define TEXT_TERMINAL_ADAPTER_H

#include "terminal_adapter.h"

typedef struct TextTerminalAdapterState {
	int currentLineIndex;
	int columns;
	int rows;
} TextTerminalAdapterState;

extern const TerminalAdapterVTable kTextTerminalAdapterVTable;

#endif
