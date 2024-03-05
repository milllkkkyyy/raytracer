#pragma once
#include "graphics/GrPoint.h"
class CLight
{
public:
	CLight(CGrPoint direction, double intensity, CGrPoint position) { m_direction = direction; m_intensity = intensity; m_position = position; }
	CGrPoint m_direction;
	CGrPoint m_position;
	double m_intensity;
};

