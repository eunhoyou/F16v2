#pragma once
#include "../../behaviortree_cpp_v3/action_node.h"
#include "../../behaviortree_cpp_v3/bt_factory.h"
#include "../../../Geometry/Vector3.h"
#include "../Functions.h"
#include "../BlackBoard/CPPBlackBoard.h"

using namespace BT;

namespace Action
{
    // 히스테리시스를 적용한 BFM 모드 결정
    BFM_Mode DetermineBFMWithHysteresis(float distance, float aspectAngle, float angleOff, BFM_Mode currentBFM);
    
    // 긴급 상황에서의 즉시 모드 변경 허용 여부 판단
    bool IsEmergencyModeChange(float distance, float aspectAngle, BFM_Mode from, BFM_Mode to);

    class BFM_ModeDetermination : public SyncActionNode
    {
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