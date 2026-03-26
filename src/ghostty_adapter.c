#include "ghostty_adapter.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pty_backend.h"

static Font gTerminalFont = {0};
static bool gTerminalFontLoaded = false;
static int gTerminalFontRefCount = 0;

typedef struct CodepointRange {
	int start;
	int end;
} CodepointRange;

static int *GhosttyAdapter_BuildFontCodepoints(int *count)
{
	static const CodepointRange ranges[] = {
		{ 0x0020, 0x007E }, /* Basic Latin */
		{ 0x00A0, 0x00FF }, /* Latin-1 Supplement */
		{ 0x0100, 0x024F }, /* Latin Extended */
		{ 0x2000, 0x206F }, /* General Punctuation */
		{ 0x20A0, 0x20CF }, /* Currency Symbols */
		{ 0x2190, 0x21FF }, /* Arrows */
		{ 0x2200, 0x22FF }, /* Math Operators */
		{ 0x2300, 0x23FF }, /* Misc Technical */
		{ 0x2500, 0x257F }, /* Box Drawing */
		{ 0x2580, 0x259F }, /* Block Elements */
		{ 0x25A0, 0x25FF }, /* Geometric Shapes */
		{ 0x2600, 0x26FF }, /* Misc Symbols */
		{ 0x2700, 0x27BF }, /* Dingbats */
		{ 0x2800, 0x28FF }, /* Braille Patterns */
		{ 0xE000, 0xF8FF }, /* BMP Private Use Area */
	};

	int total = 0;
	for (int i = 0; i < (int)(sizeof(ranges) / sizeof(ranges[0])); i++)
	{
		total += (ranges[i].end - ranges[i].start + 1);
	}

	int *codepoints = (int *)MemAlloc((unsigned int)(sizeof(int) * total));
	if (codepoints == NULL)
	{
		*count = 0;
		return NULL;
	}

	int index = 0;
	for (int i = 0; i < (int)(sizeof(ranges) / sizeof(ranges[0])); i++)
	{
		for (int cp = ranges[i].start; cp <= ranges[i].end; cp++)
		{
			codepoints[index++] = cp;
		}
	}

	*count = index;
	return codepoints;
}

static bool GhosttyAdapter_EnsureTerminalFontLoaded(void)
{
	if (gTerminalFontLoaded)
	{
		gTerminalFontRefCount++;
		return true;
	}

	int codepointCount = 0;
	int *codepoints = GhosttyAdapter_BuildFontCodepoints(&codepointCount);
	if (codepoints == NULL)
	{
		return false;
	}

	gTerminalFont = LoadFontEx("JetBrainsMonoNerdFontMono-Regular.ttf", 40, codepoints, codepointCount);
	MemFree(codepoints);

	if (gTerminalFont.texture.id == 0)
	{
		return false;
	}

	SetTextureFilter(gTerminalFont.texture, TEXTURE_FILTER_BILINEAR);
	gTerminalFontLoaded = true;
	gTerminalFontRefCount = 1;
	return true;
}

static void GhosttyAdapter_ReleaseTerminalFont(void)
{
	if (!gTerminalFontLoaded)
	{
		return;
	}

	gTerminalFontRefCount--;
	if (gTerminalFontRefCount > 0)
	{
		return;
	}

	UnloadFont(gTerminalFont);
	gTerminalFont = (Font){0};
	gTerminalFontLoaded = false;
	gTerminalFontRefCount = 0;
}

static void GhosttyAdapter_WritePty(GhosttyTerminal terminal, void *userdata, const uint8_t *data, size_t len)
{
	(void)terminal;
	GhosttyEffectsContext *context = (GhosttyEffectsContext *)userdata;
	if (context == NULL || context->ptyFd < 0)
	{
		return;
	}

	PtyBackend backend = {
		.masterFd = context->ptyFd,
		.childPid = -1,
		.isOpen = true
	};
	PtyBackend_Write(&backend, (const char *)data, len);
}

static bool GhosttyAdapter_Size(GhosttyTerminal terminal, void *userdata, GhosttySizeReportSize *outSize)
{
	(void)terminal;
	GhosttyEffectsContext *context = (GhosttyEffectsContext *)userdata;
	outSize->rows = context->rows;
	outSize->columns = context->columns;
	outSize->cell_width = (uint32_t)context->cellWidth;
	outSize->cell_height = (uint32_t)context->cellHeight;
	return true;
}

