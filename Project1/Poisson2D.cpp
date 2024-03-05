#include "pch.h"
#include "Poisson2D.h"

/// <summary>
/// Default constructor
/// </summary>
CPoisson2D::CPoisson2D()
{
	m_minimumDistance = -99999999;
	m_maxX = 1.0;
	m_maxY = 1.0;
}

/// <summary>
/// For each set of numbers you must clear the last set before continuing
/// </summary>
void CPoisson2D::Reset()
{
	m_history.clear();
}

/// <summary>
/// Set the minimum distance points can be between
/// </summary>
/// <param name="distance">minimum distance between two random numbers</param>
void CPoisson2D::SetMinDistance(double distance)
{
	m_minimumDistance = distance;
}

/// <summary>
/// Set the maximum possible random value
/// </summary>
/// <param name="maxX"></param>
/// <param name="maxY"></param>
void CPoisson2D::SetRandomMax(double maxX, double maxY)
{
	m_maxX = maxX;
	m_maxY = maxY;
}

/// <summary>
/// Generate a Poisson Disk Sample
/// </summary>
/// <returns>A Random Point Sample Between 0 and 1</returns>
CGrPoint CPoisson2D::Generate()
{
	CGrPoint p;
	bool valid;
	do
	{
		valid = true;
		p = CGrPoint(m_maxX * ((double)rand() / (double)RAND_MAX), m_maxY * ((double)rand() / (double)RAND_MAX));
		for (const auto& item : m_history)
		{
			if (Distance(item, p) < m_minimumDistance)
			{
				valid = false;
			}
		}
	} while (!valid);
	m_history.insert(p);
	return p;
}


