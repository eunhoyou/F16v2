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