static bool GhosttyAdapter_DeviceAttributes(GhosttyTerminal terminal, void *userdata, GhosttyDeviceAttributes *outAttributes)
{
	(void)terminal;
	(void)userdata;

	outAttributes->primary.conformance_level = GHOSTTY_DA_CONFORMANCE_VT220;
	outAttributes->primary.features[0] = GHOSTTY_DA_FEATURE_COLUMNS_132;
	outAttributes->primary.features[1] = GHOSTTY_DA_FEATURE_SELECTIVE_ERASE;
	outAttributes->primary.features[2] = GHOSTTY_DA_FEATURE_ANSI_COLOR;
	outAttributes->primary.num_features = 3;
	outAttributes->secondary.device_type = GHOSTTY_DA_DEVICE_TYPE_VT220;
	outAttributes->secondary.firmware_version = 1;
	outAttributes->secondary.rom_cartridge = 0;
	outAttributes->tertiary.unit_id = 0;
	return true;
}

static GhosttyString GhosttyAdapter_XtVersion(GhosttyTerminal terminal, void *userdata)
{
	(void)terminal;
	(void)userdata;
	return (GhosttyString){ .ptr = (const uint8_t *)"figmux", .len = 6 };
}

static void GhosttyAdapter_TitleChanged(GhosttyTerminal terminal, void *userdata)
{
	(void)terminal;
	(void)userdata;
}

static bool GhosttyAdapter_ColorScheme(GhosttyTerminal terminal, void *userdata, GhosttyColorScheme *outScheme)
{
	(void)terminal;
	(void)userdata;
	(void)outScheme;
	return false;
}

static GhosttyKey GhosttyAdapter_MapKey(int raylibKey)
{
	if (raylibKey >= KEY_A && raylibKey <= KEY_Z) return GHOSTTY_KEY_A + (raylibKey - KEY_A);
	if (raylibKey >= KEY_ZERO && raylibKey <= KEY_NINE) return GHOSTTY_KEY_DIGIT_0 + (raylibKey - KEY_ZERO);
	if (raylibKey >= KEY_F1 && raylibKey <= KEY_F12) return GHOSTTY_KEY_F1 + (raylibKey - KEY_F1);

	switch (raylibKey)
	{
		case KEY_SPACE: return GHOSTTY_KEY_SPACE;
		case KEY_ENTER: return GHOSTTY_KEY_ENTER;
		case KEY_KP_ENTER: return GHOSTTY_KEY_NUMPAD_ENTER;
		case KEY_TAB: return GHOSTTY_KEY_TAB;
		case KEY_BACKSPACE: return GHOSTTY_KEY_BACKSPACE;
		case KEY_DELETE: return GHOSTTY_KEY_DELETE;
		case KEY_ESCAPE: return GHOSTTY_KEY_ESCAPE;
		case KEY_UP: return GHOSTTY_KEY_ARROW_UP;
		case KEY_DOWN: return GHOSTTY_KEY_ARROW_DOWN;
		case KEY_LEFT: return GHOSTTY_KEY_ARROW_LEFT;
		case KEY_RIGHT: return GHOSTTY_KEY_ARROW_RIGHT;
		case KEY_HOME: return GHOSTTY_KEY_HOME;
		case KEY_END: return GHOSTTY_KEY_END;
		case KEY_PAGE_UP: return GHOSTTY_KEY_PAGE_UP;
		case KEY_PAGE_DOWN: return GHOSTTY_KEY_PAGE_DOWN;
		case KEY_INSERT: return GHOSTTY_KEY_INSERT;
		case KEY_MINUS: return GHOSTTY_KEY_MINUS;
		case KEY_EQUAL: return GHOSTTY_KEY_EQUAL;
		case KEY_LEFT_BRACKET: return GHOSTTY_KEY_BRACKET_LEFT;
		case KEY_RIGHT_BRACKET: return GHOSTTY_KEY_BRACKET_RIGHT;
		case KEY_BACKSLASH: return GHOSTTY_KEY_BACKSLASH;
		case KEY_SEMICOLON: return GHOSTTY_KEY_SEMICOLON;
		case KEY_APOSTROPHE: return GHOSTTY_KEY_QUOTE;
		case KEY_COMMA: return GHOSTTY_KEY_COMMA;
		case KEY_PERIOD: return GHOSTTY_KEY_PERIOD;
		case KEY_SLASH: return GHOSTTY_KEY_SLASH;
		case KEY_GRAVE: return GHOSTTY_KEY_BACKQUOTE;
		default: return GHOSTTY_KEY_UNIDENTIFIED;
	}
}

