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

        // 교본 기반 위치 판단 함수들
        bool IsInsideTargetTurnCircle(CPPBlackBoard* BB);
        bool IsInWEZ(CPPBlackBoard* BB);
        bool IsApproachingWEZ(CPPBlackBoard* BB);

        // 교본 기반 BFM 단계별 함수들
        Vector3 CalculateEntryWindow(CPPBlackBoard* BB);     // 엔트리 윈도우 진입
        Vector3 CalculateLagPursuit(CPPBlackBoard* BB);      // 래그 추적 (3000피트까지)
        Vector3 CalculateLeadPursuit(CPPBlackBoard* BB);     // 리드 추적 (3000피트 이내)

        // 교본 기반 계산 헬퍼 함수들
        float CalculateCornerSpeed(CPPBlackBoard* BB);       // F-16 코너 속도
        float CalculateTurnRadius(float speed, float gLoad); // 선회반경 계산
        Vector3 CalculateInterceptPoint(CPPBlackBoard* BB);  // 요격점 계산
        float CalculateOptimalThrottle(CPPBlackBoard* BB, float targetSpeed);
        
        // WEZ 최적화 함수들
        Vector3 OptimizeWEZAngle(CPPBlackBoard* BB);         // WEZ 각도 최적화

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