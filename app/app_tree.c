/*
File:   app_tree.c
Author: Taylor Robbins
Date:   03\27\2025
Description: 
	** Holds the API for the SkillTree structure which contains some number of TreeNodes and TreeBranches
*/

void FreeTreeNode(SkillTree* tree, TreeNode* node)
{
	NotNull(tree);
	NotNull(tree->arena);
	NotNull(node);
	FreeStr8(tree->arena, &node->name);
	ClearPointer(node);
}

void FreeTreeBranch(SkillTree* tree, TreeBranch* branch)
{
	NotNull(tree);
	NotNull(tree->arena);
	NotNull(branch);
	FreeStr8(tree->arena, &branch->name);
	ClearPointer(branch);
}

void FreeSkillTree(SkillTree* tree)
{
	NotNull(tree);
	if (tree->arena != nullptr)
	{
		VarArrayLoop(&tree->nodes, nIndex)
		{
			VarArrayLoopGet(TreeNode, node, &tree->nodes, nIndex);
			FreeTreeNode(tree, node);
			if (tree->referencesBaked) { FreeVarArray(&node->references); }
		}
		FreeVarArray(&tree->nodes);
		VarArrayLoop(&tree->branches, bIndex)
		{
			VarArrayLoopGet(TreeBranch, branch, &tree->branches, bIndex);
			FreeTreeBranch(tree, branch);
		}
		FreeVarArray(&tree->branches);
	}
	ClearPointer(tree);
}

void InitSkillTree(Arena* arena, SkillTree* treeOut)
{
	NotNull(arena);
	NotNull(treeOut);
	ClearPointer(treeOut);
	treeOut->arena = arena;
	treeOut->nextNodeId = 1;
	treeOut->referencesBaked = false;
	InitVarArray(TreeNode, &treeOut->nodes, arena);
	InitVarArray(TreeBranch, &treeOut->branches, arena);
}

TreeNode* GetTreeNodeById(SkillTree* tree, uxx nodeId)
{
	VarArrayLoop(&tree->nodes, nIndex)
	{
		VarArrayLoopGet(TreeNode, node, &tree->nodes, nIndex);
		if (node->id == nodeId) { return node; }
	}
	return nullptr;
}
TreeBranch* GetTreeBranchById(SkillTree* tree, uxx nodeId, uxx index)
{
	uxx foundIndex = 0;
	VarArrayLoop(&tree->branches, bIndex)
	{
		VarArrayLoopGet(TreeBranch, branch, &tree->branches, bIndex);
		if (branch->fromId == nodeId || branch->toId == nodeId)
		{
			if (foundIndex >= index) { return branch; }
			foundIndex++;
		}
	}
	return nullptr;
}

TreeBranch* GetBranchForNode(SkillTree* tree, TreeNode* node, bool includeIncoming, bool includeOutgoing, uxx index, TreeNode** nodeOut)
{
	NotNull(tree);
	NotNull(node);
	if (tree->referencesBaked)
	{
		uxx foundIndex = 0;
		VarArrayLoop(&node->references, rIndex)
		{
			VarArrayLoopGet(TreeReference, reference, &node->references, rIndex);
			if ((reference->isIncoming && includeIncoming) ||
				(!reference->isIncoming && includeOutgoing))
			{
				if (foundIndex >= index)
				{
					SetOptionalOutPntr(nodeOut, reference->node);
					return reference->branch;
				}
				foundIndex++;
			}
		}
		return nullptr;
	}
	else
	{
		uxx foundIndex = 0;
		VarArrayLoop(&tree->branches, bIndex)
		{
			VarArrayLoopGet(TreeBranch, branch, &tree->branches, bIndex);
			if ((branch->toId == node->id && includeIncoming) ||
				(branch->fromId == node->id && includeOutgoing))
			{
				if (foundIndex >= index)
				{
					if (nodeOut != nullptr) { *nodeOut = GetTreeNodeById(tree, (branch->fromId == node->id) ? branch->toId : branch->fromId); }
					return branch;
				}
				foundIndex++;
			}
		}
		return nullptr;
	}
}

