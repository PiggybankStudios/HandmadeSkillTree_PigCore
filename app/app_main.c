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
#include "app_tree.h"
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
#include "app_tree.c"
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
	
	InitSkillTree(stdHeap, &app->tree);
	uxx softwareRasterId = AddTreeNode(&app->tree, TreeNodeType_Concept, StrLit("Software Rasterization"), V2_Zero, MonokaiBlue)->id;
	
	uxx rustId = AddTreeNode(&app->tree, TreeNodeType_Language, StrLit("Rust"), V2_Zero, MonokaiBlue)->id;
	uxx odinId = AddTreeNode(&app->tree, TreeNodeType_Language, StrLit("Odin"), V2_Zero, MonokaiBlue)->id;
	uxx zigId = AddTreeNode(&app->tree, TreeNodeType_Language, StrLit("Zig"), V2_Zero, MonokaiBlue)->id;
	uxx cppId = AddTreeNode(&app->tree, TreeNodeType_Language, StrLit("C/C++"), V2_Zero, MonokaiBlue)->id;
	
	uxx win32ApiId = AddTreeNode(&app->tree, TreeNodeType_API, StrLit("Win32"), V2_Zero, MonokaiBlue)->id;
	uxx winAudioId = AddTreeNode(&app->tree, TreeNodeType_API, StrLit("WinAudio"), V2_Zero, MonokaiBlue)->id;
	
	uxx stbLibrariesId = AddTreeNode(&app->tree, TreeNodeType_Project, StrLit("Stb Libraries"), V2_Zero, MonokaiBlue)->id;
	AddTreeBranch(&app->tree, TreeBranchType_Dependency, Str8_Empty, cppId, stbLibrariesId);
	
	uxx handmadeHeroNodeId = AddTreeNode(&app->tree, TreeNodeType_Project, StrLit("Handmade Hero"), V2_Zero, MonokaiBlue)->id;
	AddTreeBranch(&app->tree, TreeBranchType_Dependency, Str8_Empty, cppId, handmadeHeroNodeId);
	AddTreeBranch(&app->tree, TreeBranchType_Dependency, Str8_Empty, win32ApiId, handmadeHeroNodeId);
	AddTreeBranch(&app->tree, TreeBranchType_Dependency, Str8_Empty, stbLibrariesId, handmadeHeroNodeId);
	AddTreeBranch(&app->tree, TreeBranchType_Dependency, Str8_Empty, softwareRasterId, handmadeHeroNodeId);
	AddTreeBranch(&app->tree, TreeBranchType_Dependency, Str8_Empty, winAudioId, handmadeHeroNodeId);
	
	BakeTreeReferences(&app->tree);
	VarArrayLoop(&app->tree.nodes, nIndex)
	{
		VarArrayLoopGet(TreeNode, node, &app->tree.nodes, nIndex);
		node->position.X = GetRandR32Range(&app->random, -100, 100);
		node->position.Y = GetRandR32Range(&app->random, -100, 100);
	}
	
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
	ClayId viewportClayId = CLAY_ID("Viewport");
	rec viewportRec = GetClayElementDrawRec(viewportClayId);
	bool viewportRecReady = (viewportRec.Width > 0 && viewportRec.Height > 0);
	v2 viewportHalfSize = Div(viewportRec.Size, 2.0f);
	
	// +==================================+
	// | Mouse View with MouseBtn_Middle  |
	// +==================================+
	if (app->isMovingView)
	{
		if (!IsMouseBtnDown(&appIn->mouse, MouseBtn_Middle)) { app->isMovingView = false; }
		else
		{
			app->viewPosition = Sub(app->movingViewGrabPos, Sub(Sub(mousePos, viewportRec.TopLeft), viewportHalfSize));
			app->viewPosition.X = ClampR32(app->viewPosition.X, app->graphBounds.X - viewportRec.Width/2 + 10, app->graphBounds.X + app->graphBounds.Width + viewportRec.Width/2 - 10);
			app->viewPosition.Y = ClampR32(app->viewPosition.Y, app->graphBounds.Y - viewportRec.Height/2 + 10, app->graphBounds.Y + app->graphBounds.Height + viewportRec.Height/2 - 10);
		}
	}
	else if (IsInsideRec(viewportRec, mousePos)) //TODO: Somehow we need to know if the mouse is over something else that is overlapping with the viewport!
	{
		if (IsMouseBtnPressed(&appIn->mouse, MouseBtn_Middle))
		{
			app->isMovingView = true;
			app->movingViewGrabPos = Add(Sub(Sub(mousePos, viewportRec.TopLeft), viewportHalfSize), app->viewPosition);
		}
	}
	
	// +==============================+
	// |      Find Hovered Node       |
	// +==============================+
	app->hoveredNode = nullptr;
	if (app->isMovingNode)
	{
		app->hoveredNode = GetTreeNodeById(&app->tree, app->movingNodeId);
	}
	else if (IsInsideRec(viewportRec, mousePos)) //TODO: Somehow we need to know if the mouse is over something else that is overlapping with the viewport!
	{
		app->graphBounds = Rec_Zero;
		VarArrayLoop(&app->tree.nodes, nIndex)
		{
			VarArrayLoopGet(TreeNode, node, &app->tree.nodes, nIndex);
			ClayId nodeClayId = ToClayIdPrint("Node%llu", (u64)node->id);
			rec nodeDrawRec = GetClayElementDrawRec(nodeClayId);
			if (nIndex == 0) { app->graphBounds = nodeDrawRec; }
			else { app->graphBounds = BothRec(app->graphBounds, nodeDrawRec); }
			if (IsInsideRec(nodeDrawRec, mousePos))
			{
				app->hoveredNode = node;
			}
		}
	}
	
	// +==============================+
	// |     Update Graph Bounds      |
	// +==============================+
	if (viewportRecReady)
	{
		app->graphBounds = Rec_Zero;
		VarArrayLoop(&app->tree.nodes, nIndex)
		{
			VarArrayLoopGet(TreeNode, node, &app->tree.nodes, nIndex);
			ClayId nodeClayId = ToClayIdPrint("Node%llu", (u64)node->id);
			ClayId nodeNameIdStr = ToClayIdPrint("Node%lluName", (u64)node->id);
			rec nodeUiRec = GetClayElementDrawRec(nodeClayId);
			rec nodeNameUiRec = GetClayElementDrawRec(nodeNameIdStr);
			rec nodeRec = NewRecCenteredV(node->position, nodeUiRec.Size);
			rec nameRec = NewRecV(Add(nodeRec.TopLeft, Sub(nodeNameUiRec.TopLeft, nodeUiRec.TopLeft)), nodeNameUiRec.Size);
			if (nIndex == 0) { app->graphBounds = nodeRec; }
			else { app->graphBounds = BothRec(app->graphBounds, nodeRec); }
			app->graphBounds = BothRec(app->graphBounds, nameRec);
		}
	}
	
	// +==============================+
	// |    Move Nodes With Mouse     |
	// +==============================+
	if (app->hoveredNode != nullptr && !app->isMovingNode)
	{
		if (IsMouseBtnPressed(&appIn->mouse, MouseBtn_Left))
		{
			ClayId nodeClayId = ToClayIdPrint("Node%llu", (u64)app->hoveredNode->id);
			rec nodeDrawRec = GetClayElementDrawRec(nodeClayId);
			if (nodeDrawRec.Width > 0 && nodeDrawRec.Height > 0)
			{
				v2 nodeCenter = Add(nodeDrawRec.TopLeft, Div(nodeDrawRec.Size, 2.0f));
				app->isMovingNode = true;
				app->movingNodeGrabOffset = Sub(mousePos, nodeCenter);
				app->movingNodeId = app->hoveredNode->id;
			}
		}
	}
	if (app->isMovingNode)
	{
		TreeNode* movingNode = GetTreeNodeById(&app->tree, app->movingNodeId);
		if (movingNode == nullptr) { app->isMovingNode = false; }
		else if (viewportRecReady)
		{
			if (!IsMouseBtnDown(&appIn->mouse, MouseBtn_Left)) { app->isMovingNode = false; }
			else
			{
				v2 newPosition = Add(Sub(Sub(Sub(mousePos, app->movingNodeGrabOffset), viewportRec.TopLeft), viewportHalfSize), app->viewPosition);
				movingNode->position = newPosition;
			}
		}
	}
	
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
		
		// +==============================+
		// |          Render UI           |
		// +==============================+
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
				// +==============================+
				// |        Render Topbar         |
				// +==============================+
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
				
				// +==============================+
				// |       Render Viewport        |
				// +==============================+
				v2 viewportOffset = Sub(viewportHalfSize, app->viewPosition);
				CLAY({ .id=viewportClayId,
					.layout = { .sizing = { .width=CLAY_SIZING_GROW(0), .height=CLAY_SIZING_GROW(0) } },
				})
				{
					// +==============================+
					// |     Render Tree Branches     |
					// +==============================+
					if (viewportRecReady)
					{
						VarArrayLoop(&app->tree.branches, bIndex)
						{
							VarArrayLoopGet(TreeBranch, branch, &app->tree.branches, bIndex);
							if (branch->fromPntr != nullptr && branch->toPntr != nullptr)
							{
								// Str8 fromNodeUiIdStr = PrintInArenaStr(scratch, "Node%llu", (u64)branch->fromId);
								// Str8 toNodeUiIdStr = PrintInArenaStr(scratch, "Node%llu", (u64)branch->toId);
								v2 startPos = Add(Add(branch->fromPntr->position, viewportOffset), viewportRec.TopLeft);
								v2 endPos = Add(Add(branch->toPntr->position, viewportOffset), viewportRec.TopLeft);
								DrawLine(startPos, endPos, 3.0f, UiHoveredBlue);
							}
						}
					}
					
					// +==============================+
					// |      Render Tree Nodes       |
					// +==============================+
					VarArrayLoop(&app->tree.nodes, nIndex)
					{
						VarArrayLoopGet(TreeNode, node, &app->tree.nodes, nIndex);
						Str8 nodeIdStr = PrintInArenaStr(scratch, "Node%llu", (u64)node->id);
						Str8 nodeNameIdStr = PrintInArenaStr(scratch, "Node%lluName", (u64)node->id);
						bool isHovered = (app->hoveredNode == node);
						bool isMoving = (app->isMovingNode && app->movingNodeId == node->id);
						
						u16 borderWidth = 0;
						Color32 borderColor = Transparent;
						if (isMoving) { borderWidth = 1; borderColor = MonokaiYellow; }
						else if (isHovered) { borderWidth = 2; borderColor = MonokaiLightBlue; }
						
						CLAY({ .id = ToClayId(nodeIdStr),
							.layout = {
								.sizing = { .width = CLAY_SIZING_FIXED(32), .height = CLAY_SIZING_FIXED(32) },
							},
							.floating = {
								.attachTo = CLAY_ATTACH_TO_PARENT,
								.zIndex = -2,
								.offset = ToClayVector2(Add(node->position, viewportOffset)),
								.attachPoints = { .element = CLAY_ATTACH_POINT_CENTER_CENTER },
							},
							.cornerRadius = CLAY_CORNER_RADIUS(8),
							.backgroundColor = ToClayColor(node->color),
							.border = { .width=CLAY_BORDER_OUTSIDE(borderWidth), .color=ToClayColor(borderColor) },
						})
						{
							
							CLAY({ .id = ToClayId(nodeNameIdStr),
								.layout = {
									.sizing = { .width = CLAY_SIZING_FIT(0, MAX_NODE_NAME_WIDTH), .height = CLAY_SIZING_FIT(0) },
									.padding = { .bottom = 5 },
								},
								.floating = {
									.zIndex = -1,
									.attachTo = CLAY_ATTACH_TO_PARENT,
									.attachPoints = { .parent = CLAY_ATTACH_POINT_CENTER_TOP, .element = CLAY_ATTACH_POINT_CENTER_BOTTOM },
								},
							})
							{
								CLAY_TEXT(
									ToClayString(node->name),
									CLAY_TEXT_CONFIG({
										.fontId = app->clayUiFontId,
										.fontSize = UI_FONT_SIZE,
										.textColor = ToClayColor(UiTextWhite),
										.wrapMode = CLAY_TEXT_WRAP_WORDS,
										.textAlignment = CLAY_TEXT_ALIGN_CENTER,
									})
								);
							}
						}
					}
					
					#if DEBUG_BUILD
					CLAY({.id = CLAY_ID("Graph Bounds"),
						.layout = {
							.sizing = { .width = CLAY_SIZING_FIXED(app->graphBounds.Width), .height = CLAY_SIZING_FIXED(app->graphBounds.Height) },
						},
						.floating = {
							.zIndex = -3,
							.attachTo = CLAY_ATTACH_TO_PARENT,
							.offset = ToClayVector2(Add(app->graphBounds.TopLeft, viewportOffset)),
						},
						.border = { .width=CLAY_BORDER_OUTSIDE(3), .color=ToClayColor(UiHoveredBlue) },
					}) {}
					#endif
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
