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
        // 교본 기반 시저스 단계 정의
        enum ScissorsPhase
        {
            SCISSORS_NEUTRAL_SETUP,
            SCISSORS_ENERGY_FIGHT,
            SCISSORS_ESCAPE_OPPORTUNITY,
        };

        // 교본 기반 시저스 단계 판단
        ScissorsPhase DetermineScissorsPhase(CPPBlackBoard* BB);

        // 교본 기반 각 단계별 기동 계산
        Vector3 CalculateAggressiveScissors(CPPBlackBoard* BB);
        Vector3 CalculateDefensiveScissors(CPPBlackBoard* BB);
        Vector3 CalculateNeutralScissors(CPPBlackBoard* BB);
        Vector3 CalculateScissorsEscape(CPPBlackBoard* BB);

        // 교본 기반 에너지 관리 함수들
        float CalculateEnergyAdvantage(CPPBlackBoard* BB);
        float CalculateCornerSpeed(CPPBlackBoard* BB);
        float CalculateCornerSpeedThrottle(CPPBlackBoard* BB);

        // 교본 기반 상황 판단 함수들
        bool ShouldEscapeScissors(CPPBlackBoard* BB);

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