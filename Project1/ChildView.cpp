
// ChildView.cpp : implementation of the CChildView class
//

#include "pch.h"
#include "framework.h"
#include "Project1.h"
#include "ChildView.h"
#include "graphics/OpenGLRenderer.h"
#include "CMyRaytraceRenderer.h"
#include "GrTexture.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
{
    /*
    Need: 
        Objects: Shape not cube, Textured Box, Red Box, Reflective Black Large Floor
        Lights: 2 Points Lights With Different Colors, 1 Directional Light    
    */
	m_camera.Set(20, 10, 50, 0, 0, 0, 0, 1, 0);

    CGrPtr<CGrComposite> scene = new CGrComposite;
    m_scene = scene;

    // A red box
    CGrPtr<CGrMaterial> redpaint = new CGrMaterial;
    redpaint->AmbientAndDiffuse(0.8f, 0.0f, 0.0f);
    redpaint->Specular(0.0f, 0.4f, 0.0f);
    redpaint->Shininess(8);
    scene->Child(redpaint);

    CGrPtr<CGrComposite> redbox = new CGrComposite;
    redpaint->Child(redbox);
    redbox->Box(-2.5, -3, -10, 5, 5, 5);

    // A white box
    CGrPtr<CGrMaterial> whitepaint = new CGrMaterial;
    whitepaint->AmbientAndDiffuse(0.2f, 0.2f, 0.2f);
    whitepaint->SpecularOther(1.0f, 1.0f, 1.0f);
    whitepaint->Shininess(16);
    scene->Child(whitepaint);

    CGrPtr<CGrComposite> whitebox = new CGrComposite;
    whitepaint->Child(whitebox);
    whitebox->Box(-500, -5, -500, 1000, 1, 1000);



    // A box with a texture
    CGrPtr<CGrTexture> tex = new CGrTexture;
    tex->LoadFile(L"textures/marble03.bmp");
    CGrPtr<CGrComposite> texbox = new CGrComposite;
    scene->Child(texbox);
    texbox->Box(7, 5, 15, 5, 5, 5, tex);

    // fish
    CGrPtr<CGrTexture> tex2 = new CGrTexture;
    tex2->LoadFile(L"models/BLUEGILL.bmp");

    CGrPtr<CGrMaterial> fishMat = new CGrMaterial;
    fishMat->AmbientAndDiffuse(0.8f, 0.8f, 0.8f);
    fishMat->Specular(0.0f, 0.2f, 0.2f);
    scene->Child(fishMat);

    fish.LoadOBJ("models\\fish4.obj");
    CGrPtr<CGrComposite> c = new CGrComposite;
    fish.Generate(c, tex2);

    CGrPtr<CGrRotate> rotate = new CGrRotate;
    rotate->Child(c);
    rotate->Angle(-90);

    fishMat->Child(rotate);

    m_raytrace = false;
    m_rayimage = NULL;
}

CChildView::~CChildView()
{
}


BEGIN_MESSAGE_MAP(CChildView, COpenGLWnd)
	ON_WM_PAINT()
    ON_COMMAND(ID_RENDER_RAYTRACE, &CChildView::OnRenderRaytrace)
    ON_UPDATE_COMMAND_UI(ID_RENDER_RAYTRACE, &CChildView::OnUpdateRenderRaytrace)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()



// CChildView message handlers

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!COpenGLWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), nullptr);

	return TRUE;
}

//
// Name :         CChildView::ConfigureRenderer()
// Description :  Configures our renderer so it is able to render the scene.
//                Indicates how we'll do our projection, where the camera is,
//                and where any lights are located.
//