static GhosttyMods GhosttyAdapter_GetMods(void)
{
	GhosttyMods mods = 0;
	if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) mods |= GHOSTTY_MODS_SHIFT;
	if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) mods |= GHOSTTY_MODS_CTRL;
	if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) mods |= GHOSTTY_MODS_ALT;
	if (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) mods |= GHOSTTY_MODS_SUPER;
	return mods;
}

static uint32_t GhosttyAdapter_UnshiftedCodepoint(int raylibKey)
{
	if (raylibKey >= KEY_A && raylibKey <= KEY_Z) return 'a' + (uint32_t)(raylibKey - KEY_A);
	if (raylibKey >= KEY_ZERO && raylibKey <= KEY_NINE) return '0' + (uint32_t)(raylibKey - KEY_ZERO);

	switch (raylibKey)
	{
		case KEY_SPACE: return ' ';
		case KEY_MINUS: return '-';
		case KEY_EQUAL: return '=';
		case KEY_LEFT_BRACKET: return '[';
		case KEY_RIGHT_BRACKET: return ']';
		case KEY_BACKSLASH: return '\\';
		case KEY_SEMICOLON: return ';';
		case KEY_APOSTROPHE: return '\'';
		case KEY_COMMA: return ',';
		case KEY_PERIOD: return '.';
		case KEY_SLASH: return '/';
		case KEY_GRAVE: return '`';
		default: return 0;
	}
}

static bool GhosttyAdapter_Init(void *state, TerminalSurface *surface, int ptyFd, int columns, int rows, float cellWidth, float cellHeight)
{
	GhosttyAdapterState *ghostty = (GhosttyAdapterState *)state;
	(void)surface;
	memset(ghostty, 0, sizeof(*ghostty));

	GhosttyTerminalOptions options = {
		.cols = (uint16_t)columns,
		.rows = (uint16_t)rows,
		.max_scrollback = 1000
	};

	if (ghostty_terminal_new(NULL, &ghostty->terminal, options) != GHOSTTY_SUCCESS) return false;
	if (ghostty_key_encoder_new(NULL, &ghostty->keyEncoder) != GHOSTTY_SUCCESS) return false;
	if (ghostty_key_event_new(NULL, &ghostty->keyEvent) != GHOSTTY_SUCCESS) return false;
	if (ghostty_render_state_new(NULL, &ghostty->renderState) != GHOSTTY_SUCCESS) return false;
	if (ghostty_render_state_row_iterator_new(NULL, &ghostty->rowIterator) != GHOSTTY_SUCCESS) return false;
	if (ghostty_render_state_row_cells_new(NULL, &ghostty->rowCells) != GHOSTTY_SUCCESS) return false;
	if (!GhosttyAdapter_EnsureTerminalFontLoaded()) return false;

	ghostty->effectsContext = (GhosttyEffectsContext){
		.ptyFd = ptyFd,
		.cellWidth = cellWidth,
		.cellHeight = cellHeight,
		.columns = (uint16_t)columns,
		.rows = (uint16_t)rows
	};

	ghostty_terminal_set(ghostty->terminal, GHOSTTY_TERMINAL_OPT_USERDATA, &ghostty->effectsContext);
	ghostty_terminal_set(ghostty->terminal, GHOSTTY_TERMINAL_OPT_WRITE_PTY, (const void *)GhosttyAdapter_WritePty);
	ghostty_terminal_set(ghostty->terminal, GHOSTTY_TERMINAL_OPT_SIZE, (const void *)GhosttyAdapter_Size);
	ghostty_terminal_set(ghostty->terminal, GHOSTTY_TERMINAL_OPT_DEVICE_ATTRIBUTES, (const void *)GhosttyAdapter_DeviceAttributes);
	ghostty_terminal_set(ghostty->terminal, GHOSTTY_TERMINAL_OPT_XTVERSION, (const void *)GhosttyAdapter_XtVersion);
	ghostty_terminal_set(ghostty->terminal, GHOSTTY_TERMINAL_OPT_TITLE_CHANGED, (const void *)GhosttyAdapter_TitleChanged);
	ghostty_terminal_set(ghostty->terminal, GHOSTTY_TERMINAL_OPT_COLOR_SCHEME, (const void *)GhosttyAdapter_ColorScheme);
	ghostty_render_state_update(ghostty->renderState, ghostty->terminal);

	ghostty->available = true;
	return true;
}

