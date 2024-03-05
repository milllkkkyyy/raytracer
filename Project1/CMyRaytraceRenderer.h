#pragma once
#include <list>
#include "graphics/GrRenderer.h"
#include "graphics/RayIntersection.h"
#include "graphics/GrTransform.h""
#include "Light.h"

class CMyRaytraceRenderer :
    public CGrRenderer
{
private:
    int m_rayimagewidth;
    int m_rayimageheight;
    BYTE** m_rayimage;
    CWnd* m_window;
    CRayIntersection m_intersection;
    std::list<CGrTransform> m_mstack;
    CGrMaterial* m_material;
    std::vector<CGrRenderer::Light> m_lights;

public:
    CMyRaytraceRenderer();
    virtual ~CMyRaytraceRenderer();

    void SetImage(BYTE** rayimage, int rayimagewidth, int rayimageheight);
    void SetWindow(CWnd* p_window);
    bool RendererStart();
    void RendererMaterial(CGrMaterial* p_material);
    void RendererPushMatrix();
    void RendererPopMatrix();
    void RendererRotate(double angle, double x, double y, double z);
    void RendererTranslate(double x, double y, double z);
    void RendererEndPolygon();
    bool RendererEnd();
    bool IsShadowed(CGrPoint intersection, CGrPoint lightDirection, const CRayIntersection::Object* toIgnore, double distanceToLight);
    CGrPoint CalculateColor(CGrPoint intersection, CGrPoint surfaceNormal, CGrPoint viewDirection, CGrPoint textureColor, CGrMaterial* material, const CRayIntersection::Object* toIgnore, double& t, int recurse);
    void RayColor(const CRay& p_ray, CGrPoint& p_color, int p_recurse, const CRayIntersection::Object* p_ignore, double& t);
    CGrPoint TextureColor(CGrTexture* texture, CGrPoint texcoord);
};

