/*
File:   app_main.c
Author: Taylor Robbins
Date:   01\19\2025
Description: 
	** Contains the dll entry point and all exported functions that the platform
	** layer can lookup by name and call. Also includes all other source files
	** required for the application to compile.
*/

#include "build_config.h"
#include "defines.h"
#define PIG_CORE_IMPLEMENTATION BUILD_INTO_SINGLE_UNIT

#include "base/base_all.h"
#include "std/std_all.h"
#include "os/os_all.h"
#include "mem/mem_all.h"
#include "struct/struct_all.h"
#include "misc/misc_all.h"
#include "input/input_all.h"
#include "file_fmt/file_fmt_all.h"
#include "ui/ui_all.h"
#include "gfx/gfx_all.h"
#include "gfx/gfx_system_global.h"
#include "phys/phys_all.h"

// +--------------------------------------------------------------+
// |                         Header Files                         |
// +--------------------------------------------------------------+
#include "platform_interface.h"
#include "main2d_shader.glsl.h"
#include "app_main.h"

// +--------------------------------------------------------------+
// |                           Globals                            |
// +--------------------------------------------------------------+
static AppData* app = nullptr;
static AppInput* appIn = nullptr;

#if !BUILD_INTO_SINGLE_UNIT //NOTE: The platform layer already has these globals
static PlatformInfo* platformInfo = nullptr;
static PlatformApi* platform = nullptr;
static Arena* stdHeap = nullptr;
#endif

// +--------------------------------------------------------------+
// |                         Source Files                         |
// +--------------------------------------------------------------+
#include "app_helpers.c"
#include "app_clay_widgets.c"

// +==============================+
// |           DllMain            |
// +==============================+
#if (TARGET_IS_WINDOWS && !BUILD_INTO_SINGLE_UNIT)
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL, // handle to DLL module
	DWORD fdwReason,    // reason for calling function
	LPVOID lpReserved)
{
	UNUSED(hinstDLL);
	UNUSED(lpReserved);
	switch(fdwReason)
	{ 
		case DLL_PROCESS_ATTACH: break;
		case DLL_PROCESS_DETACH: break;
		case DLL_THREAD_ATTACH: break;
		case DLL_THREAD_DETACH: break;
		default: break;
	}
	//If we don't return TRUE here then the LoadLibraryA call will return a failure!
	return TRUE;
}
#endif //(TARGET_IS_WINDOWS && !BUILD_INTO_SINGLE_UNIT)

void UpdateDllGlobals(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi, void* memoryPntr, AppInput* appInput)
{
	#if !BUILD_INTO_SINGLE_UNIT
	platformInfo = inPlatformInfo;
	platform = inPlatformApi;
	stdHeap = inPlatformInfo->platformStdHeap;
	#else
	UNUSED(inPlatformApi);
	UNUSED(inPlatformInfo);
	#endif
	app = (AppData*)memoryPntr;
	appIn = appInput;
}

// +==============================+
// |           AppInit            |
// +==============================+
// void* AppInit(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi)
EXPORT_FUNC(AppInit) APP_INIT_DEF(AppInit)
{
	#if !BUILD_INTO_SINGLE_UNIT
	InitScratchArenasVirtual(Gigabytes(4));
	#endif
	ScratchBegin(scratch);
	ScratchBegin1(scratch2, scratch);
	ScratchBegin2(scratch3, scratch, scratch2);
	
	AppData* appData = AllocType(AppData, inPlatformInfo->platformStdHeap);
	ClearPointer(appData);
	UpdateDllGlobals(inPlatformInfo, inPlatformApi, (void*)appData, nullptr);
	
	#if BUILD_WITH_SOKOL_APP
	platform->SetWindowTitle(StrLit(PROJECT_READABLE_NAME_STR));
	LoadWindowIcon();
	#endif
	
	InitRandomSeriesDefault(&app->random);
	SeedRandomSeriesU64(&app->random, OsGetCurrentTimestamp(false));
	
	InitCompiledShader(&app->mainShader, stdHeap, main2d);
	
	FontCharRange fontCharRanges[] = {
		FontCharRange_ASCII,
		FontCharRange_LatinExt,
		NewFontCharRangeSingle(UNICODE_ELLIPSIS_CODEPOINT),
		NewFontCharRangeSingle(UNICODE_RIGHT_ARROW_CODEPOINT),
	};
	
	app->uiFont = InitFont(stdHeap, StrLit("uiFont"));
	Result attachUiFontTtfResult = AttachOsTtfFileToFont(&app->uiFont, StrLit(UI_FONT_NAME), UI_FONT_SIZE, UI_FONT_STYLE);
	Assert(attachUiFontTtfResult == Result_Success);
	Result uiFontBakeResult = BakeFontAtlas(&app->uiFont, UI_FONT_SIZE, UI_FONT_STYLE, NewV2i(256, 256), ArrayCount(fontCharRanges), &fontCharRanges[0]);
	Assert(uiFontBakeResult == Result_Success);
	RemoveAttachedTtfFile(&app->uiFont);
	
	InitClayUIRenderer(stdHeap, V2_Zero, &app->clay);
	app->clayUiFontId = AddClayUIRendererFont(&app->clay, &app->uiFont, UI_FONT_STYLE);
	
	app->initialized = true;
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
	return (void*)app;
}

