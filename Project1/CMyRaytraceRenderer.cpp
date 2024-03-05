#include "pch.h"
#include "CMyRaytraceRenderer.h"
#include "GrTexture.h"
#include "Poisson2D.h"

const double Epsilon = 1e-10;

CMyRaytraceRenderer::CMyRaytraceRenderer()
{
	m_window = NULL;
}

CMyRaytraceRenderer::~CMyRaytraceRenderer()
{
}

void CMyRaytraceRenderer::SetImage(BYTE** rayimage, int rayimagewidth, int rayimageheight)
{
	m_rayimage = rayimage;
	m_rayimagewidth = rayimagewidth;
	m_rayimageheight = rayimageheight;
}

void CMyRaytraceRenderer::SetWindow(CWnd* p_window)
{
	m_window = p_window;
}

bool CMyRaytraceRenderer::RendererStart()
{
	m_intersection.Initialize();
	m_mstack.clear();

	// We have to do all of the matrix work ourselves.
	// Set up the matrix stack.
	CGrTransform t;
	t.SetLookAt(Eye().X(), Eye().Y(), Eye().Z(),
		Center().X(), Center().Y(), Center().Z(),
		Up().X(), Up().Y(), Up().Z());

	m_mstack.push_back(t);

    int light_cnt = LightCnt();
    for (int i = 0; i < light_cnt; i++)
    {
        const CGrRenderer::Light& old_light = GetLight(i);
        CGrRenderer::Light new_light;
        new_light.m_pos = m_mstack.back() * old_light.m_pos;
        for (int c = 0; c < 4; c++)
        {
            new_light.m_ambient[c] = old_light.m_ambient[c];
        }
        for (int c = 0; c < 4; c++)
        {
            new_light.m_diffuse[c] = old_light.m_diffuse[c];
        }
        for (int c = 0; c < 4; c++)
        {
            new_light.m_specular[c] = old_light.m_specular[c];
        }
        m_lights.push_back(new_light);
    }

    m_material = NULL;

	return true;
}

void CMyRaytraceRenderer::RendererMaterial(CGrMaterial* p_material)
{
	m_material = p_material;
}

void CMyRaytraceRenderer::RendererPushMatrix()
{
    m_mstack.push_back(m_mstack.back());
}

void CMyRaytraceRenderer::RendererPopMatrix()
{
    m_mstack.pop_back();
}

void CMyRaytraceRenderer::RendererRotate(double angle, double x, double y, double z)
{
    CGrTransform r;
    r.SetRotate(angle, CGrPoint(x, y, z));
    m_mstack.back() *= r;
}

void CMyRaytraceRenderer::RendererTranslate(double x, double y, double z)
{
    CGrTransform r;
    r.SetTranslate(x, y, z);
    m_mstack.back() *= r;
}

//
// Name : CMyRaytraceRenderer::RendererEndPolygon()
// Description : End definition of a polygon. The superclass has
// already collected the polygon information
//

void CMyRaytraceRenderer::RendererEndPolygon()
{
    const std::list<CGrPoint>& vertices = PolyVertices();
    const std::list<CGrPoint>& normals = PolyNormals();
    const std::list<CGrPoint>& tvertices = PolyTexVertices();

    // Allocate a new polygon in the ray intersection system
    m_intersection.PolygonBegin();
    m_intersection.Material(m_material);

    if (PolyTexture())
    {
        m_intersection.Texture(PolyTexture());
    }

    std::list<CGrPoint>::const_iterator normal = normals.begin();
    std::list<CGrPoint>::const_iterator tvertex = tvertices.begin();

    for (std::list<CGrPoint>::const_iterator i = vertices.begin(); i != vertices.end(); i++)
    {
        if (normal != normals.end())
        {
            m_intersection.Normal(m_mstack.back() * *normal);
            normal++;
        }

        if (tvertex != tvertices.end())
        {
            m_intersection.TexVertex(*tvertex);
            tvertex++;
        }

        m_intersection.Vertex(m_mstack.back() * *i);
    }

    m_intersection.PolygonEnd();
}

