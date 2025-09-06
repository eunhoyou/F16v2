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
    class Task_Scissors : public SyncActionNode
    {
    private:
        enum ScissorsPhase
        {
            SCISSORS_ENTRY,
            SCISSORS_CROSSING,
            SCISSORS_REPOSITIONING,
            SCISSORS_ESCAPE_ATTEMPT
        };

        // 시저스 단계 판단
        ScissorsPhase DetermineScissorsPhase(CPPBlackBoard* BB);

        // 각 단계별 기동 계산
        Vector3 CalculateScissorsEntry(CPPBlackBoard* BB);
        Vector3 CalculateAggressiveScissors(CPPBlackBoard* BB);
        Vector3 CalculateDefensiveScissors(CPPBlackBoard* BB);
        Vector3 CalculateStandardScissors(CPPBlackBoard* BB);
        Vector3 CalculateRepositioning(CPPBlackBoard* BB);
        Vector3 CalculateScissorsEscape(CPPBlackBoard* BB);

        // 에너지 관리 함수들
        float CalculateEnergyAdvantage(CPPBlackBoard* BB);
        float CalculateEnergyConservingThrottle(float mySpeed, float altitude);
        float CalculateRepositioningThrottle(float mySpeed, float targetSpeed, float energyAdvantage);

    public:
        Task_Scissors(const std::string& name, const NodeConfiguration& config) : SyncActionNode(name, config)
        {
        }

        ~Task_Scissors()
        {
        }

        static PortsList providedPorts();
        NodeStatus tick() override;
    };
}