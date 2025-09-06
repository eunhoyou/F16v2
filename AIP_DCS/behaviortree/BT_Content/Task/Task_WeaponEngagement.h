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
    class Task_WeaponEngagement : public SyncActionNode
    {
    private:
        // WEZ 상수들 (시뮬레이터 규격)
        static constexpr float WEZ_MIN_RANGE = 152.4f;  // 500 피트 = 152.4 미터
        static constexpr float WEZ_MAX_RANGE = 914.4f;  // 3000 피트 = 914.4 미터
        static constexpr float WEZ_MAX_ANGLE = 2.0f;    // ±2도
        static constexpr float M_PI = 3.14159265359f;

        // 무기 교전 관련 함수들
        Vector3 CalculateGunTrackingPoint(CPPBlackBoard* BB);
        Vector3 CalculateWEZApproach(CPPBlackBoard* BB);
        Vector3 CalculateWEZEntry(CPPBlackBoard* BB);
        Vector3 CalculateInterceptCourse(CPPBlackBoard* BB);
        Vector3 ApplyGunLeadCorrection(Vector3 basePoint, CPPBlackBoard* BB);
        
        // 계산 헬퍼 함수들
        float CalculateTimeOfFlight(float distance);
        float CalculateEngagementThrottle(CPPBlackBoard* BB);
        
        // 상태 확인 함수들
        bool IsInWEZ(CPPBlackBoard* BB);
        bool IsApproachingWEZ(CPPBlackBoard* BB);

    public:
        Task_WeaponEngagement(const std::string& name, const NodeConfiguration& config) : SyncActionNode(name, config)
        {
        }

        ~Task_WeaponEngagement()
        {
        }

        static PortsList providedPorts();
        NodeStatus tick() override;
    };
}