/*
File:   app_clay_widgets.c
Author: Taylor Robbins
Date:   03\27\2025
Description: 
	** Holds various functions that comprise common widgets for Clay
*/

#define CLAY_ICON(texturePntr, size, color) CLAY({      \
	.layout = {                                         \
		.sizing = {                                     \
			.width = CLAY_SIZING_FIXED((size).Width),   \
			.height = CLAY_SIZING_FIXED((size).Height), \
		},                                              \
	},                                                  \
	.image = {                                          \
		.imageData = (texturePntr),                     \
		.sourceDimensions = {                           \
			.width = (r32)((texturePntr)->Width),       \
			.height = (r32)((texturePntr)->Height),     \
		},                                              \
	},                                                  \
	.backgroundColor = ToClayColor(color),              \
}) {}

bool IsMouseOverClay(ClayId clayId)
{
	return appIn->mouse.isOverWindow && Clay_PointerOver(clayId);
}
bool IsMouseOverClayInContainer(ClayId containerId, ClayId clayId)
{
	return appIn->mouse.isOverWindow && Clay_PointerOver(containerId) && Clay_PointerOver(clayId);
}

// This should produce the same value as ClayUIRendererMeasureText in gfx_clay_renderer.h
v2 ClayUiTextSize(PigFont* font, r32 fontSize, u8 styleFlags, Str8 text)
{
	TextMeasure textMeasure = MeasureTextEx(font, fontSize, styleFlags, text);
	return NewV2(CeilR32(textMeasure.Width - textMeasure.OffsetX), CeilR32(textMeasure.Height));
}

//Call Clay__CloseElement once if false, three times if true (i.e. twice inside the if statement and once after)
bool ClayTopBtn(const char* btnText, bool showAltText, bool* isOpenPntr, bool* keepOpenUntilMouseoverPntr, bool isSubmenuOpen)
{
	ScratchBegin(scratch);
	ScratchBegin1(persistScratch, scratch);
	
	Str8 normalDisplayStr = StrLit(btnText);
	Assert(!IsEmptyStr(normalDisplayStr));
	Str8 altDisplayStr = PrintInArenaStr(persistScratch, "(%c)%.*s", normalDisplayStr.chars[0], normalDisplayStr.length-1, &normalDisplayStr.chars[1]);
	v2 normalDisplayStrSize = ClayUiTextSize(&app->uiFont, UI_FONT_SIZE, UI_FONT_STYLE, normalDisplayStr);
	v2 altDisplayStrSize = ClayUiTextSize(&app->uiFont, UI_FONT_SIZE, UI_FONT_STYLE, altDisplayStr);
	u16 leftPadding = (u16)(showAltText ? 0 : (altDisplayStrSize.Width - normalDisplayStrSize.Width)/2);
	
	Str8 btnIdStr = PrintInArenaStr(scratch, "%s_TopBtn", btnText);
	Str8 menuIdStr = PrintInArenaStr(scratch, "%s_TopBtnMenu", btnText);
	ClayId btnId = ToClayId(btnIdStr);
	ClayId menuId = ToClayId(menuIdStr);
	bool isBtnHovered = IsMouseOverClay(btnId);
	bool isHovered = (isBtnHovered || IsMouseOverClay(menuId));
	bool isBtnHoveredOrMenuOpen = (isBtnHovered || *isOpenPntr);
	Color32 backgroundColor = isBtnHoveredOrMenuOpen ? UiHoveredBlue : Transparent;
	Color32 borderColor = UiSelectedBlue;
	u16 borderWidth = isHovered ? 1 : 0;
	Clay__OpenElement();
	Clay__ConfigureOpenElement((Clay_ElementDeclaration){
		.id = btnId,
		.layout = { .padding = { 4, 4, 2, 2 } },
		.backgroundColor = ToClayColor(backgroundColor),
		.cornerRadius = CLAY_CORNER_RADIUS(4),
		.border = { .width=CLAY_BORDER_OUTSIDE(borderWidth), .color=ToClayColor(borderColor) },
	});
	CLAY({
		.layout = {
			.sizing = { .width=CLAY_SIZING_FIXED(altDisplayStrSize.Width), .height=CLAY_SIZING_FIT(0) },
			.padding = { .left = leftPadding },
		},
	})
	{
		CLAY_TEXT(
			ToClayString(showAltText ? altDisplayStr : normalDisplayStr),
			CLAY_TEXT_CONFIG({
				.fontId = app->clayUiFontId,
				.fontSize = (u16)UI_FONT_SIZE,
				.textColor = ToClayColor(UiTextWhite),
				.wrapMode = CLAY_TEXT_WRAP_NONE,
				.textAlignment = CLAY_TEXT_ALIGN_CENTER,
			})
		);
	}
	if (IsMouseOverClay(btnId) && IsMouseBtnPressed(&appIn->mouse, MouseBtn_Left)) { *isOpenPntr = !*isOpenPntr; }
	if (*isOpenPntr == true && isHovered && *keepOpenUntilMouseoverPntr) { *keepOpenUntilMouseoverPntr = false; } //once we are closed or the mouse is over, clear this flag, mouse leaving now will constitute closing
	if (*isOpenPntr == true && !isHovered && !*keepOpenUntilMouseoverPntr && !isSubmenuOpen) { *isOpenPntr = false; }
	if (*isOpenPntr)
	{
		r32 maxDropdownWidth = isSubmenuOpen ? appIn->screenSize.Width/2.0f : appIn->screenSize.Width;
		Clay__OpenElement();
		Clay__ConfigureOpenElement((Clay_ElementDeclaration){
			.id = menuId,
			.floating = {
				.attachTo = CLAY_ATTACH_TO_PARENT,
				.zIndex = 5,
				.attachPoints = {
					.parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
				},
			},
			.layout = {
				.padding = { 2, 2, 0, 0 },
				.sizing = { .width = CLAY_SIZING_FIT(0, maxDropdownWidth) },
			}
		});
		
		Clay__OpenElement();
		Clay__ConfigureOpenElement((Clay_ElementDeclaration){
			.layout = {
				.layoutDirection = CLAY_TOP_TO_BOTTOM,
				.padding = {
					.left = 1,
					.right = 1,
					.top = 2,
					.bottom = 2,
				},
				.childGap = 2,
			},
			.backgroundColor = ToClayColor(UiBackgroundGray),
			.border = { .color=ToClayColor(UiOutlineGray), .width={ .bottom=1 } },
			.cornerRadius = { 0, 0, 4, 4 },
		});
	}
	//NOTE: We do NOT do ScratchEnd on persistScratch, the string needs to live to the end of the frame where the UI will get rendered
	ScratchEnd(scratch);
	return *isOpenPntr;
}