// +==============================+
// |          AppUpdate           |
// +==============================+
// bool AppUpdate(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi, void* memoryPntr, AppInput* appInput)
EXPORT_FUNC(AppUpdate) APP_UPDATE_DEF(AppUpdate)
{
	ScratchBegin(scratch);
	ScratchBegin1(scratch2, scratch);
	ScratchBegin2(scratch3, scratch, scratch2);
	bool shouldContinueRunning = true;
	UpdateDllGlobals(inPlatformInfo, inPlatformApi, memoryPntr, appInput);
	v2 screenSize = ToV2Fromi(appIn->screenSize);
	v2 screenCenter = Div(screenSize, 2.0f);
	v2 mousePos = appIn->mouse.position;
	
	//TODO: Implement me!
	
	// +--------------------------------------------------------------+
	// |                            Render                            |
	// +--------------------------------------------------------------+
	BeginFrame(platform->GetSokolSwapchain(), appIn->screenSize, UiBackgroundBlack, 1.0f);
	{
		BindShader(&app->mainShader);
		ClearDepthBuffer(1.0f);
		SetDepth(1.0f);
		mat4 projMat = Mat4_Identity;
		TransformMat4(&projMat, MakeScaleXYZMat4(1.0f/(screenSize.Width/2.0f), 1.0f/(screenSize.Height/2.0f), 1.0f));
		TransformMat4(&projMat, MakeTranslateXYZMat4(-1.0f, -1.0f, 0.0f));
		TransformMat4(&projMat, MakeScaleYMat4(-1.0f));
		SetProjectionMat(projMat);
		SetViewMat(Mat4_Identity);
		
		bool isScrolling = UpdateClayScrolling(&app->clay.clay, 1000.0f/60.0f, false, appIn->mouse.scrollDelta, false);
		UNUSED(isScrolling);
		BeginClayUIRender(&app->clay.clay, screenSize, false, appIn->mouse.position, IsMouseBtnDown(&appIn->mouse, MouseBtn_Left));
		{
			CLAY({ .id=CLAY_ID("FullscreenContainer"),
				.layout =
				{
					.layoutDirection = CLAY_TOP_TO_BOTTOM,
					.sizing = { .width=CLAY_SIZING_FIXED(screenSize.Width), .height=CLAY_SIZING_FIXED(screenSize.Height) },
				},
			})
			{
				CLAY({ .id = CLAY_ID("Topbar"),
					.layout = {
						.sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIT(0) },
						.padding = { 0, 0, 0, 1 },
						.childGap = 2,
						.childAlignment = { .y = CLAY_ALIGN_Y_CENTER },
					},
					.backgroundColor = ToClayColor(UiBackgroundDarkGray),
					.border = { .color=ToClayColor(UiBackgroundGray), .width={ .bottom=1 } },
				})
				{
					if (ClayTopBtn("File", false, &app->isFileMenuOpen, &app->keepFileMenuOpenUntilMouseOver, false))
					{
						if (ClayBtn("Open" UNICODE_ELLIPSIS_STR, "Ctrl+O", true, nullptr))
						{
							//TODO: Implement me!
							app->isFileMenuOpen = false;
						} Clay__CloseElement();
						
						Clay__CloseElement();
						Clay__CloseElement();
					} Clay__CloseElement();
				}
			}
		}
		Clay_RenderCommandArray uiRenderCommands = EndClayUIRender(&app->clay.clay);
		RenderClayCommandArray(&app->clay, &gfx, &uiRenderCommands);
	}
	EndFrame();
	
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
	return shouldContinueRunning;
}

// +==============================+
// |          AppClosing          |
// +==============================+
// void AppClosing(PlatformInfo* inPlatformInfo, PlatformApi* inPlatformApi, void* memoryPntr)
EXPORT_FUNC(AppClosing) APP_CLOSING_DEF(AppClosing)
{
	ScratchBegin(scratch);
	ScratchBegin1(scratch2, scratch);
	ScratchBegin2(scratch3, scratch, scratch2);
	UpdateDllGlobals(inPlatformInfo, inPlatformApi, memoryPntr, nullptr);
	
	#if BUILD_WITH_IMGUI
	igSaveIniSettingsToDisk(app->imgui->io->IniFilename);
	#endif
	
	ScratchEnd(scratch);
	ScratchEnd(scratch2);
	ScratchEnd(scratch3);
}

// +==============================+
// |          AppGetApi           |
// +==============================+
// AppApi AppGetApi()
EXPORT_FUNC(AppGetApi) APP_GET_API_DEF(AppGetApi)
{
	AppApi result = ZEROED;
	result.AppInit = AppInit;
	result.AppUpdate = AppUpdate;
	result.AppClosing = AppClosing;
	return result;
}
