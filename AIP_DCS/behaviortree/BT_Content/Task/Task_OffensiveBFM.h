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
        // WEZ 상수 정의 (시뮬레이터 기준)
        static constexpr float WEZ_MIN_RANGE = 152.4f;  // 500 피트 = 152.4 미터
        static constexpr float WEZ_MAX_RANGE = 914.4f;  // 3000 피트 = 914.4 미터
        static constexpr float WEZ_MAX_ANGLE = 2.0f;    // ±2도

        // 교본 기반 BFM 함수들
        Vector3 CalculateEntryWindow(CPPBlackBoard* BB);
        Vector3 CalculateLagPursuit(CPPBlackBoard* BB);
        Vector3 CalculateLeadPursuit(CPPBlackBoard* BB);
        Vector3 CalculateGunTracking(CPPBlackBoard* BB);

        // WEZ 및 계산 헬퍼 함수들
        bool IsInWEZ(CPPBlackBoard* BB);
        float CalculateLagDistance(float distance);
        float CalculateTimeOfFlight(float distance);
        float CalculateApproachThrottle(float mySpeed, float distance);
        float CalculateGunThrottle(float mySpeed);

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