static void GhosttyAdapter_Shutdown(void *state)
{
	GhosttyAdapterState *ghostty = (GhosttyAdapterState *)state;
	if (ghostty->rowCells) ghostty_render_state_row_cells_free(ghostty->rowCells);
	if (ghostty->rowIterator) ghostty_render_state_row_iterator_free(ghostty->rowIterator);
	if (ghostty->renderState) ghostty_render_state_free(ghostty->renderState);
	if (ghostty->keyEvent) ghostty_key_event_free(ghostty->keyEvent);
	if (ghostty->keyEncoder) ghostty_key_encoder_free(ghostty->keyEncoder);
	if (ghostty->terminal) ghostty_terminal_free(ghostty->terminal);
	GhosttyAdapter_ReleaseTerminalFont();
	memset(ghostty, 0, sizeof(*ghostty));
}

static void GhosttyAdapter_FeedOutput(void *state, TerminalSurface *surface, const char *buffer, size_t length)
{
	GhosttyAdapterState *ghostty = (GhosttyAdapterState *)state;
	(void)surface;
	ghostty_terminal_vt_write(ghostty->terminal, (const uint8_t *)buffer, length);
	ghostty_render_state_update(ghostty->renderState, ghostty->terminal);
}

static void GhosttyAdapter_Resize(void *state, TerminalSurface *surface, int columns, int rows, float cellWidth, float cellHeight)
{
	GhosttyAdapterState *ghostty = (GhosttyAdapterState *)state;
	(void)surface;
	ghostty->effectsContext.columns = (uint16_t)columns;
	ghostty->effectsContext.rows = (uint16_t)rows;
	ghostty->effectsContext.cellWidth = cellWidth;
	ghostty->effectsContext.cellHeight = cellHeight;
	ghostty_terminal_resize(ghostty->terminal, (uint16_t)columns, (uint16_t)rows, (uint32_t)cellWidth, (uint32_t)cellHeight);
	ghostty_render_state_update(ghostty->renderState, ghostty->terminal);
}

