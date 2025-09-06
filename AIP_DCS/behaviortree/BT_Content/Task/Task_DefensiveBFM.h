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
    class Task_DefensiveBFM : public SyncActionNode
    {
    private:
        // WEZ 상수들 (시뮬레이터 기준)
        static constexpr float WEZ_MIN_RANGE = 152.4f;  // 500 피트
        static constexpr float WEZ_MAX_RANGE = 914.4f;  // 3000 피트
        static constexpr float WEZ_MAX_ANGLE = 2.0f;    // ±2도

        // 방어 기동 함수들
        Vector3 CalculateEmergencyEvasion(CPPBlackBoard* BB);    // WEZ 내 긴급 회피
        Vector3 CalculateAggressiveDefense(CPPBlackBoard* BB);   // WEZ 접근 시 적극적 방어
        Vector3 CalculateDefensiveTurn(CPPBlackBoard* BB);       // 일반 방어 선회
        
        // 위협 판단 함수들
        bool IsInWEZ(float distance, float los);
        bool IsApproachingWEZ(float distance, float los);
        
        // 계산 헬퍼 함수들
        float CalculateCornerSpeed(CPPBlackBoard* BB);
        float CalculateTurnRadius(float speed, float gLoad);
        float CalculateDefensiveThrottle(float mySpeed);

    public:
        Task_DefensiveBFM(const std::string& name, const NodeConfiguration& config) : SyncActionNode(name, config)
        {
        }

        ~Task_DefensiveBFM()
        {
        }

        static PortsList providedPorts();
        NodeStatus tick() override;
    };
}