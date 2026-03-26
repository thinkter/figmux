#include "text_terminal_adapter.h"

#include <ctype.h>
#include <string.h>

#include "pty_backend.h"

static void TextTerminalAdapter_StartNewLine(TextTerminalAdapterState *state, TerminalSurface *surface)
{
	if (surface->lineCount < TERMINAL_MAX_LINES)
	{
		state->currentLineIndex = surface->lineCount;
		surface->lineCount++;
		surface->lines[state->currentLineIndex][0] = '\0';
		return;
	}

	for (int i = 1; i < TERMINAL_MAX_LINES; i++)
	{
		memcpy(surface->lines[i - 1], surface->lines[i], TERMINAL_MAX_LINE_LENGTH);
	}

	state->currentLineIndex = TERMINAL_MAX_LINES - 1;
	surface->lines[state->currentLineIndex][0] = '\0';
}

static void TextTerminalAdapter_AppendChar(TextTerminalAdapterState *state, TerminalSurface *surface, char value)
{
	size_t lineLength = strlen(surface->lines[state->currentLineIndex]);
	if (lineLength >= TERMINAL_MAX_LINE_LENGTH - 1)
	{
		TextTerminalAdapter_StartNewLine(state, surface);
		lineLength = 0;
	}

	surface->lines[state->currentLineIndex][lineLength] = value;
	surface->lines[state->currentLineIndex][lineLength + 1] = '\0';
}

static void TextTerminalAdapter_Backspace(TextTerminalAdapterState *state, TerminalSurface *surface)
{
	size_t lineLength = strlen(surface->lines[state->currentLineIndex]);
	(void)state;
	if (lineLength == 0)
	{
		return;
	}

	surface->lines[state->currentLineIndex][lineLength - 1] = '\0';
}

static bool TextTerminalAdapter_Init(void *state, TerminalSurface *surface, int ptyFd, int columns, int rows, float cellWidth, float cellHeight)
{
	TextTerminalAdapterState *textState = (TextTerminalAdapterState *)state;
	(void)ptyFd;
	(void)cellWidth;
	(void)cellHeight;
	memset(surface, 0, sizeof(*surface));
	surface->lineCount = 1;
	textState->currentLineIndex = 0;
	textState->columns = columns;
	textState->rows = rows;
	return true;
}

static void TextTerminalAdapter_Shutdown(void *state)
{
	(void)state;
}

static void TextTerminalAdapter_FeedOutput(void *state, TerminalSurface *surface, const char *buffer, size_t length)
{
	TextTerminalAdapterState *textState = (TextTerminalAdapterState *)state;
	bool inEscape = false;

	for (size_t i = 0; i < length; i++)
	{
		unsigned char byte = (unsigned char)buffer[i];

		if (inEscape)
		{
			if (byte >= '@' && byte <= '~')
			{
				inEscape = false;
			}
			continue;
		}

		if (byte == 0x1b)
		{
			inEscape = true;
			continue;
		}

		if (byte == '\r')
		{
			surface->lines[textState->currentLineIndex][0] = '\0';
			continue;
		}

		if (byte == '\n')
		{
			TextTerminalAdapter_StartNewLine(textState, surface);
			continue;
		}

		if (byte == '\b' || byte == 0x7f)
		{
			TextTerminalAdapter_Backspace(textState, surface);
			continue;
		}

		if (byte == '\t')
		{
			TextTerminalAdapter_AppendChar(textState, surface, ' ');
			TextTerminalAdapter_AppendChar(textState, surface, ' ');
			continue;
		}

		if (isprint(byte))
		{
			TextTerminalAdapter_AppendChar(textState, surface, (char)byte);
		}
	}
}

static void TextTerminalAdapter_Resize(void *state, TerminalSurface *surface, int columns, int rows, float cellWidth, float cellHeight)
{
	TextTerminalAdapterState *textState = (TextTerminalAdapterState *)state;
	(void)surface;
	(void)cellWidth;
	(void)cellHeight;
	textState->columns = columns;
	textState->rows = rows;
}