static void GhosttyAdapter_HandleInput(void *state, int ptyFd)
{
	GhosttyAdapterState *ghostty = (GhosttyAdapterState *)state;
	ghostty_key_encoder_setopt_from_terminal(ghostty->keyEncoder, ghostty->terminal);

	char utf8[64];
	size_t utf8Length = 0;
	int codepoint = GetCharPressed();
	while (codepoint != 0)
	{
		int partLength = 0;
		const char *part = CodepointToUTF8(codepoint, &partLength);
		if (utf8Length + (size_t)partLength < sizeof(utf8))
		{
			memcpy(&utf8[utf8Length], part, (size_t)partLength);
			utf8Length += (size_t)partLength;
		}
		codepoint = GetCharPressed();
	}

	static const int keys[] = {
		KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
		KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
		KEY_ZERO, KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE,
		KEY_SPACE, KEY_ENTER, KEY_KP_ENTER, KEY_TAB, KEY_BACKSPACE, KEY_DELETE, KEY_ESCAPE,
		KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_HOME, KEY_END, KEY_PAGE_UP, KEY_PAGE_DOWN, KEY_INSERT,
		KEY_MINUS, KEY_EQUAL, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_BACKSLASH, KEY_SEMICOLON, KEY_APOSTROPHE,
		KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_GRAVE, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
		KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12
	};

	for (int i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); i++)
	{
		int raylibKey = keys[i];
		bool pressed = IsKeyPressed(raylibKey);
		bool repeated = IsKeyPressedRepeat(raylibKey);
		bool released = IsKeyReleased(raylibKey);
		if (!pressed && !repeated && !released)
		{
			continue;
		}

		GhosttyKey key = GhosttyAdapter_MapKey(raylibKey);
		if (key == GHOSTTY_KEY_UNIDENTIFIED)
		{
			continue;
		}

		GhosttyKeyAction action = released ? GHOSTTY_KEY_ACTION_RELEASE : (pressed ? GHOSTTY_KEY_ACTION_PRESS : GHOSTTY_KEY_ACTION_REPEAT);
		GhosttyMods mods = GhosttyAdapter_GetMods();
		GhosttyMods consumed = 0;
		uint32_t unshiftedCodepoint = GhosttyAdapter_UnshiftedCodepoint(raylibKey);
		if (unshiftedCodepoint != 0 && (mods & GHOSTTY_MODS_SHIFT))
		{
			consumed |= GHOSTTY_MODS_SHIFT;
		}

		ghostty_key_event_set_key(ghostty->keyEvent, key);
		ghostty_key_event_set_action(ghostty->keyEvent, action);
		ghostty_key_event_set_mods(ghostty->keyEvent, mods);
		ghostty_key_event_set_unshifted_codepoint(ghostty->keyEvent, unshiftedCodepoint);
		ghostty_key_event_set_consumed_mods(ghostty->keyEvent, consumed);
		ghostty_key_event_set_utf8(ghostty->keyEvent, utf8Length > 0 && !released ? utf8 : NULL, utf8Length > 0 && !released ? utf8Length : 0);

		char buffer[128];
		size_t written = 0;
		if (ghostty_key_encoder_encode(ghostty->keyEncoder, ghostty->keyEvent, buffer, sizeof(buffer), &written) == GHOSTTY_SUCCESS && written > 0)
		{
			PtyBackend backend = { .masterFd = ptyFd, .childPid = -1, .isOpen = true };
			PtyBackend_Write(&backend, buffer, written);
			utf8Length = 0;
		}
	}

	if (utf8Length > 0)
	{
		PtyBackend backend = { .masterFd = ptyFd, .childPid = -1, .isOpen = true };
		PtyBackend_Write(&backend, utf8, utf8Length);
	}
}

