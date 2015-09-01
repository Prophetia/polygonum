/*
*		This Code Was Created By Jeff Molofee 2000
*		A HUGE Thanks To Fredric Echols For Cleaning Up
*		And Optimizing This Code, Making It More Flexible!
*		If You've Found This Code Useful, Please Let Me Know.
*		Visit My Site At nehe.gamedev.net
*/

#include "stdafx.h"
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
//*********seems to not work anymore and not needed too************
//#include <gl\glaux.h>		// Header File For The Glaux Library
#include "3DEngine.h"

HDC hDC = NULL;
HGLRC hRC = NULL;
HWND hWnd = NULL;
HINSTANCE hInstance;

bool keys[256];
bool active = TRUE;
bool fullscreen = TRUE;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)
{
    if (height == 0)
    {
        height = 1;
    }

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float fieldOfView = 45.0f;
    float aspectRatio = (GLfloat)width / (GLfloat)height;
    float zNear = 0.01f;
    float zFar = 100.0f;
    gluPerspective(fieldOfView, aspectRatio, zNear, zFar);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int InitGL(GLvoid)
{
    glShadeModel(GL_SMOOTH);
    glClearColor(255.0f, 255.0f, 255.0f, 0.5f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    return TRUE;
}

int DrawGLScene(GLvoid)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    return TRUE;
}

GLvoid KillGLWindow(GLvoid)
{
    if (fullscreen)
    {
        ChangeDisplaySettings(NULL, 0);
        ShowCursor(TRUE);
    }

    if (hRC)
    {
        if (!wglMakeCurrent(NULL, NULL))
        {
            MessageBox(NULL, _T("Release Of DC And RC Failed."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
        }

        if (!wglDeleteContext(hRC))
        {
            MessageBox(NULL, _T("Release Rendering Context Failed."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
        }
        hRC = NULL;
    }

    if (hDC && !ReleaseDC(hWnd, hDC))
    {
        MessageBox(NULL, _T("Release Device Context Failed."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
        hDC = NULL;
    }

    if (hWnd && !DestroyWindow(hWnd))
    {
        MessageBox(NULL, _T("Could Not Release hWnd."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
        hWnd = NULL;
    }

    if (!UnregisterClass(_T("OpenGL"), hInstance))
    {
        MessageBox(NULL, _T("Could Not Unregister Class."), _T("SHUTDOWN ERROR"), MB_OK | MB_ICONINFORMATION);
        hInstance = NULL;
    }
}

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
    GLuint PixelFormat;
    WNDCLASS wc;
    DWORD dwExStyle;
    DWORD dwStyle;
    RECT WindowRect;
    WindowRect.left = (long)0;
    WindowRect.right = (long)width;
    WindowRect.top = (long)0;
    WindowRect.bottom = (long)height;

    fullscreen = fullscreenflag;

    hInstance = GetModuleHandle(NULL);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("OpenGL");

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, _T("Failed To Register The Window Class."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if (fullscreen)
    {
        DEVMODE dmScreenSettings;
        memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth = width;
        dmScreenSettings.dmPelsHeight = height;
        dmScreenSettings.dmBitsPerPel = bits;
        dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            if (MessageBox(NULL, _T("The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?"), _T("NeHe GL"), MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
            {
                fullscreen = FALSE;
            }
            else
            {
                MessageBox(NULL, _T("Program Will Now Close."), _T("ERROR"), MB_OK | MB_ICONSTOP);
                return FALSE;
            }
        }
    }

    if (fullscreen)
    {
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle = WS_POPUP;
        ShowCursor(FALSE);
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle = WS_OVERLAPPEDWINDOW;
    }

    AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

    if (!(hWnd = CreateWindowEx(dwExStyle,
        _T("OpenGL"),
        _T("Title"),
        dwStyle |
        WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN,
        0, 0,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        NULL,
        NULL,
        hInstance,
        NULL)))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Window Creation Error."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    static	PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        bits,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        16,
        0,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    if (!(hDC = GetDC(hWnd)))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Create A GL Device Context."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Find A Suitable PixelFormat."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if (!SetPixelFormat(hDC, PixelFormat, &pfd))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Set The PixelFormat."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if (!(hRC = wglCreateContext(hDC)))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Create A GL Rendering Context."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if (!wglMakeCurrent(hDC, hRC))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Activate The GL Rendering Context."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
    ReSizeGLScene(width, height);

    if (!InitGL())
    {
        KillGLWindow();
        MessageBox(NULL, _T("Initialization Failed."), _T("ERROR"), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ACTIVATE:
    {
        if (!HIWORD(wParam))
        {
            active = TRUE;
        }
        else
        {
            active = FALSE;
        }

        return 0;
    }

    case WM_SYSCOMMAND:
    {
        switch (wParam)
        {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            return 0;
        }
        break;
    }

    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    }

    case WM_KEYDOWN:
    {
        keys[wParam] = TRUE;
        return 0;
    }

    case WM_KEYUP:
    {
        keys[wParam] = FALSE;
        return 0;
    }

    case WM_SIZE:
    {
        ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));
        return 0;
    }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    BOOL done = FALSE;

    if (MessageBox(NULL, _T("Would You Like To Run In Fullscreen Mode?"), _T("Start FullScreen?"), MB_YESNO | MB_ICONQUESTION) == IDNO)
    {
        fullscreen = FALSE;
    }

    if (!CreateGLWindow("NeHe's OpenGL Framework", 640, 480, 16, fullscreen))
    {
        return 0;
    }

    while (!done)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                done = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            if (active)
            {
                if (keys[VK_ESCAPE])
                {
                    done = TRUE;
                }
                else
                {
                    DrawGLScene();
                    SwapBuffers(hDC);
                }
            }

            if (keys[VK_F1])
            {
                keys[VK_F1] = FALSE;
                KillGLWindow();
                fullscreen = !fullscreen;

                if (!CreateGLWindow("NeHe's OpenGL Framework", 640, 480, 16, fullscreen))
                {
                    return 0;
                }
            }
        }
    }
    
    KillGLWindow();
    return (msg.wParam);
}