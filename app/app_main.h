/*
File:   app_main.h
Author: Taylor Robbins
Date:   02\25\2025
*/

#ifndef _APP_MAIN_H
#define _APP_MAIN_H

typedef struct AppData AppData;
struct AppData
{
	bool initialized;
	RandomSeries random;
	
	Shader mainShader;
	PigFont uiFont;
	
	ClayUIRenderer clay;
	u16 clayUiFontId;
	
	bool isFileMenuOpen;
	bool keepFileMenuOpenUntilMouseOver;
	
	SkillTree tree;
	
	v2 viewPosition; //center of view
	bool isMovingView;
	v2 movingViewGrabPos;
	rec graphBounds;
	
	TreeNode* hoveredNode;
	bool isMovingNode;
	v2 movingNodeGrabOffset;
	uxx movingNodeId;
};

#endif //  _APP_MAIN_H