static void GhosttyAdapter_Draw(void *state, const TerminalSurface *surface, const TerminalDrawParams *params)
{
	GhosttyAdapterState *ghostty = (GhosttyAdapterState *)state;
	(void)surface;

	GhosttyRenderStateColors colors = GHOSTTY_INIT_SIZED(GhosttyRenderStateColors);
	if (ghostty_render_state_colors_get(ghostty->renderState, &colors) != GHOSTTY_SUCCESS)
	{
		return;
	}

	if (ghostty_render_state_get(ghostty->renderState, GHOSTTY_RENDER_STATE_DATA_ROW_ITERATOR, &ghostty->rowIterator) != GHOSTTY_SUCCESS)
	{
		return;
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

	float y = params->bounds.y;
	Font font = gTerminalFontLoaded ? gTerminalFont : GetFontDefault();
	while (ghostty_render_state_row_iterator_next(ghostty->rowIterator))
	{
		if (ghostty_render_state_row_get(ghostty->rowIterator, GHOSTTY_RENDER_STATE_ROW_DATA_CELLS, &ghostty->rowCells) != GHOSTTY_SUCCESS)
		{
			y += params->cellHeight;
			continue;
		}

		float x = params->bounds.x;
		while (ghostty_render_state_row_cells_next(ghostty->rowCells))
		{
			uint32_t graphemeLength = 0;
			ghostty_render_state_row_cells_get(ghostty->rowCells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_GRAPHEMES_LEN, &graphemeLength);

			GhosttyColorRgb bgRgb = colors.background;
			bool hasBg = ghostty_render_state_row_cells_get(ghostty->rowCells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_BG_COLOR, &bgRgb) == GHOSTTY_SUCCESS;
			if (hasBg)
			{
				DrawRectangle((int)x, (int)y, (int)params->cellWidth + 1, (int)params->cellHeight + 1, (Color){ bgRgb.r, bgRgb.g, bgRgb.b, 255 });
			}

			if (graphemeLength > 0)
			{
				uint32_t codepoints[16] = {0};
				ghostty_render_state_row_cells_get(ghostty->rowCells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_GRAPHEMES_BUF, codepoints);

				char text[64];
				int textLength = 0;
				uint32_t maxGraphemes = graphemeLength < 16 ? graphemeLength : 16;
				for (uint32_t i = 0; i < maxGraphemes && textLength < 60; i++)
				{
					int utf8Size = 0;
					const char *utf8 = CodepointToUTF8((int)codepoints[i], &utf8Size);
					memcpy(&text[textLength], utf8, (size_t)utf8Size);
					textLength += utf8Size;
				}
				text[textLength] = '\0';

				GhosttyColorRgb fgRgb = colors.foreground;
				ghostty_render_state_row_cells_get(ghostty->rowCells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_FG_COLOR, &fgRgb);
				GhosttyStyle style = GHOSTTY_INIT_SIZED(GhosttyStyle);
				ghostty_render_state_row_cells_get(ghostty->rowCells, GHOSTTY_RENDER_STATE_ROW_CELLS_DATA_STYLE, &style);

				if (style.inverse)
				{
					GhosttyColorRgb swap = fgRgb;
					fgRgb = bgRgb;
					bgRgb = swap;
					DrawRectangle((int)x, (int)y, (int)params->cellWidth + 1, (int)params->cellHeight + 1, (Color){ bgRgb.r, bgRgb.g, bgRgb.b, 255 });
				}

				Color fg = { fgRgb.r, fgRgb.g, fgRgb.b, 255 };
				int italicOffset = style.italic ? (params->fontSize / 6) : 0;
				DrawTextEx(font, text, (Vector2){ x + italicOffset, y }, (float)params->fontSize, 0.0f, fg);
				if (style.bold)
				{
					DrawTextEx(font, text, (Vector2){ x + italicOffset + 1.0f, y }, (float)params->fontSize, 0.0f, fg);
				}
			}

			x += params->cellWidth;
		}

		bool clean = false;
		ghostty_render_state_row_set(ghostty->rowIterator, GHOSTTY_RENDER_STATE_ROW_OPTION_DIRTY, &clean);
		y += params->cellHeight;
	}

	bool cursorVisible = false;
	bool cursorInViewport = false;
	ghostty_render_state_get(ghostty->renderState, GHOSTTY_RENDER_STATE_DATA_CURSOR_VISIBLE, &cursorVisible);
	ghostty_render_state_get(ghostty->renderState, GHOSTTY_RENDER_STATE_DATA_CURSOR_VIEWPORT_HAS_VALUE, &cursorInViewport);
	if (cursorVisible && cursorInViewport)
	{
		uint16_t cursorX = 0;
		uint16_t cursorY = 0;
		ghostty_render_state_get(ghostty->renderState, GHOSTTY_RENDER_STATE_DATA_CURSOR_VIEWPORT_X, &cursorX);
		ghostty_render_state_get(ghostty->renderState, GHOSTTY_RENDER_STATE_DATA_CURSOR_VIEWPORT_Y, &cursorY);

		GhosttyColorRgb cursorRgb = colors.cursor_has_value ? colors.cursor : colors.foreground;
		DrawRectangle(
			(int)(params->bounds.x + (cursorX * params->cellWidth)),
			(int)(params->bounds.y + (cursorY * params->cellHeight)),
			(int)params->cellWidth,
			(int)params->cellHeight,
			(Color){ cursorRgb.r, cursorRgb.g, cursorRgb.b, 120 }
		);
	}

	GhosttyRenderStateDirty cleanState = GHOSTTY_RENDER_STATE_DIRTY_FALSE;
	ghostty_render_state_set(ghostty->renderState, GHOSTTY_RENDER_STATE_OPTION_DIRTY, &cleanState);
	EndScissorMode();
}

static const char *GhosttyAdapter_BackendName(void)
{
	return "ghostty-vt";
}

static bool GhosttyAdapter_IsRealTerminalCore(void)
{
	return true;
}

const TerminalAdapterVTable kGhosttyAdapterVTable = {
	.init = GhosttyAdapter_Init,
	.shutdown = GhosttyAdapter_Shutdown,
	.feed_output = GhosttyAdapter_FeedOutput,
	.resize = GhosttyAdapter_Resize,
	.handle_input = GhosttyAdapter_HandleInput,
	.draw = GhosttyAdapter_Draw,
	.backend_name = GhosttyAdapter_BackendName,
	.is_real_terminal_core = GhosttyAdapter_IsRealTerminalCore
};
