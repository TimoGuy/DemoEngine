#include "AnimatorStateMachine.h"

#include <iostream>
#include "Animator.h"
#include "../../../utils/FileLoading.h"
#include "../../../utils/json.hpp"


AnimatorStateMachine::AnimatorStateMachine(const std::string& asmFName, Animator* animator) : animatorPtr(animator)
{
	nlohmann::json hsmm = FileLoading::loadJsonFile("res/model/" + std::string(asmFName) + ".hsmm");

	if (!hsmm.contains("animation_state_machine"))
	{
		// It makes no sense to create an animator state machine
		// and provide data that has no state machine information
		std::cout << "ASM: CONSTR: YA STUPID???!???!? (everything is missing)" << std::endl;
		return;
	}
	if (!hsmm.contains("included_anims"))
	{
		std::cout << "ASM: CONSTR: YA STUPID???!???!? (imported anims section is missing)" << std::endl;
		return;
	}

	const nlohmann::json& animStateMachine = hsmm["animation_state_machine"];
	if (!animStateMachine.contains("asm_variables"))
	{
		std::cout << "ASM: CONSTR: YA STUPID???!???!? (variables are missing)" << std::endl;
		return;
	}
	if (!animStateMachine.contains("asm_nodes"))
	{
		std::cout << "ASM: CONSTR: YA STUPID???!???!? (nodes are missing)" << std::endl;
		return;
	}
	//if (!animStateMachine["asm_nodes"].contains("node_transition_condition_groups"))
	//{
	//	std::cout << "ASM: CONSTR: YA STUPID???!???!? (node transitions are missing)" << std::endl;
	//	return;
	//}

	//
	// Populate all of the variables
	//
	for (auto asmVar_j : animStateMachine["asm_variables"])
	{
		variableNameIndexMap[asmVar_j["var_name"]] = variableNames.size();
		variableNames.push_back(asmVar_j["var_name"]);
		variableValues.push_back(asmVar_j["value"]);
	}

	//
	// Populate all of the animation state machine nodes
	//
	currentASMNode = 0;
	if (animStateMachine.contains("asm_start_node_index"))
		currentASMNode = (size_t)animStateMachine["asm_start_node_index"];

	std::map<std::string, size_t> animNameToIndexMap;
	std::vector<std::string> includedAnims = hsmm["included_anims"];
	for (size_t i = 0; i < includedAnims.size(); i++)
	{
		animNameToIndexMap[includedAnims[i]] = i;
	}

	std::vector<std::string> nodeNameList;
	std::map<std::string, size_t> nodeNameToIndexMap;
	for (auto asmNode_j : animStateMachine["asm_nodes"])
	{
		nodeNameList.push_back(asmNode_j["node_name"]);

		AnimationStateMachine_Node node;
		node.animationIndex = animNameToIndexMap[asmNode_j["animation_name_1"]];

		node.isBlendTreeAnimation = !std::string(asmNode_j["animation_name_2"]).empty();
		if (node.isBlendTreeAnimation)
		{
			node.animationIndex2 = animNameToIndexMap[asmNode_j["animation_name_2"]];
			node.blendTreeVariableIndex = variableNameIndexMap[asmNode_j["animation_blend_variable"]];
			node.animationBlendBorder1 = asmNode_j["animation_blend_boundary_1"];
			node.animationBlendBorder2 = asmNode_j["animation_blend_boundary_2"];
		}

		node.loopAnimation = asmNode_j["loop_animation"];
		node.doNotTransitionUntilAnimationFinished = asmNode_j["dont_transition_until_animation_finished"];
		node.transitionTime = asmNode_j["transition_time"];

		nodeNameToIndexMap[asmNode_j["node_name"]] = nodeList.size();
		nodeList.push_back(node);
	}

	//
	// String together all of the transitions
	//
	size_t nodeIndex = 0;
	for (auto asmNode_j : animStateMachine["asm_nodes"])
	{
		for (auto asmTranConditionGroup_j : asmNode_j["node_transition_condition_groups"])
		{
			std::vector<std::string> relevantNodes = nodeNameList;
			auto itr = relevantNodes.cbegin();		// @Possibly_not_needed: remove the reference to self in the relevant transition nodes.
			while (itr != relevantNodes.cend())
			{
				if (*itr == asmNode_j["node_name"])
				{
					itr = relevantNodes.erase(itr);
				}
				else
					itr++;
			}

			AnimationStateMachine_TransitionConditionGroup transitionConditionGroup;

			// Bundle together all the transition conditions for the transition condition group
			for (auto asmTranCondition_j : asmTranConditionGroup_j)
			{
				static const std::string specialCaseKey = "JASDKFHASKDGH#@$H@K!%K!H@#KH!@#K!$BFBNSDAFNANSDF  ";		// @COPYPASTA
				if (asmTranCondition_j["var_name"] == specialCaseKey)
				{
					// Add a new rule that affects the relevantNodes for this condition group
					auto comparisonOperator = (AnimationStateMachine_TransitionCondition::ASMComparisonOperator)(int)asmTranCondition_j["comparison_operator"];
					std::string relevantNodesNewRule = asmTranCondition_j["current_asm_node_name_ref"];

					switch (comparisonOperator)
					{
					case AnimationStateMachine_TransitionCondition::ASMComparisonOperator::EQUAL:
					{
						auto itr = relevantNodes.cbegin();
						while (itr != relevantNodes.cend())
						{
							if (*itr != relevantNodesNewRule)		// @NOTE: only *leave* those that are equal, hence the !=
							{
								itr = relevantNodes.erase(itr);
							}
							else
								itr++;
						}
					}
					break;

					case AnimationStateMachine_TransitionCondition::ASMComparisonOperator::NEQUAL:
					{
						auto itr = relevantNodes.cbegin();
						while (itr != relevantNodes.cend())
						{
							if (*itr == relevantNodesNewRule)
							{
								itr = relevantNodes.erase(itr);
							}
							else
								itr++;
						}
					}
					break;
					}
				}
				else
				{
					// Add a regular transition condition to this group
					AnimationStateMachine_TransitionCondition transitionCondition;
					transitionCondition.variableIndex = variableNameIndexMap[asmTranCondition_j["var_name"]];
					transitionCondition.comparisonOperator = (AnimationStateMachine_TransitionCondition::ASMComparisonOperator)(int)asmTranCondition_j["comparison_operator"];
					transitionCondition.compareToValue = asmTranCondition_j["compare_to_value"];
					transitionConditionGroup.transitionConditions.push_back(transitionCondition);
				}
			}

			// Search thru all the relevant node names and add this transition
			for (auto relevantNodeName : relevantNodes)
			{
				auto& node = nodeList[nodeNameToIndexMap[relevantNodeName]];
				
				// See if there are any transitions with this current node index yet.
				AnimationStateMachine_Transition* transitionPtr = nullptr;
				for (auto transition : node.transitions)
				{
					if (transition.toNodeIndex == nodeIndex)
					{
						transitionPtr = &transition;
						break;
					}
				}

				if (transitionPtr == nullptr)
				{
					node.transitions.push_back({ nodeIndex });
					transitionPtr = &node.transitions[node.transitions.size() - 1];
				}

				// Tack on this transition condition group!
				transitionPtr->transitionConditionGroups.push_back(transitionConditionGroup);
			}
		}

		nodeIndex++;
	}

	// Kick off the animation state machine
	moveToStateMachineNode(currentASMNode);
}


