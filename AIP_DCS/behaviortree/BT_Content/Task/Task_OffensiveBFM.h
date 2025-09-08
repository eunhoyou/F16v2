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
    class Task_OffensiveBFM : public SyncActionNode
    {
    private:
        static constexpr float WEZ_MIN_RANGE = 152.4f;  // 500 피트 = 152.4 미터
        static constexpr float WEZ_MAX_RANGE = 914.4f;  // 3000 피트 = 914.4 미터
        static constexpr float WEZ_MAX_ANGLE = 2.0f;    // ±2도

        Vector3 CalculateEntryWindow(CPPBlackBoard* BB);
        Vector3 CalculateLagPursuit(CPPBlackBoard* BB);
        Vector3 CalculatePurePursuit(CPPBlackBoard* BB);

        float CalculateCornerSpeed(CPPBlackBoard* BB);
        float CalculateOptimalThrottle(CPPBlackBoard* BB);

    public:
        Task_OffensiveBFM(const std::string& name, const NodeConfiguration& config) : SyncActionNode(name, config)
        {
        }

        ~Task_OffensiveBFM()
        {
        }

        static PortsList providedPorts();
        NodeStatus tick() override;
    };
}