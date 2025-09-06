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
        
        std::cout << "[BFM_DEBUG] Distance: " << distance 
                  << ", AspectAngle: " << aspectAngle 
                  << ", AngleOff: " << angleOff << std::endl;

        // 턴 서클 외부 (HABFM)
        if (distance > 2000.0f) 
        {
            (*BB)->BFM = HABFM;
            std::cout << "[BFM_DEBUG] Outside Turn Circle -> HABFM selected" << std::endl;
        }
        else // 턴 서클 내부
        {
            // OBFM
            if (aspectAngle < 60.0f)
            {
                (*BB)->BFM = OBFM;
                std::cout << "[BFM_DEBUG] Inside Turn Circle -> OBFM selected" << std::endl;
            }
            // DBFM
            else if (aspectAngle > 120.0f && angleOff < 60.0f)
            {
                (*BB)->BFM = DBFM;
                std::cout << "[BFM_DEBUG] Inside Turn Circle -> DBFM selected" << std::endl;
            }
            // SCISSORS: OBFM도 DBFM도 아닌, '옆으로 나란한' 상태
            else
            {
                (*BB)->BFM = SCISSORS;
                std::cout << "[BFM_DEBUG] Neutral/Alongside -> SCISSORS selected" << std::endl;
            }
        }
        
        return NodeStatus::SUCCESS;
    }
}