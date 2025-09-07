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
            SCISSORS_NEUTRAL_SETUP,      // 중립 셋업: 두 비행기가 나란한 위치
            SCISSORS_ENERGY_FIGHT,       // 에너지 싸움: 전방 이동 속도 경쟁
            SCISSORS_RATE_FIGHT,         // 선회율 싸움: Rate Kills 단계
            SCISSORS_ESCAPE_OPPORTUNITY, // 이탈 기회: 에너지 우위로 벗어나기
            SCISSORS_EMERGENCY_SEPARATION // 응급 분리: 충돌 방지
        };

        // WEZ 상수 (시뮬레이터 특성)
        static constexpr float WEZ_MIN_RANGE = 152.4f;  // 500 피트
        static constexpr float WEZ_MAX_RANGE = 914.4f;  // 3000 피트
        static constexpr float WEZ_MAX_ANGLE = 2.0f;    // ±2도

        // 교본 기반 거리 임계값
        static constexpr float COLLISION_AVOIDANCE_DISTANCE = 300.0f;  // 충돌 회피
        static constexpr float RATE_FIGHT_DISTANCE = 800.0f;          // 선회율 싸움
        static constexpr float ENERGY_FIGHT_DISTANCE = 1500.0f;       // 에너지 싸움
        
        // 에너지 관리 임계값
        static constexpr float ENERGY_ADVANTAGE_THRESHOLD = 0.25f;    // 25% 에너지 우위

        // 교본 기반 시저스 단계 판단
        ScissorsPhase DetermineScissorsPhase(CPPBlackBoard* BB);

        // 교본 기반 각 단계별 기동 계산
        Vector3 CalculateNeutralSetup(CPPBlackBoard* BB);         // 중립 위치 설정
        Vector3 CalculateAggressiveScissors(CPPBlackBoard* BB);   // 공격적 시저스 (에너지 우위)
        Vector3 CalculateDefensiveScissors(CPPBlackBoard* BB);    // 방어적 시저스 (에너지 열세)
        Vector3 CalculateNeutralScissors(CPPBlackBoard* BB);      // 중립 시저스 (균등 에너지)
        Vector3 CalculateRateFight(CPPBlackBoard* BB);            // Rate Fight (최대 선회율)
        Vector3 CalculateScissorsEscape(CPPBlackBoard* BB);       // 시저스 이탈 기동
        Vector3 CalculateEmergencySeparation(CPPBlackBoard* BB);  // 응급 분리 기동

        // 교본 기반 에너지 관리 함수들
        float CalculateEnergyAdvantage(CPPBlackBoard* BB);        // 총 에너지 우위 계산
        float CalculateCornerSpeed(CPPBlackBoard* BB);            // F-16 코너 속도
        float CalculateTurnRadius(float speed, float gLoad);      // 교본 공식 적용
        float CalculateEnergyConservingThrottle(float mySpeed);   // 에너지 보존 스로틀
        float CalculateCornerSpeedThrottle(CPPBlackBoard* BB);    // 코너 속도 유지 스로틀

        // 교본 기반 상황 판단 함수들
        bool ShouldEscapeScissors(CPPBlackBoard* BB);             // 시저스 이탈 조건
        bool IsInWEZ(float distance, float los);                 // WEZ 내부 확인

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