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

        // 교본 기반 거리별 전술 기동 함수들
        Vector3 CalculateTacticalApproach(CPPBlackBoard* BB);    // 원거리: 측면 접근
        Vector3 CalculateLeadTurn(CPPBlackBoard* BB);            // 중거리: 리드 턴 실행
        Vector3 CalculateAggressiveSlice(CPPBlackBoard* BB);     // 에너지 우위: 8G 슬라이스
        Vector3 CalculateStandardSlice(CPPBlackBoard* BB);       // 표준: 7G 슬라이스
        Vector3 CalculateVerticalManeuver(CPPBlackBoard* BB);    // 에너지 열세: 수직 기동
        Vector3 CalculateOffsetApproach(CPPBlackBoard* BB);      // 오프셋 접근
        Vector3 CalculateBFMTransition(CPPBlackBoard* BB);       // BFM 전환 준비
        Vector3 CalculateDisengagement(CPPBlackBoard* BB);       // 이탈 기동

        // 교본 기반 상황 판단 함수들
        bool ShouldInitiateLeadTurn(CPPBlackBoard* BB);         // 리드 턴 실행 조건
        bool ShouldDisengage(CPPBlackBoard* BB);                // 이탈 윈도우 판단

        // 교본 기반 에너지 및 성능 계산
        float CalculateEnergyState(CPPBlackBoard* BB);          // 총 에너지 상태 비교
        float CalculateCornerSpeed(CPPBlackBoard* BB);          // F-16 코너 속도 계산
        float CalculateTurnRadius(float speed, float gLoad);    // 교본 공식: TR = V²/(g*G)
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