void AnimatorStateMachine::updateStateMachine(float deltaTime)
{
	//
	// Watch for any transition conditions
	//
	if (!nodeList[currentASMNode].doNotTransitionUntilAnimationFinished ||
		animatorPtr->isAnimationFinished(nodeList[currentASMNode].animationIndex, deltaTime))
	{
		for (size_t i = 0; i < nodeList[currentASMNode].transitions.size(); i++)
		{
			const AnimationStateMachine_Transition& transition = nodeList[currentASMNode].transitions[i];
			bool transitionPasses = false;

			for (size_t j = 0; j < transition.transitionConditionGroups.size(); j++)
			{
				const AnimationStateMachine_TransitionConditionGroup& transitionConditionGroup = transition.transitionConditionGroups[j];
				bool transitionConditionGroupPasses = true;

				for (size_t k = 0; k < transitionConditionGroup.transitionConditions.size(); k++)
				{
					const AnimationStateMachine_TransitionCondition& transitionCondition = transitionConditionGroup.transitionConditions[k];
					switch (transitionCondition.comparisonOperator)
					{
					case AnimationStateMachine_TransitionCondition::ASMComparisonOperator::EQUAL:
						transitionConditionGroupPasses &= (variableValues[transitionCondition.variableIndex] == transitionCondition.compareToValue);
						break;

					case AnimationStateMachine_TransitionCondition::ASMComparisonOperator::NEQUAL:
						transitionConditionGroupPasses &= (variableValues[transitionCondition.variableIndex] != transitionCondition.compareToValue);
						break;

					case AnimationStateMachine_TransitionCondition::ASMComparisonOperator::LESSER:
						transitionConditionGroupPasses &= (variableValues[transitionCondition.variableIndex] < transitionCondition.compareToValue);
						break;

					case AnimationStateMachine_TransitionCondition::ASMComparisonOperator::GREATER:
						transitionConditionGroupPasses &= (variableValues[transitionCondition.variableIndex] > transitionCondition.compareToValue);
						break;

					case AnimationStateMachine_TransitionCondition::ASMComparisonOperator::LEQUAL:
						transitionConditionGroupPasses &= (variableValues[transitionCondition.variableIndex] <= transitionCondition.compareToValue);
						break;

					case AnimationStateMachine_TransitionCondition::ASMComparisonOperator::GEQUAL:
						transitionConditionGroupPasses &= (variableValues[transitionCondition.variableIndex] >= transitionCondition.compareToValue);
						break;
					
					default:
						transitionConditionGroupPasses &= false;
						std::cout << "ERROR: The comparison operator doesn't exist" << std::endl;
						break;
					}
				}

				if (transitionConditionGroupPasses)
				{
					transitionPasses = true;
					break;
				}
			}

			if (transitionPasses)
			{
				moveToStateMachineNode(transition.toNodeIndex);
				break;
			}
		}
	}

	//
	// Update the internal animator
	//
	if (nodeList[currentASMNode].isBlendTreeAnimation)
	{
		size_t variableIndex = nodeList[currentASMNode].blendTreeVariableIndex;
		animatorPtr->setBlendTreeVariable(variableNames[variableIndex], variableValues[variableIndex]);
	}
	animatorPtr->updateAnimation(deltaTime);
}


void AnimatorStateMachine::setVariable(const std::string& varName, float value)
{
	variableValues[variableNameIndexMap[varName]] = value;		// Hopefully this doesn't crash...
}


void AnimatorStateMachine::moveToStateMachineNode(size_t nodeIndex)
{
	currentASMNode = nodeIndex;

	const AnimationStateMachine_Node& node = nodeList[currentASMNode];
	if (node.isBlendTreeAnimation)
	{
		animatorPtr->playBlendTree({
				{ node.animationIndex,  node.animationBlendBorder1, variableNames[node.blendTreeVariableIndex] },
				{ node.animationIndex2, node.animationBlendBorder2 }
			},
			node.transitionTime,
			node.loopAnimation
		);
	}
	else
	{
		animatorPtr->playAnimation(node.animationIndex, node.transitionTime, node.loopAnimation);
	}
}
