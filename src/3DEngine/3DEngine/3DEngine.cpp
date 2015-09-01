#include "stdafx.h"
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include "3DEngine.h"

HDC deviceContext = NULL;
HGLRC renderingContext = NULL;
HWND windowHandle = NULL;
HINSTANCE applicationInstance;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool keys[256];
bool hasFocus = TRUE;

const LPCWSTR ERROR_MESSAGE = _T("ERROR");
const LPCWSTR SHUTDOWN_ERROR_MESSAGE = _T("SHUTDOWN ERROR");

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)
{
    if (height == 0)
    {
        height = 1;
    }

    float zNear = 0.01f;
    float zFar = 100.0f;
    float fieldOfView = 45.0f;
    GLfloat aspectRatio = (GLfloat)width / (GLfloat)height;

    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    float screenCenterX = desktop.right / 2;
    float screenCenterY = desktop.bottom / 2;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
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
    if (renderingContext)
    {
        if (!wglMakeCurrent(NULL, NULL))
        {
            MessageBox(NULL, _T("Release Of DC And RC Failed."), SHUTDOWN_ERROR_MESSAGE, MB_OK | MB_ICONINFORMATION);
        }

        if (!wglDeleteContext(renderingContext))
        {
            MessageBox(NULL, _T("Release Rendering Context Failed."), SHUTDOWN_ERROR_MESSAGE, MB_OK | MB_ICONINFORMATION);
        }
        renderingContext = NULL;
    }

    if (deviceContext && !ReleaseDC(windowHandle, deviceContext))
    {
        MessageBox(NULL, _T("Release Device Context Failed."), SHUTDOWN_ERROR_MESSAGE, MB_OK | MB_ICONINFORMATION);
        deviceContext = NULL;
    }

    if (windowHandle && !DestroyWindow(windowHandle))
    {
        MessageBox(NULL, _T("Could Not Release windowHandle."), SHUTDOWN_ERROR_MESSAGE, MB_OK | MB_ICONINFORMATION);
        windowHandle = NULL;
    }

    if (!UnregisterClass(_T("OpenGL"), applicationInstance))
    {
        MessageBox(NULL, _T("Could Not Unregister Class."), SHUTDOWN_ERROR_MESSAGE, MB_OK | MB_ICONINFORMATION);
        applicationInstance = NULL;
    }
}

BOOL CreateGLWindow(LPCWSTR title, int width, int height, int bits)
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

    applicationInstance = GetModuleHandle(NULL);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = applicationInstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("OpenGL");

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, _T("Failed To Register The Window Class."), ERROR_MESSAGE, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW;

    AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

    if (!(windowHandle = CreateWindowEx(dwExStyle,
        _T("OpenGL"),
        title,
        dwStyle |
        WS_CLIPSIBLINGS |
        WS_CLIPCHILDREN,
        0, 0,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        NULL,
        NULL,
        applicationInstance,
        NULL)))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Window Creation Error."), ERROR_MESSAGE, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    static PIXELFORMATDESCRIPTOR pfd =
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

    if (!(deviceContext = GetDC(windowHandle)))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Create A GL Device Context."), ERROR_MESSAGE, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if (!(PixelFormat = ChoosePixelFormat(deviceContext, &pfd)))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Find A Suitable PixelFormat."), ERROR_MESSAGE, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if (!SetPixelFormat(deviceContext, PixelFormat, &pfd))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Set The PixelFormat."), ERROR_MESSAGE, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if (!(renderingContext = wglCreateContext(deviceContext)))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Create A GL Rendering Context."), ERROR_MESSAGE, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    if (!wglMakeCurrent(deviceContext, renderingContext))
    {
        KillGLWindow();
        MessageBox(NULL, _T("Can't Activate The GL Rendering Context."), ERROR_MESSAGE, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);
    SetFocus(windowHandle);
    ReSizeGLScene(width, height);

    if (!InitGL())
    {
        KillGLWindow();
        MessageBox(NULL, _T("Initialization Failed."), ERROR_MESSAGE, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND windowHandle, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ACTIVATE:
    {
        if (!HIWORD(wParam))
        {
            hasFocus = TRUE;
        }
        else
        {
            hasFocus = FALSE;
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

    return DefWindowProc(windowHandle, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE applicationInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    BOOL done = FALSE;

    int width = 1024;
    int height = 768;
    LPCWSTR title = _T("Polygonum");

    if (!CreateGLWindow(title, width, height, 16))
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
            if (hasFocus)
            {
                if (keys[VK_ESCAPE])
                {
                    done = TRUE;
                }
                else
                {
                    DrawGLScene();
                    SwapBuffers(deviceContext);
                }
            }
        }
    }

    KillGLWindow();
    return (msg.wParam);
}