//Call Clay__CloseElement once after if statement
bool ClayBtnStrEx(Str8 idStr, Str8 btnText, Str8 hotkeyStr, bool isEnabled, Texture* icon)
{
	ScratchBegin(scratch);
	Str8 fullIdStr = PrintInArenaStr(scratch, "%.*s_Btn", StrPrint(idStr));
	Str8 hotkeyIdStr = PrintInArenaStr(scratch, "%.*s_Hotkey", StrPrint(idStr));
	ClayId btnId = ToClayId(fullIdStr);
	ClayId hotkeyId = ToClayId(hotkeyIdStr);
	bool isHovered = IsMouseOverClay(btnId);
	bool isPressed = (isHovered && IsMouseBtnDown(&appIn->mouse, MouseBtn_Left));
	Color32 backgroundColor = !isEnabled ? UiBackgroundBlack : (isPressed ? UiSelectedBlue : (isHovered ? UiHoveredBlue : Transparent));
	Color32 borderColor = UiSelectedBlue;
	u16 borderWidth = (isHovered && isEnabled) ? 1 : 0;
	Clay__OpenElement();
	Clay__ConfigureOpenElement((Clay_ElementDeclaration){
		.id = btnId,
		.layout = {
			.padding = { .top = 8, .bottom = 8, .left = 4, .right = 4, },
			.sizing = { .width = CLAY_SIZING_GROW(0), },
		},
		.backgroundColor = ToClayColor(backgroundColor),
		.cornerRadius = CLAY_CORNER_RADIUS(4),
		.border = { .width=CLAY_BORDER_OUTSIDE(borderWidth), .color=ToClayColor(borderColor) },
	});
	CLAY({
		.layout = {
			.layoutDirection = CLAY_LEFT_TO_RIGHT,
			.childGap = TOPBAR_ICONS_PADDING,
			.sizing = { .width = CLAY_SIZING_GROW(0) },
			.padding = { .right = 0 },
		},
	})
	{
		if (icon != nullptr)
		{
			CLAY_ICON(icon, FillV2(TOPBAR_ICONS_SIZE), UiTextWhite);
		}
		CLAY_TEXT(
			ToClayString(btnText),
			CLAY_TEXT_CONFIG({
				.fontId = app->clayUiFontId,
				.fontSize = (u16)UI_FONT_SIZE,
				.textColor = ToClayColor(UiTextWhite),
				.wrapMode = CLAY_TEXT_WRAP_NONE,
				.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
				.userData = { .contraction = TextContraction_ClipRight },
			})
		);
		if (!IsEmptyStr(hotkeyStr))
		{
			CLAY({ .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) }, } }) {}
			
			CLAY({ .id=hotkeyId,
				.layout = {
					.layoutDirection = CLAY_LEFT_TO_RIGHT,
					.padding = CLAY_PADDING_ALL(2),
				},
				.border = { .width=CLAY_BORDER_OUTSIDE(1), .color = ToClayColor(UiTextGray) },
				.cornerRadius = CLAY_CORNER_RADIUS(5),
			})
			{
				CLAY_TEXT(
					ToClayString(hotkeyStr),
					CLAY_TEXT_CONFIG({
						.fontId = app->clayUiFontId,
						.fontSize = (u16)UI_FONT_SIZE,
						.textColor = ToClayColor(UiTextGray),
						.wrapMode = CLAY_TEXT_WRAP_NONE,
						.textAlignment = CLAY_TEXT_ALIGN_SHRINK,
						.userData = { .contraction = TextContraction_ClipRight },
					})
				);
			}
		}
	}
	ScratchEnd(scratch);
	return (isHovered && isEnabled && IsMouseBtnPressed(&appIn->mouse, MouseBtn_Left));
}
bool ClayBtnStr(Str8 btnText, Str8 hotkeyStr, bool isEnabled, Texture* icon)
{
	return ClayBtnStrEx(btnText, btnText, hotkeyStr, isEnabled, icon);
}
bool ClayBtn(const char* btnText, const char* hotkeyStr, bool isEnabled, Texture* icon)
{
	return ClayBtnStr(StrLit(btnText), StrLit(hotkeyStr), isEnabled, icon);
}
