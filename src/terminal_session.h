#ifndef TERMINAL_SESSION_H
#define TERMINAL_SESSION_H

#include <stdbool.h>

#include "ghostty_adapter.h"
#include "pty_backend.h"
#include "terminal_adapter.h"
#include "text_terminal_adapter.h"

typedef union TerminalAdapterState {
	GhosttyAdapterState ghostty;
	TextTerminalAdapterState text;
} TerminalAdapterState;

typedef struct TerminalSession {
	PtyBackend backend;
	const TerminalAdapterVTable *adapter;
	TerminalAdapterState adapterState;
	TerminalSurface surface;
	int columns;
	int rows;
	bool isActive;
	bool hasExited;
} TerminalSession;

bool TerminalSession_Init(TerminalSession *session, int columns, int rows);
void TerminalSession_Shutdown(TerminalSession *session);
void TerminalSession_Update(TerminalSession *session);
void TerminalSession_Resize(TerminalSession *session, int columns, int rows, float cellWidth, float cellHeight);
void TerminalSession_SendText(TerminalSession *session, const char *text);
void TerminalSession_SendKey(TerminalSession *session, int key);
void TerminalSession_HandleInput(TerminalSession *session);
void TerminalSession_Draw(const TerminalSession *session, const TerminalDrawParams *params);
const char *TerminalSession_GetLine(const TerminalSession *session, int index);
int TerminalSession_GetLineCount(const TerminalSession *session);
bool TerminalSession_IsActive(const TerminalSession *session);
bool TerminalSession_HasExited(const TerminalSession *session);
const char *TerminalSession_GetBackendName(const TerminalSession *session);
bool TerminalSession_UsingRealTerminalCore(const TerminalSession *session);

#endif
