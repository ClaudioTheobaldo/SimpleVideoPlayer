// CppVideoPlayer.cpp : Defines the entry point for the application.
//
#define __STDC_CONSTANT_MACROS

#include "framework.h"
#include "commctrl.h"
#include "CppVideoPlayer.h"
#include "VideoPlayer.h"

extern "C"
{
    #include "libavutil/avutil.h"
    #include "libavcodec/avcodec.h"
}

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Controls
HWND fStartButton;
HWND fStopButton;
HWND fLoadButton;
HWND fImage;
HWND fTrackBar;

// Classes
VideoPlayer* videoPlayer;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

typedef struct BGRPixel
{
    BYTE B;
    BYTE G;
    BYTE R;
} BGRPixel;

typedef BGRPixel* PBGRPixel;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CPPVIDEOPLAYER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CPPVIDEOPLAYER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CPPVIDEOPLAYER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CPPVIDEOPLAYER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

HBITMAP GenerateRandomColorBitmap()
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 240;         // Width of the bitmap
    bmi.bmiHeader.biHeight = -180;      // Height of the bitmap (negative value to indicate top-down orientation)
    bmi.bmiHeader.biPlanes = 1;            // Number of color planes
    bmi.bmiHeader.biBitCount = 24;    // Number of bits per pixel
    bmi.bmiHeader.biCompression = BI_RGB;  // Compression type (RGB uncompressed)

    auto hdc = CreateCompatibleDC(NULL);
    LPBYTE imgbuffer = nullptr;
    HBITMAP bmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&imgbuffer, NULL, 0);
    BGRPixel pixel;
    pixel.R = 234;
    pixel.G = 224;
    pixel.B = 204;      
    for (int i = 0; i < 180; i++)
        for (int j = 0; j < 80; j++)
            *(PBGRPixel)(imgbuffer + ((240 * 3) * i) + (j * 3)) = pixel;
    pixel.R = 142;
    pixel.G = 100;
    pixel.B = 82;
    for (int i = 0; i < 180; i++)
        for (int j = 80; j < 160; j++)
            *(PBGRPixel)(imgbuffer + ((240 * 3) * i) + (j * 3)) = pixel;
    pixel.R = 160;
    pixel.G = 160;
    pixel.B = 131;
    for (int i = 0; i < 180; i++)
        for (int j = 160; j < 240; j++)
            *(PBGRPixel)(imgbuffer + ((240 * 3) * i) + (j * 3)) = pixel;
    ReleaseDC(NULL, hdc);
    return bmp;
}

void CreateTrackBar(HWND mainWindowHandle)
{
    fTrackBar = CreateWindowEx(
        0,                               // no extended styles 
        TRACKBAR_CLASS,                  // class name 
        NULL,              // title (caption) 
        WS_CHILD |
        WS_VISIBLE |
        TBS_AUTOTICKS |
        TBS_ENABLESELRANGE,              // style 
        150, 400,                          // position 
        640, 30,                         // size 
        mainWindowHandle,                         // parent window 
        HMENU(150),                     // control identifier 
        (HINSTANCE)GetWindowLongPtr(mainWindowHandle, GWLP_HINSTANCE),                         // instance 
        NULL                             // no WM_CREATE parameter 
    );
}

void CreateStartButton(HWND mainWindowHandle)
{
    fStartButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Start",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        10, // x position 
        50, // y position 
        120, // Button width
        30, // Button height
        mainWindowHandle, // Parent window
        NULL, // No menu.
        (HINSTANCE)GetWindowLongPtr(mainWindowHandle, GWLP_HINSTANCE),
        NULL); // Pointer not needed.
}

void CreateStopButton(HWND mainWindowHandle)
{
    fStopButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Stop",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        10, // x position 
        90, // y position 
        120, // Button width
        30, // Button height
        mainWindowHandle, // Parent window
        NULL, // No menu.
        (HINSTANCE)GetWindowLongPtr(mainWindowHandle, GWLP_HINSTANCE),
        NULL); // Pointer not needed.
}

void CreateLoadFileButton(HWND mainWindowHandle)
{
    fLoadButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Load",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        10, // x position 
        10, // y position 
        120, // Button width
        30, // Button height
        mainWindowHandle, // Parent window
        NULL, // No menu.
        (HINSTANCE)GetWindowLongPtr(mainWindowHandle, GWLP_HINSTANCE),
        NULL); // Pointer not needed.
}

void CreateCanvas(HWND mainWindowHandle)
{
    fImage = CreateWindow(L"Static", L"Canvas", WS_VISIBLE | WS_CHILD | SS_BITMAP, 
        150, 10, 240, 180, mainWindowHandle, NULL, 
        (HINSTANCE)GetWindowLongPtr(mainWindowHandle, GWLP_HINSTANCE),
        NULL);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   videoPlayer = new VideoPlayer();

   CreateLoadFileButton(hWnd);
   CreateStartButton(hWnd);
   CreateStopButton(hWnd);
   CreateCanvas(hWnd);
   CreateTrackBar(hWnd);
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int notificationCode = HIWORD(wParam);
            if ((HWND)lParam == fStartButton)
            {
                HBITMAP bmp = GenerateRandomColorBitmap();
                SendMessage(fImage, STM_SETIMAGE, IMAGE_BITMAP, LPARAM(bmp));
            }
            else if ((HWND)lParam == fLoadButton)
            {
                videoPlayer->LoadFile();
            }
            else
            {
                switch (wmId)
                {
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
                }
            }
        }
        break;
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
