#include "CPPBlackBoard.h"

CPPBlackBoard::CPPBlackBoard()
{
	RunningTime = 0;
	DeltaSecond = 0.005566170;

	MyLocation_Cartesian		= Vector3(0,0,0);
	TargetLocaion_Cartesian		= Vector3(0, 0, 0);
	VP_Cartesian				= Vector3(0, 0, 0);

	MyForwardVector = Vector3(0, 0, 0);
	MyUpVector		= Vector3(0, 0, 0);
	MyRightVector	= Vector3(0, 0, 0);

	TargetForwardVector = Vector3(0, 0, 0);
	TargetUpVector		= Vector3(0, 0, 0);
	TargetRightVector	= Vector3(0, 0, 0);

	MyRotation_EDegree		= EulerAngle(0,0,0);
	TargetRotation_EDegree	= EulerAngle(0, 0, 0);

	MySpeed_MS		= 0;
	TargetSpeed_MS	= 0;

	Distance = 0;
	Throttle = 0;

	Los_Degree = 0;
	Los_Degree_Target = 0;

	MyAngleOff_Degree = 0;
	MyAspectAngle_Degree = 0;

    MyCurrentNz = 0;          // 내 현재 수직 G-load (Nz)
    MyCurrentNy = 0;          // 내 현재 측면 G-load (Ny)
    TargetCurrentNz = 0;      // 적기 현재 수직 G-load
    TargetCurrentNy = 0;      // 적기 현재 측면 G-load
    MyTotalG = 0;             // sqrt(Nz² + Ny²)
    TargetTotalG = 0;         // 적기 총 G

	BFM = NONE;
	ACM = EF;
	
	Team = UNKNOWN;
}

CPPBlackBoard::~CPPBlackBoard()
{
}