bool CMyRaytraceRenderer::RendererEnd()
{
    m_intersection.LoadingComplete();

    double ymin = -tan(ProjectionAngle() / 2. * GR_DTOR);
    double yhit = -ymin * 2.;
    double xmin = ymin * ProjectionAspect();
    double xwid = -xmin * 2.;
    int recursionAmount = 4;
    int antialiascnt = 4;
    CPoisson2D poisson;
    poisson.SetMinDistance(1 / (2 * sqrt(antialiascnt)));

    double fogDensity = 0.0002;

    // raytracing loop will go here
    for (int r = 0; r < m_rayimageheight; r++)
    {
        for (int c = 0; c < m_rayimagewidth; c++)
        {
            double fogFactor = 0.0f;

            // reset possion distribution for each new set
            poisson.Reset();
            CGrPoint finalColor = CGrPoint();

            // Shoot an amount of rays equal to anti alias count in a single pixel
            for (int a = 0; a < antialiascnt; a++)
            {
                CGrPoint p = poisson.Generate();
                double x = xmin + (c + p[0]) / m_rayimagewidth * xwid;
                double y = ymin + (r + p[1]) / m_rayimageheight * yhit;

                // Construct a Ray
                CRay ray(CGrPoint(0, 0, 0), Normalize3(CGrPoint(x, y, -1, 0)));

                double t;                                   // Will be distance to intersection
                CGrPoint intersect;                         // Will by x,y,z location of intersection
                const CRayIntersection::Object* nearest;    // Pointer to intersecting object

                if (m_intersection.Intersect(ray, 1e20, NULL, nearest, t, intersect))
                {
                    // We hit something...
                    // Determine information about the intersection
                    CGrPoint N;
                    CGrMaterial* material;
                    CGrTexture* texture;
                    CGrPoint texcoord;

                    m_intersection.IntersectInfo(ray, nearest, t,
                        N, material, texture, texcoord);

                    // calculate fog lerp value from distance
                    fogFactor = 1.f / (pow(2, fogDensity * t * t));

                    if (material != NULL)
                    {
                        CGrPoint textureColor;
                        if (texture != NULL)
                        {

                            textureColor = TextureColor(texture, texcoord);
                        }
                        else
                        {
                            textureColor = CGrPoint(1.0, 1.0, 1.0);
                        }
                        finalColor += CalculateColor(intersect, Normalize3(N), Normalize3(-intersect), textureColor, material, nearest, t, 4);
                    }
                }
            }

            for (int c = 0; c < 3; c++)
            {
                finalColor[c] = min(1.0, finalColor[c] / antialiascnt);
            }
            
            // apply fog
            finalColor = finalColor * fogFactor + CGrPoint(0.9f, 0.9f, 0.9f) * (1 - fogFactor);

            m_rayimage[r][c * 3] = BYTE(finalColor[0] * 255);
            m_rayimage[r][c * 3 + 1] = BYTE(finalColor[1] * 255);
            m_rayimage[r][c * 3 + 2] = BYTE(finalColor[2] * 255);
        }
        if ((r % 50) == 0)
        {
            m_window->Invalidate();
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                DispatchMessage(&msg);
        }
    }
    return true;
}

bool CMyRaytraceRenderer::IsShadowed(CGrPoint intersection, CGrPoint directionToLight, const CRayIntersection::Object* toIgnore, double distanceToLight)
{
    double t;                                   // Will be distance to intersection
    CGrPoint intersect;                         // Will by x,y,z location of intersection
    const CRayIntersection::Object* nearest;    // Pointer to intersecting object
    CRay ray(intersection, directionToLight);
    if (m_intersection.Intersect(ray, distanceToLight, toIgnore, nearest, t, intersect))
    {
        if (t > Epsilon)
        {
            return true;
        }
    }
    return false;
}

