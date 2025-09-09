#include "Task_WeaponEngagement.h"

namespace Action
{
    PortsList Task_WeaponEngagement::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus Task_WeaponEngagement::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");

        float distance = (*BB)->Distance;
        float los = (*BB)->Los_Degree;
        float aspectAngle = (*BB)->MyAspectAngle_Degree;
        float angleOff = (*BB)->MyAngleOff_Degree;
        
        std::cout << "[WeaponEngagement] Dist:" << distance 
                << " LOS:" << los 
                << " AA:" << aspectAngle << std::endl;

        Vector3 calculated_vp;
        float wez_throttle;

        if (distance < 1500.0f && aspectAngle > 90.0f)
        {
            std::cout << "[WeaponEngagement] ⚔️ MERGE - Aggressive Turn Fight!" << std::endl;
            calculated_vp = (*BB)->TargetLocaion_Cartesian;
        }
        // 🎯 WEZ 내: LOS 정확도 최우선
        if (IsInWEZ(BB.value())) {
            calculated_vp = CalculateHighPrecisionLOS(BB.value());
            wez_throttle = 0.2f;
            std::cout << "[WeaponEngagement] 🎯 WEZ - High Precision LOS" << std::endl;
        }
        // 📐 WEZ 근처: 각도 우선 보정 
        else if (IsNearWEZ(BB.value())) {
            calculated_vp = CalculateAnglePriorityCorrection(BB.value());
            wez_throttle = 0.4f;
            std::cout << "[WeaponEngagement] 📐 Near WEZ - Angle Priority" << std::endl;
        }
        // 🚀 WEZ 밖: 직진 접근
        else {
            calculated_vp = CalculateDirectApproach(BB.value());
            wez_throttle = 0.7f;
            std::cout << "[WeaponEngagement] 🚀 Direct Approach" << std::endl;
        }

        (*BB)->VP_Cartesian = calculated_vp;
        (*BB)->Throttle = wez_throttle;

        return NodeStatus::SUCCESS;
    }

    Vector3 Task_WeaponEngagement::CalculateHighPrecisionLOS(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        float distance = BB->Distance;
        
        if (distance <= WEZ_MAX_RANGE) {
            return targetLocation;
        }
        else {
            // WEZ 밖에서는 적기 뒤 최적 거리로
            Vector3 targetForward = BB->TargetForwardVector;
            targetForward.normalize();
            float optimalDistance = (WEZ_MIN_RANGE + WEZ_MAX_RANGE) * 0.5f;
            return targetLocation - targetForward * optimalDistance;
        }
    }

    // 📐 핵심 함수: 각도 우선 보정 (WEZ 근처)
    Vector3 Task_WeaponEngagement::CalculateAnglePriorityCorrection(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        targetForward.normalize();
        
        float los = BB->Los_Degree;
        float distance = BB->Distance;
        
        // 목표: WEZ 중간 거리에서 LOS 0도
        float targetDistance = (WEZ_MIN_RANGE + WEZ_MAX_RANGE) * 0.5f; // 533m
        Vector3 basePosition = targetLocation - targetForward * targetDistance;
        
        Vector3 adjustedPosition = BB->TargetLocaion_Cartesian;
        
        // 현재 조준점이 적기와 어긋나 있을 경우, 빠르게 각도를 보정
        if (std::abs(los) > 2.0f) {
            Vector3 myRight = BB->MyRightVector;
            myRight.normalize();
            
            // 거리의 5%만큼 강력하게 보정
            float superCorrection = los * distance * 0.05f;
            
            // 극단적인 각도는 보정 강도를 더욱 증가
            if (std::abs(los) > 45.0f) {
                superCorrection *= 2.0f;
            }
            if (std::abs(los) > 90.0f) {
                superCorrection *= 2.0f; 
            }
            
            // 계산된 보정 값을 적기 위치에 더하여 최종 VP 설정
            adjustedPosition += myRight * (los > 0 ? -superCorrection : superCorrection);
            
            std::cout << "[AnglePriorityCorrection] LOS:" << los 
                    << "° -> Super Correction applied to Target Location" << std::endl;
        }
        
        return adjustedPosition;
    }

    // 🚀 단순 함수: 직진 접근
    Vector3 Task_WeaponEngagement::CalculateDirectApproach(CPPBlackBoard* BB)
    {
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        targetForward.normalize();
        
        // 단순히 적기 뒤로 직진
        float approachDistance = WEZ_MAX_RANGE * 0.9f; // 823m
        Vector3 directPosition = targetLocation - targetForward * approachDistance;
        
        return directPosition;
    }

    // 🎯 개선된 WEZ 판정 (더 관대하게)
    bool Task_WeaponEngagement::IsInWEZ(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;

        bool rangeValid = (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE);
        
        return rangeValid;
    }

    // 📐 새로운 함수: WEZ 근처 판정
    bool Task_WeaponEngagement::IsNearWEZ(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;
        
        // WEZ 거리 범위의 1.5배, 각도 20도 이내
        bool nearRange = (distance <= WEZ_MAX_RANGE * 1.5f);
        
        return nearRange;
    }

    float Task_WeaponEngagement::CalculateWEZThrottle(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;
        
        if (distance < WEZ_MIN_RANGE * 1.2f) {
            return 0.2f; // 매우 낮은 스로틀
        } 
        else if (distance > WEZ_MAX_RANGE * 0.8f) {
            return 0.8f;
        } 
        else {
            if (std::abs(los) < 2.0f) {
                return 0.2f; // 정밀 제어
            } else {
                return 0.4f; // 보정 필요
            }
        }
    }
}