void CChildView::ConfigureRenderer(CGrRenderer* p_renderer)
{
    // Determine the screen size so we can determine the aspect ratio
    int width, height;
    GetSize(width, height);
    double aspectratio = double(width) / double(height);

    //
    // Set up the camera in the renderer
    //

    p_renderer->Perspective(m_camera.FieldOfView(),
        aspectratio, // The aspect ratio.
        20., // Near clipping
        1000.); // Far clipping

    // m_camera.FieldOfView is the vertical field of view in degrees.

    //
    // Set the camera location
    //

    p_renderer->LookAt(m_camera.Eye()[0], m_camera.Eye()[1], m_camera.Eye()[2],
        m_camera.Center()[0], m_camera.Center()[1], m_camera.Center()[2],
        m_camera.Up()[0], m_camera.Up()[1], m_camera.Up()[2]);

    //
    // Set the light locations and colors
    //

    float dimd = 0.5f;
    GLfloat dim[] = { dimd, dimd, dimd, 1.0f };
    GLfloat brightwhite[] = { 1.f, 1.f, 1.f, 1.0f };
    GLfloat brightBlue[] = { 0.f, 1.f, 0.f, 1.0f };
    GLfloat brightRed[] = { 1.f, 0.f, 0.f, 1.0f };


    p_renderer->AddLight(CGrPoint(-10, 10, 0, 1),
        dim, brightwhite, brightwhite);

    p_renderer->AddLight(CGrPoint(10, 10, 0, 1),
        dim, brightBlue, brightBlue);

    p_renderer->AddLight(CGrPoint(-5, 0, 5, 0),
        dim, brightRed, brightRed);
}

void CChildView::OnGLDraw(CDC* pDC)
{
    
    if (m_raytrace)
    {
        // Clear the color buffer
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set up for parallel projection
        int width, height;
        GetSize(width, height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, 0, height, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // If we got it, draw it
        if (m_rayimage)
        {
            glRasterPos3i(0, 0, 0);
            glDrawPixels(m_rayimagewidth, m_rayimageheight,
                GL_RGB, GL_UNSIGNED_BYTE, m_rayimage[0]);
        }

        glFlush();
    }
    else
    {
        //
        // Instantiate a renderer
        //

        COpenGLRenderer renderer;

        // Configure the renderer
        ConfigureRenderer(&renderer);

        //
        // Render the scene
        //

        renderer.Render(m_scene);
    }

}

void CChildView::OnRenderRaytrace()
{
    // TODO: Add your command handler code here
    m_raytrace = !m_raytrace;
    Invalidate();
    if (!m_raytrace)
    {
        delete[] m_rayimage[0];
        delete[] m_rayimage;
        return;
    }

    GetSize(m_rayimagewidth, m_rayimageheight);
    m_rayimage = new BYTE * [m_rayimageheight];

    int rowwid = m_rayimagewidth * 3;
    while (rowwid % 4)
        rowwid++;

    m_rayimage[0] = new BYTE[m_rayimageheight * rowwid];

    for (int i = 1; i < m_rayimageheight; i++)
    {
        m_rayimage[i] = m_rayimage[0] + i * rowwid;
    }

    for (int i = 0; i < m_rayimageheight; i++)
    {
        // Fill the image with blue
        for (int j = 0; j < m_rayimagewidth; j++)
        {
            m_rayimage[i][j * 3] = 0;               // red
            m_rayimage[i][j * 3 + 1] = 0;           // green
            m_rayimage[i][j * 3 + 2] = BYTE(255);   // blue
        }
    }

    // Instantiate a raytrace object
    CMyRaytraceRenderer raytrace;

    // Generic configurations for all renderers
    ConfigureRenderer(&raytrace);

    // Set the image
    raytrace.SetImage(m_rayimage, m_rayimagewidth, m_rayimageheight);

    // Set the window
    raytrace.SetWindow(this);

    //
    // Render the Scene
    //

    raytrace.Render(m_scene);

    Invalidate();
}


void CChildView::OnUpdateRenderRaytrace(CCmdUI* pCmdUI)
{
    // TODO: Add your command update UI handler code here
    pCmdUI->SetCheck(m_raytrace);
}


void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    m_camera.MouseDown(point.x, point.y);

    COpenGLWnd::OnLButtonDown(nFlags, point);
}


void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    if (m_camera.MouseMove(point.x, point.y, nFlags))
    {
        Invalidate();
    }

    COpenGLWnd::OnMouseMove(nFlags, point);
}


void CChildView::OnRButtonDown(UINT nFlags, CPoint point)
{
    // TODO: Add your message handler code here and/or call default
    m_camera.MouseDown(point.x, point.y, 2);

    COpenGLWnd::OnRButtonDown(nFlags, point);
}
