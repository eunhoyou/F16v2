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
    class BFM_ModeDetermination : public SyncActionNode
    {
    private:
        // WEZ 상수 (시뮬레이터 기준)
        static constexpr float WEZ_MIN_RANGE = 152.4f;  // 500 피트
        static constexpr float WEZ_MAX_RANGE = 914.4f;  // 3000 피트
        static constexpr float WEZ_MAX_ANGLE = 2.0f;    // ±2도

        // 턴 서클 관계 분류
        enum TurnCircleRelation
        {
            INSIDE_TARGET_CIRCLE,    // 적기 턴 서클 내부 (공격/방어 가능)
            OUTSIDE_TARGET_CIRCLE,   // 적기 턴 서클 외부 (정면 BFM)
            ON_CIRCLE_EDGE          // 턴 서클 경계 (상황에 따라 결정)
        };

        TurnCircleRelation DetermineTurnCircleRelation(CPPBlackBoard* BB);
        float EstimateTargetTurnRadius(CPPBlackBoard* BB);
        bool IsHeadOnSituation(float aspectAngle, float angleOff);
        bool IsInWEZ(float distance, float los);

    public:
        BFM_ModeDetermination(const std::string& name, const NodeConfiguration& config) : SyncActionNode(name, config)
        {
        }

        ~BFM_ModeDetermination()
        {
        }

        static PortsList providedPorts();
        NodeStatus tick() override;
    };
}