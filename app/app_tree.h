/*
File:   app_tree.h
Author: Taylor Robbins
Date:   03\27\2025
*/

#ifndef _APP_TREE_H
#define _APP_TREE_H

typedef enum TreeNodeType TreeNodeType;
enum TreeNodeType
{
	TreeNodeType_None = 0,
	TreeNodeType_Concept,
	TreeNodeType_Language,
	TreeNodeType_API,
	TreeNodeType_Project,
	TreeNodeType_Count,
};
const char* GetTreeNodeTypeStr(TreeNodeType enumValue)
{
	switch (enumValue)
	{
		case TreeNodeType_None:     return "None";
		case TreeNodeType_Concept:  return "Concept";
		case TreeNodeType_Language: return "Language";
		case TreeNodeType_API:      return "API";
		case TreeNodeType_Project:  return "Project";
		default: return UNKNOWN_STR;
	}
}

typedef enum TreeBranchType TreeBranchType;
enum TreeBranchType
{
	TreeBranchType_None = 0,
	TreeBranchType_Dependency,
	TreeBranchType_Commonality,
	TreeBranchType_Reference,
	TreeBranchType_Count,
};
const char* GetTreeBranchTypeStr(TreeBranchType enumValue)
{
	switch (enumValue)
	{
		case TreeBranchType_None:        return "None";
		case TreeBranchType_Dependency:  return "Dependency";
		case TreeBranchType_Commonality: return "Commonality";
		case TreeBranchType_Reference:   return "Reference";
		default: return UNKNOWN_STR;
	}
}

typedef struct TreeReference TreeReference;
struct TreeReference
{
	bool isIncoming; //if true, we should be the branch->toId, otherwise we should be the branch->fromId
	struct TreeBranch* branch;
	struct TreeNode* node;
};

typedef struct TreeNode TreeNode;
struct TreeNode
{
	uxx id;
	TreeNodeType type;
	Str8 name;
	v2 position;
	Color32 color;
	VarArray references; //TreeReference (only filled if tree->referencesBaked)
};

typedef struct TreeBranch TreeBranch;
struct TreeBranch
{
	TreeBranchType type;
	Str8 name;
	union
	{
		struct { uxx fromId; uxx toId; };
		struct { uxx leftId; uxx rightId; };
		struct { uxx firstId; uxx secondId; };
		struct { uxx dependencyId; uxx dependentId; };
	};
	union
	{
		// These are only filled if tree->referencesBaked
		struct { TreeNode* fromPntr; TreeNode* toPntr; };
		struct { TreeNode* leftPntr; TreeNode* rightPntr; };
		struct { TreeNode* firstPntr; TreeNode* secondPntr; };
		struct { TreeNode* dependencyPntr; TreeNode* dependentPntr; };
	};
};

typedef struct SkillTree SkillTree;
struct SkillTree
{
	Arena* arena;
	uxx nextNodeId;
	bool referencesBaked;
	VarArray nodes; //TreeNode
	VarArray branches; //TreeBranch
};

#endif //  _APP_TREE_H
