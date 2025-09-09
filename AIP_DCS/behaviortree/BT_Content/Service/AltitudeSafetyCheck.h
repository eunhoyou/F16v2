#pragma once
#include "../../behaviortree_cpp_v3/action_node.h"
#include "../../behaviortree_cpp_v3/bt_factory.h"
#include "../../../Geometry/Vector3.h"
#include "../Functions.h"
#include "../BlackBoard/CPPBlackBoard.h"

using namespace BT;

namespace Action
{
    class AltitudeSafetyCheck : public SyncActionNode
    {
    private:
        static constexpr float SAFE_ALTITUDE = 2000.0;
        static constexpr float CRITICAL_ALTITUDE = 1500.0;
        static constexpr float EMERGENCY_ALTITUDE = 1000.0;

    public:
        AltitudeSafetyCheck(const std::string& name, const NodeConfiguration& config) : SyncActionNode(name, config)
        {
        }

        ~AltitudeSafetyCheck()
        {
        }

        static PortsList providedPorts();
        NodeStatus tick() override;
    };
}