CGrPoint CMyRaytraceRenderer::CalculateColor(CGrPoint intersection, CGrPoint surfaceNormal, CGrPoint viewDirection, CGrPoint textureColor, CGrMaterial* material, const CRayIntersection::Object* toIgnore, double & t, int recurse)
{
    CGrPoint hallColor = CGrPoint(0.0, 0.0, 0.0);
    int numLights = static_cast<int>(m_lights.size());
    for (int i = 0; i < numLights; i++)
    {
        for (int c = 0; c < 3; c++)
        {
            hallColor[c] += (m_lights[i].m_ambient[c] * material->Ambient(c)) / numLights;
        }
    }


    for (int i = 0; i < numLights; i++)
    {
        CGrPoint lightDirection;
        if (m_lights[i].m_pos[3] == 0)
        {
            lightDirection = Normalize3(m_lights[i].m_pos);
        }
        else
        {
            lightDirection = Normalize3((m_lights[i].m_pos - intersection));
        }

        if (Dot3(surfaceNormal, lightDirection) < 0)
        {
            continue;
        }

        if (IsShadowed(intersection, lightDirection, toIgnore, Distance(intersection, m_lights[i].m_pos)))
            continue;

        for (int c = 0; c < 3; c++)
        {
            hallColor[c] += m_lights[i].m_diffuse[c] * material->Diffuse(c) * Dot3(surfaceNormal, lightDirection);
        }

        CGrPoint halfDirection = Normalize3(lightDirection + viewDirection);
        float sif = pow(Dot3(surfaceNormal, halfDirection), material->Shininess());
        for (int c = 0; c < 3; c++)
        {
            hallColor[c] += m_lights[i].m_specular[c] * material->Specular(c) * sif;
        }

    }

    if (recurse > 1 && (material->SpecularOther(0) > 0 || material->SpecularOther(1) > 0 || material->SpecularOther(2) > 0))
    {
        CGrPoint R = surfaceNormal * 2 * Dot3(surfaceNormal, viewDirection) - viewDirection;
        CRay reflectionRay(intersection, R);
        CGrPoint rayColor = CGrPoint(0.0, 0.0, 0.0);
        RayColor(reflectionRay, rayColor, recurse - 1, toIgnore, t);
        for (int c = 0; c < 3; c++)
        {
            double value_of_refraction = 1;
            double T = 1; //(Normalize3(N) * (value_of_refraction * Dot3(Normalize3(N), Normalize3(V)) + sqrt(1 - value_of_refraction * value_of_refraction * (1 - Dot3(Normalize3(N), Normalize3(V)) * Dot3(Normalize3(N), Normalize3(V))))) - Normalize3(V) * value_of_refraction).Length3();
            hallColor[c] += material->SpecularOther(c) * rayColor[c] * pow(T, t);
        }
    }

    for (int c = 0; c < 3; c++)
    {
        hallColor[c] = hallColor[c] * textureColor[c] / numLights;
    }

    return hallColor;
}

void CMyRaytraceRenderer::RayColor(const CRay& p_ray, CGrPoint& p_color, int p_recurse, const CRayIntersection::Object* p_ignore, double& t)
{
    double distance;                            // Will be distance to intersection
    CGrPoint intersect;                         // Will by x,y,z location of intersection
    const CRayIntersection::Object* nearest;    // Pointer to intersecting object
    if (m_intersection.Intersect(p_ray, 1e20, p_ignore, nearest, distance, intersect))
    {
        // We hit something...
        // Determine information about the intersection
        CGrPoint N;
        CGrMaterial* material;
        CGrTexture* texture;
        CGrPoint texcoord;

        m_intersection.IntersectInfo(p_ray, nearest, distance, N, material, texture, texcoord);

        if (material != NULL) 
        {
            CGrPoint textureColor;
            if (texture != NULL)
            {
                textureColor = TextureColor(texture, texcoord);
            }
            else
            {
                textureColor = CGrPoint(1.0, 1.0, 1.0);
            }
            t += distance;
            p_color = CalculateColor(intersect, N, -intersect, textureColor, material, nearest, t, p_recurse);
        }
    }
}

CGrPoint CMyRaytraceRenderer::TextureColor(CGrTexture* texture, CGrPoint texcoord)
{
    int texWidth = texture->Width();
    int texHeight = texture->Height();

    double xd = texWidth * texcoord.X();
    int xi = (int)xd;

    double yd = texHeight * texcoord.Y();
    int yi = (int)yd;

    double fx = fmod(xd, 1);
    double fy = fmod(yd, 1);
    BYTE* color = new BYTE[3];
    for (int i = 0; i < 3; i++)
    {
        BYTE ct = texture->Row((yi + 1) % texHeight)[3 * (xi % texWidth) + i] * (1. - fx)
            + texture->Row((yi + 1) % texHeight)[3 * ((xi + 1) % texWidth) + i] * fx;

        BYTE cb = texture->Row(yi % texHeight)[3 * (xi % texWidth) + i] * (1. - fx)
            + texture->Row(yi % texHeight)[3 * ((xi + 1) % texWidth) + i] * fx;

        color[i] = ct * fy + cb * (1. - fy);
    }


    // We must convert the color into the range of 0-1
    CGrPoint converted_color = CGrPoint(0.0,0.0,0.0);
    for (int c = 0; c < 3; c++)
    {
        converted_color[c] = color[c] / 255.0;
    }
    delete[]color;

    return converted_color;
}