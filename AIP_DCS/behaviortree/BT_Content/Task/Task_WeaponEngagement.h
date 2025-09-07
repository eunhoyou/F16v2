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

        // 시뮬레이터 특성에 맞춘 무기 교전 함수들
        Vector3 CalculateAiming(CPPBlackBoard* BB);               // WEZ 내 조준
        Vector3 CalculateWEZEntry(CPPBlackBoard* BB);             // WEZ 진입 경로
        Vector3 CalculateRapidApproach(CPPBlackBoard* BB);        // 신속 접근
        Vector3 CalculateInterceptCourse(CPPBlackBoard* BB);      // 요격 코스
        
        // 스로틀 제어
        float CalculateWEZThrottle(CPPBlackBoard* BB);
        
        // 상태 확인 함수들
        bool IsInWEZ(CPPBlackBoard* BB);
        bool IsWEZApproachable(CPPBlackBoard* BB);

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