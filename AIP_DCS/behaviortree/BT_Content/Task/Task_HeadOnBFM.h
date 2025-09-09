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
        // 교본 기반 상수 정의
        static constexpr float M_PI = 3.14159265359f;
        
        // WEZ 상수 (시뮬레이터 특성)
        static constexpr float WEZ_MIN_RANGE = 152.4f;  // 500ft
        static constexpr float WEZ_MAX_RANGE = 914.4f;  // 3000ft
        static constexpr float WEZ_MAX_ANGLE = 2.0f;
        
        // 교본 기반 거리 임계값
        static constexpr float LONG_RANGE_THRESHOLD = 6000.0f;     // 6km
        static constexpr float MEDIUM_RANGE_THRESHOLD = 3000.0f;   // 3km  
        static constexpr float CLOSE_RANGE_THRESHOLD = 1500.0f;    // 1.5km
        
        // 에너지 관리 임계값
        static constexpr float ENERGY_ADVANTAGE_THRESHOLD = 0.2f;  // 20% 에너지 우위

        Vector3 CalculateOffsetApproach(CPPBlackBoard* BB);      // 오프셋 접근
        Vector3 CalculateDisengagement(CPPBlackBoard* BB);       // 이탈 기동
        
        float CalculateCornerSpeedThrottle(CPPBlackBoard* BB);  // 코너 속도 유지용 스로틀

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