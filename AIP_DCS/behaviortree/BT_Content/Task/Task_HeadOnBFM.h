#pragma once

#include "../../behaviortree_cpp_v3/action_node.h"
#include "../../behaviortree_cpp_v3/bt_factory.h"
#include "../../../Geometry/Vector3.h"
#include "../Functions.h"
#include "../BlackBoard/CPPBlackBoard.h"
#include <algorithm>
#include <cmath>

using namespace BT;

namespace Action
{
    class Task_HeadOnBFM : public SyncActionNode
    {
    private:
        // 교차시 기동 옵션들
        Vector3 CalculateSliceTurn(CPPBlackBoard* BB);
        Vector3 CalculateLevelTurn(CPPBlackBoard* BB);
        Vector3 CalculateVerticalManeuver(CPPBlackBoard* BB);
        Vector3 CalculateLeadTurn(CPPBlackBoard* BB);
        Vector3 CalculateDisengagement(CPPBlackBoard* BB);

        // 판단 함수들
        bool ShouldInitiateLeadTurn(CPPBlackBoard* BB);
        bool ShouldDisengage(CPPBlackBoard* BB);

    public:
        Task_HeadOnBFM(const std::string& name, const NodeConfiguration& config) : SyncActionNode(name, config)
        {
        }

        ~Task_HeadOnBFM()
        {
        }

        static PortsList providedPorts();
        NodeStatus tick() override;
    };
}