void UnbakeTreeReferences(SkillTree* tree)
{
	NotNull(tree);
	NotNull(tree->arena);
	Assert(tree->referencesBaked);
	
	VarArrayLoop(&tree->nodes, nIndex)
	{
		VarArrayLoopGet(TreeNode, node, &tree->nodes, nIndex);
		FreeVarArray(&node->references);
	}
	VarArrayLoop(&tree->branches, bIndex)
	{
		VarArrayLoopGet(TreeBranch, branch, &tree->branches, bIndex);
		branch->fromPntr = nullptr;
		branch->toPntr = nullptr;
	}
	
	tree->referencesBaked = false;
}
void BakeTreeReferences(SkillTree* tree)
{
	NotNull(tree);
	NotNull(tree->arena);
	Assert(!tree->referencesBaked);
	
	VarArrayLoop(&tree->nodes, nIndex)
	{
		VarArrayLoopGet(TreeNode, node, &tree->nodes, nIndex);
		InitVarArray(TreeReference, &node->references, tree->arena);
	}
	
	VarArrayLoop(&tree->branches, bIndex)
	{
		VarArrayLoopGet(TreeBranch, branch, &tree->branches, bIndex);
		TreeNode* fromNode = GetTreeNodeById(tree, branch->fromId);
		branch->fromPntr = fromNode;
		TreeNode* toNode = GetTreeNodeById(tree, branch->toId);
		branch->toPntr = toNode;
		if (fromNode != nullptr)
		{
			TreeReference* outgoingReference = VarArrayAdd(TreeReference, &fromNode->references);
			NotNull(outgoingReference);
			ClearPointer(outgoingReference);
			outgoingReference->isIncoming = false;
			outgoingReference->branch = branch;
			outgoingReference->node = toNode;
		}
		if (toNode != nullptr)
		{
			TreeReference* incomingReference = VarArrayAdd(TreeReference, &toNode->references);
			NotNull(incomingReference);
			ClearPointer(incomingReference);
			incomingReference->isIncoming = true;
			incomingReference->branch = branch;
			incomingReference->node = fromNode;
		}
	}
	
	tree->referencesBaked = true;
}

void RemoveTreeBranch(SkillTree* tree, TreeBranch* branch)
{
	NotNull(tree);
	NotNull(branch);
	Assert(!tree->referencesBaked);
	uxx branchIndex = 0;
	bool foundIndex = VarArrayGetIndexOf(TreeBranch, &tree->branches, branch, &branchIndex);
	Assert(foundIndex);
	FreeTreeBranch(tree, branch);
	VarArrayRemoveAt(TreeBranch, &tree->branches, branchIndex);
}
void RemoveTreeBranchesForId(SkillTree* tree, uxx nodeId)
{
	NotNull(tree);
	while (true)
	{
		TreeBranch* branch = GetTreeBranchById(tree, nodeId, 0);
		if (branch == nullptr) { break; }
		RemoveTreeBranch(tree, branch);
	}
}

void RemoveTreeNode(SkillTree* tree, TreeNode* node)
{
	NotNull(tree);
	NotNull(node);
	Assert(!tree->referencesBaked);
	uxx nodeIndex = 0;
	bool foundIndex = VarArrayGetIndexOf(TreeNode, &tree->nodes, node, &nodeIndex);
	Assert(foundIndex);
	FreeTreeNode(tree, node);
	VarArrayRemoveAt(TreeNode, &tree->nodes, nodeIndex);
}
void RemoveTreeNodeById(SkillTree* tree, uxx nodeId)
{
	TreeNode* node = GetTreeNodeById(tree, nodeId);
	NotNull(node);
	RemoveTreeNode(tree, node);
}

TreeNode* AddTreeNode(SkillTree* tree, TreeNodeType type, Str8 name, v2 position, Color32 color)
{
	NotNull(tree);
	NotNull(tree->arena);
	Assert(!tree->referencesBaked);
	TreeNode* result = VarArrayAdd(TreeNode, &tree->nodes);
	NotNull(result);
	ClearPointer(result);
	result->id = tree->nextNodeId;
	tree->nextNodeId++;
	result->type = type;
	result->name = AllocStr8(tree->arena, name);
	result->position = position;
	result->color = color;
	return result;
}

TreeBranch* AddTreeBranch(SkillTree* tree, TreeBranchType type, Str8 name, uxx fromId, uxx toId)
{
	NotNull(tree);
	NotNull(tree->arena);
	Assert(!tree->referencesBaked);
	TreeBranch* result = VarArrayAdd(TreeBranch, &tree->branches);
	NotNull(result);
	ClearPointer(result);
	result->type = type;
	result->name = AllocStr8(tree->arena, name);
	result->fromId = fromId;
	result->toId = toId;
	return result;
}
