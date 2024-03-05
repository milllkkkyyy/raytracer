#pragma once
#include <set>
#include <GrPoint.h>

class CPoisson2D
{
private:
	std::set<CGrPoint> m_history;
	double m_minimumDistance;
	double m_maxY;
	double m_maxX;

public:
	CPoisson2D();
	void Reset();
	void SetMinDistance(double distance);
	void SetRandomMax(double maxX, double maxY);
	CGrPoint Generate();
};

