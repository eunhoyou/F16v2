#include "SelectTarget.h"

namespace Action
{
	PortsList SelectTarget::providedPorts()
	{
		return {
			InputPort<CPPBlackBoard*>("BB")
		};
	}

	NodeStatus SelectTarget::tick()
	{
		Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");

		if((*BB)->Enemy.size() > 0)
		{
			(*BB)->ACM = EF;
			
			(*BB)->TargetLocaion_Cartesian = (*BB)->Enemy.at(0).Location;
			(*BB)->TargetRotation_EDegree = (*BB)->Enemy.at(0).Rotation;
			(*BB)->TargetSpeed_MS = (*BB)->Enemy.at(0).Speed;

		}
        
        return NodeStatus::SUCCESS;

	}

}