static void TextTerminalAdapter_HandleInput(void *state, int ptyFd)
{
	(void)state;

	if (ptyFd < 0)
	{
		return;
	}

	PtyBackend backend = {
		.masterFd = ptyFd,
		.childPid = -1,
		.isOpen = true
	};

	int codepoint = GetCharPressed();
	while (codepoint != 0)
	{
		int utf8Length = 0;
		const char *utf8 = CodepointToUTF8(codepoint, &utf8Length);
		if (utf8Length > 0)
		{
			PtyBackend_Write(&backend, utf8, (size_t)utf8Length);
		}
		codepoint = GetCharPressed();
	}

	struct {
		int key;
		const char *sequence;
	} specialKeys[] = {
		{ KEY_ENTER, "\n" },
		{ KEY_KP_ENTER, "\n" },
		{ KEY_BACKSPACE, "\x7f" },
		{ KEY_TAB, "\t" },
		{ KEY_ESCAPE, "\x1b" },
		{ KEY_UP, "\x1b[A" },
		{ KEY_DOWN, "\x1b[B" },
		{ KEY_RIGHT, "\x1b[C" },
		{ KEY_LEFT, "\x1b[D" },
		{ KEY_HOME, "\x1b[H" },
		{ KEY_END, "\x1b[F" },
		{ KEY_DELETE, "\x1b[3~" },
		{ KEY_INSERT, "\x1b[2~" },
		{ KEY_PAGE_UP, "\x1b[5~" },
		{ KEY_PAGE_DOWN, "\x1b[6~" },
	};

	for (int i = 0; i < (int)(sizeof(specialKeys) / sizeof(specialKeys[0])); i++)
	{
		if (IsKeyPressed(specialKeys[i].key) || IsKeyPressedRepeat(specialKeys[i].key))
		{
			PtyBackend_Write(&backend, specialKeys[i].sequence, strlen(specialKeys[i].sequence));
		}
	}
}

static void TextTerminalAdapter_Draw(void *state, const TerminalSurface *surface, const TerminalDrawParams *params)
{
	(void)state;

	int visibleLineCount = (int)(params->bounds.height / params->cellHeight);
	if (visibleLineCount < 1)
	{
		visibleLineCount = 1;
	}

	int startLine = surface->lineCount - visibleLineCount;
	if (startLine < 0)
	{
		startLine = 0;
	}

	Vector2 contentTopLeftScreen = GetWorldToScreen2D((Vector2){ params->bounds.x, params->bounds.y }, params->camera);
	Vector2 contentBottomRightScreen = GetWorldToScreen2D((Vector2){ params->bounds.x + params->bounds.width, params->bounds.y + params->bounds.height }, params->camera);
	int scissorX = (int)contentTopLeftScreen.x;
	int scissorY = (int)contentTopLeftScreen.y;
	int scissorWidth = (int)(contentBottomRightScreen.x - contentTopLeftScreen.x);
	int scissorHeight = (int)(contentBottomRightScreen.y - contentTopLeftScreen.y);
	if (scissorWidth < 1) scissorWidth = 1;
	if (scissorHeight < 1) scissorHeight = 1;

	BeginScissorMode(scissorX, scissorY, scissorWidth, scissorHeight);
	for (int lineIndex = startLine; lineIndex < surface->lineCount; lineIndex++)
	{
		int visibleIndex = lineIndex - startLine;
		DrawText(
			surface->lines[lineIndex],
			(int)params->bounds.x,
			(int)(params->bounds.y + (visibleIndex * params->cellHeight)),
			params->fontSize,
			params->textColor
		);
	}

	DrawText(
		"[fallback parser: escape handling is incomplete]",
		(int)params->bounds.x,
		(int)(params->bounds.y + params->bounds.height - params->cellHeight),
		params->fontSize,
		params->accentColor
	);
	EndScissorMode();
}

static const char *TextTerminalAdapter_BackendName(void)
{
	return "text-fallback";
}

static bool TextTerminalAdapter_IsRealTerminalCore(void)
{
	return false;
}

const TerminalAdapterVTable kTextTerminalAdapterVTable = {
	.init = TextTerminalAdapter_Init,
	.shutdown = TextTerminalAdapter_Shutdown,
	.feed_output = TextTerminalAdapter_FeedOutput,
	.resize = TextTerminalAdapter_Resize,
	.handle_input = TextTerminalAdapter_HandleInput,
	.draw = TextTerminalAdapter_Draw,
	.backend_name = TextTerminalAdapter_BackendName,
	.is_real_terminal_core = TextTerminalAdapter_IsRealTerminalCore
};
