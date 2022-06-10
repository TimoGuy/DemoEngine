#pragma once

#include <string>
#include <map>
#include <vector>

class Animator;


struct AnimationStateMachine_TransitionCondition
{
	size_t variableIndex;
	float compareToValue;
	enum ASMComparisonOperator
	{
		EQUAL,
		NEQUAL,
		LESSER,
		GREATER,
		LEQUAL,
		GEQUAL
	} comparisonOperator;
};
struct AnimationStateMachine_TransitionConditionGroup
{
	std::vector<AnimationStateMachine_TransitionCondition> transitionConditions;
};
struct AnimationStateMachine_Transition		// NOTE: the specialCaseCurrentASMNodeName isn't included here bc the transition list is connected to all of the nodes in such a way that it's not needed  -Timo
{
	size_t toNodeIndex;
	std::vector<AnimationStateMachine_TransitionConditionGroup> transitionConditionGroups;
};


struct AnimationStateMachine_Node		// The node index will be what index it's stored in the node list
{
	size_t animationIndex;

	bool isBlendTreeAnimation;
	size_t animationIndex2;
	size_t blendTreeVariableIndex;
	float animationBlendBorder1;
	float animationBlendBorder2;

	bool loopAnimation;
	bool doNotTransitionUntilAnimationFinished;
	float transitionTime;		// It's here for now... it may be necessary to move this to the _Transition struct depending on how the data should get structured.

	std::vector<AnimationStateMachine_Transition> transitions;
};

struct AnimationStateMachine_Variable
{
	float value;
	bool isTriggerVariable;		// @NOTE: this is set to true if variable name starts with the name "trigger"
};


/**
 * @brief I guess this is what you call a Decorator Class
 * 
 * It decorates the Animator class and allows you to load in a .hsmm file that contains Animator State Machine information.
 * 
 */
class AnimatorStateMachine
{
public:
	AnimatorStateMachine() { }		// @HACK: Compiler appeasement
	AnimatorStateMachine(const std::string& asmFName, Animator* animator);
	void updateStateMachine(float deltaTime);

	inline void setVariable(const std::string& varName, bool value) { setVariable(varName, (float)value); }
	inline void setVariable(const std::string& varName, int value) { setVariable(varName, (float)value); }
	void setVariable(const std::string& varName, float value);

private:
	void moveToStateMachineNode(size_t nodeIndex);

	Animator* animatorPtr;

	std::map<std::string, size_t> variableNameIndexMap;		// Populates in the constructor!
	std::vector<std::string> variableNames;						// Hopefully this is fast eh!
	std::vector<AnimationStateMachine_Variable> variableValues;						// Hopefully this is fast eh!

	size_t currentASMNode;
	std::vector<AnimationStateMachine_Node> nodeList;
};
