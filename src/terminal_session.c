#include "terminal_session.h"

#include <string.h>

#include "raylib.h"

bool TerminalSession_Init(TerminalSession *session, int columns, int rows)
{
	memset(session, 0, sizeof(*session));
	session->columns = columns;
	session->rows = rows;

	if (!PtyBackend_Spawn(&session->backend, NULL, columns, rows))
	{
		session->hasExited = true;
		return false;
	}

	session->adapter = &kGhosttyAdapterVTable;
	if (!session->adapter->init(&session->adapterState, &session->surface, session->backend.masterFd, columns, rows, 9.0f, 18.0f))
	{
		session->adapter = &kTextTerminalAdapterVTable;
		session->adapter->init(&session->adapterState, &session->surface, session->backend.masterFd, columns, rows, 9.0f, 18.0f);
	}

	session->isActive = true;
	return true;
}

void TerminalSession_Shutdown(TerminalSession *session)
{
	PtyBackend_Close(&session->backend);
	session->adapter->shutdown(&session->adapterState);
	session->isActive = false;
}

void TerminalSession_Update(TerminalSession *session)
{
	if (!session->isActive)
	{
		return;
	}

	char buffer[1024];
	for (;;)
	{
		ssize_t bytesRead = PtyBackend_Read(&session->backend, buffer, sizeof(buffer));
		if (bytesRead <= 0)
		{
			break;
		}

		session->adapter->feed_output(&session->adapterState, &session->surface, buffer, (size_t)bytesRead);
	}

	if (!PtyBackend_IsChildRunning(&session->backend))
	{
		session->hasExited = true;
		session->isActive = false;
	}
}

void TerminalSession_Resize(TerminalSession *session, int columns, int rows, float cellWidth, float cellHeight)
{
	if (columns == session->columns && rows == session->rows)
	{
		return;
	}

	session->columns = columns;
	session->rows = rows;
	PtyBackend_Resize(&session->backend, columns, rows);
	session->adapter->resize(&session->adapterState, &session->surface, columns, rows, cellWidth, cellHeight);
}

void TerminalSession_SendText(TerminalSession *session, const char *text)
{
	if (!session->isActive || text == NULL || text[0] == '\0')
	{
		return;
	}

	PtyBackend_Write(&session->backend, text, strlen(text));
}

void TerminalSession_SendKey(TerminalSession *session, int key)
{
	if (!session->isActive)
	{
		return;
	}

	switch (key)
	{
		case KEY_ENTER:
		case KEY_KP_ENTER:
			TerminalSession_SendText(session, "\n");
			break;
		case KEY_BACKSPACE:
			TerminalSession_SendText(session, "\x7f");
			break;
		case KEY_TAB:
			TerminalSession_SendText(session, "\t");
			break;
		case KEY_ESCAPE:
			TerminalSession_SendText(session, "\x1b");
			break;
		default:
			break;
	}
}

void TerminalSession_HandleInput(TerminalSession *session)
{
	if (!session->isActive)
	{
		return;
	}

	session->adapter->handle_input(&session->adapterState, session->backend.masterFd);
}

void TerminalSession_Draw(const TerminalSession *session, const TerminalDrawParams *params)
{
	session->adapter->draw((void *)&session->adapterState, &session->surface, params);
}

const char *TerminalSession_GetLine(const TerminalSession *session, int index)
{
	if (index < 0 || index >= session->surface.lineCount)
	{
		return "";
	}

	return session->surface.lines[index];
}

int TerminalSession_GetLineCount(const TerminalSession *session)
{
	return session->surface.lineCount;
}

bool TerminalSession_IsActive(const TerminalSession *session)
{
	return session->isActive;
}

bool TerminalSession_HasExited(const TerminalSession *session)
{
	return session->hasExited;
}

const char *TerminalSession_GetBackendName(const TerminalSession *session)
{
	return session->adapter->backend_name();
}

bool TerminalSession_UsingRealTerminalCore(const TerminalSession *session)
{
	return session->adapter->is_real_terminal_core();
}
