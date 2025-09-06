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
        static constexpr float M_PI = 3.14159265359f;

        // 거리별 전술 기동 함수들
        Vector3 CalculateTacticalApproach(CPPBlackBoard* BB);    // 원거리 측면 접근
        Vector3 CalculateOffsetApproach(CPPBlackBoard* BB);      // 중거리 오프셋 접근
        Vector3 CalculateAggressiveSlice(CPPBlackBoard* BB);     // 에너지 우위시 공격적 슬라이스
        Vector3 CalculateDefensiveManeuver(CPPBlackBoard* BB);   // 에너지 열세시 방어 기동
        Vector3 CalculateStandardManeuver(CPPBlackBoard* BB);    // 표준 기동
        Vector3 CalculateBFMTransition(CPPBlackBoard* BB);       // BFM 전환 준비

        // 기존 기동 함수들 (개선됨)
        Vector3 CalculateLeadTurn(CPPBlackBoard* BB);
        Vector3 CalculateDisengagement(CPPBlackBoard* BB);

        // 상황 판단 함수들
        bool ShouldInitiateLeadTurn(CPPBlackBoard* BB);
        bool ShouldDisengage(CPPBlackBoard* BB);
        
        // 에너지 관리 함수
        float CalculateEnergyState(CPPBlackBoard* BB);

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