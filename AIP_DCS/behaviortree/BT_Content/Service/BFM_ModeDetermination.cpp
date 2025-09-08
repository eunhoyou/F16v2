#include "BFM_ModeDetermination.h"

namespace Action
{
    PortsList BFM_ModeDetermination::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus BFM_ModeDetermination::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");

        float distance = (*BB)->Distance;
        float aspectAngle = (*BB)->MyAspectAngle_Degree;
        float angleOff = (*BB)->MyAngleOff_Degree;
        float los = (*BB)->Los_Degree;
        
        BFM_Mode previousBFM = (*BB)->BFM;
        
        std::cout << "[BFM_ModeDetermination] Distance: " << distance 
                  << ", AspectAngle: " << aspectAngle 
                  << ", AngleOff: " << angleOff 
                  << ", LOS: " << los << std::endl;

        // 교본 기준 2nm(3704m) 이상은 턴 서클 바깥
        if (distance >= 3704.0f) 
        {
            if (angleOff > 120.0f) {
                (*BB)->BFM = HABFM;
            }
            else if (aspectAngle < 45.0f) {
                (*BB)->BFM = DBFM;
            }
            else {
                (*BB)->BFM = HABFM;
            }
        }
        else 
        {
            if (aspectAngle < 45.0f && angleOff < 60.0f) {
                (*BB)->BFM = DBFM;
            }
            else if (aspectAngle > 135.0f && angleOff < 60.0f) {
                (*BB)->BFM = OBFM;
            }
            else if (aspectAngle > 60.0f && aspectAngle < 120.0f) {
                (*BB)->BFM = SCISSORS;
            }
            else {
                (*BB)->BFM = HABFM;
            }
        }
        
        return NodeStatus::SUCCESS;
    }
}