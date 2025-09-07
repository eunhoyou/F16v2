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
        static constexpr float MIN_SAFE_ALTITUDE = 1000.0;
        static constexpr float CRITICAL_ALTITUDE = 800.0;
        static constexpr float EMERGENCY_ALTITUDE = 600.0;
        
        Vector3 CalculateEmergencyClimb(CPPBlackBoard* BB);
        bool IsAltitudeCritical(float currentAltitude);
        bool IsAltitudeEmergency(float currentAltitude);

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