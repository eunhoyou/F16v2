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

        // 교본 기반 위협 수준 분류
        enum ThreatLevel
        {
            IMMEDIATE_WEZ_THREAT,   // WEZ 내 즉시 위험
            HIGH_THREAT,            // WEZ 접근 중
            MODERATE_THREAT,        // 일반 방어 상황
            LOW_THREAT             // 적기 실수 중
        };

        // 위협 평가
        ThreatLevel AssessThreatLevel(CPPBlackBoard* BB);

        // 교본 기반 방어 기동 함수들
        Vector3 CalculateEmergencyWEZEvasion(CPPBlackBoard* BB);     // WEZ 내 긴급 회피
        Vector3 CalculateLiftVectorDefense(CPPBlackBoard* BB);       // 양력벡터 방어
        Vector3 CalculateStandardDefensiveTurn(CPPBlackBoard* BB);   // 표준 방어 선회
        Vector3 CalculateCounterAttack(CPPBlackBoard* BB);           // 반격 기회 포착
        
        // 위협 판단 함수들
        bool IsInWEZ(float distance, float los);
        bool IsApproachingWEZ(float distance, float los);
        
        // 교본 기반 계산 함수들
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