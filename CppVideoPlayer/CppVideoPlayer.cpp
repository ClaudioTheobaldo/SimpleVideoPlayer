#define __STDC_CONSTANT_MACROS

#include "framework.h"
#include "commctrl.h"
#include "CppVideoPlayer.h"
#include "VideoPlayer.h"
#include "WinTimer.h"
#include <sstream>
#include "Utils.h"
#include <timeapi.h>
#include <mmsystem.h>
#include "MultimediaTimer.h"
#include <vector>

extern "C"
{
    #include "libavutil/avutil.h"
    #include "libavcodec/avcodec.h"
}

#define MAX_LOADSTRING 100

// Global Variables
HINSTANCE hInst;                 
WCHAR szTitle[MAX_LOADSTRING];            
WCHAR szWindowClass[MAX_LOADSTRING];
int fImageWidth;
int fImageHeight;
int fFPS;
int fLastFPS;
HBITMAP fBmp;
LPBYTE fImgBuffer;
VideoPlayer* fVideoPlayer;
WinTimer* fFPSTimer;
bool fCanChangeSlider = false;

// Controls
HWND fLoadButton;
HWND fStartButton;
HWND fStopButton;
HWND fUnloadButton;
HWND fLoopCheckBox;
HWND fFileEdit;
HWND fImage;
HWND fLeftTrackbarLabel;
HWND fRightTrackbarLabel;
HWND fPositionLabel;
HWND fTrackBar;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

void FPSCount(void* opaque)
{
    fLastFPS = fFPS;
    fFPS = 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CPPVIDEOPLAYER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    if (!InitInstance (hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CPPVIDEOPLAYER));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
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
    wcex.lpszMenuName = nullptr; 
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

void CreateBitmap(int width, int height)
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;         // Width of the bitmap
    bmi.bmiHeader.biHeight = -height;      // Height of the bitmap (negative value to indicate top-down orientation)
    bmi.bmiHeader.biPlanes = 1;            // Number of color planes
    bmi.bmiHeader.biBitCount = 24;    // Number of bits per pixel
    bmi.bmiHeader.biCompression = BI_RGB;  // Compression type (RGB uncompressed)

    auto hdc = CreateCompatibleDC(NULL);
    LPBYTE fImgbuffer = nullptr;
    fBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS,
        (void**)&fImgBuffer, NULL, 0);
    ReleaseDC(NULL, hdc);
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
        150, 580,                          // position 
        640, 35,                         // size 
        mainWindowHandle,                         // parent window 
        HMENU(150),                     // control identifier 
        (HINSTANCE)GetWindowLongPtr(mainWindowHandle, GWLP_HINSTANCE),                         // instance 
        NULL                             // no WM_CREATE parameter 
    );

    fLeftTrackbarLabel = CreateWindowW(L"Static", L"00:00:00",
        SS_CENTER | WS_CHILD | WS_VISIBLE, 75, 580, 75, 35, 
        mainWindowHandle, (HMENU)151, hInst, NULL);

    fRightTrackbarLabel = CreateWindowW(L"Static", L"00:00:00",
        SS_CENTER | WS_CHILD | WS_VISIBLE, 790, 580, 75, 35,
        mainWindowHandle, (HMENU)152, hInst, NULL);

    fPositionLabel = CreateWindowW(L"Static", L"00:00:00",
        SS_CENTER | WS_CHILD | WS_VISIBLE, 150, 615, 75, 20,
        mainWindowHandle, (HMENU)152, hInst, NULL);

    SendMessageW(fTrackBar, TBM_SETRANGE, TRUE, MAKELONG(0, 5000));
    SendMessageW(fTrackBar, TBM_SETPAGESIZE, 0, 10);
    SendMessageW(fTrackBar, TBM_SETTICFREQ, 10, 0);
    SendMessageW(fTrackBar, TBM_SETPOS, FALSE, 0);
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

void CreateUnloadFileButtion(HWND mainWindowHandle)
{
    fUnloadButton = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Unload",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        10, // x position 
        130, // y position 
        120, // Button width
        30, // Button height
        mainWindowHandle, // Parent window
        NULL, // No menu.
        (HINSTANCE)GetWindowLongPtr(mainWindowHandle, GWLP_HINSTANCE),
        NULL); // Pointer not needed.
}

void CreateCheckBox(HWND mainWindowHandle)
{
    fLoopCheckBox = CreateWindow(
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"Loop",      // Button text 
        WS_VISIBLE | WS_CHILD | BS_CHECKBOX,  // Styles 
        10, // x position 
        170, // y position 
        120, // Button width
        20, // Button height
        mainWindowHandle, // Parent window
        (HMENU)1, // No menu.
        (HINSTANCE)GetWindowLongPtr(mainWindowHandle, GWLP_HINSTANCE),
        NULL); // Pointer not needed.
}

void CreateFileInputEdit(HWND mainWindowHandle)
{
    fFileEdit = CreateWindow(L"Edit", NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        150, 10, 480, 25, mainWindowHandle, (HMENU)0,
        NULL, NULL);
    SetWindowTextA(fFileEdit, "..\\Data\\Wildlife.mp4");
}

void CreateCanvas(HWND mainWindowHandle)
{
    fImage = CreateWindow(L"Static", L"Canvas", WS_VISIBLE | WS_CHILD | SS_BITMAP, 
        150, 60, fImageWidth, fImageHeight, mainWindowHandle, NULL, 
        (HINSTANCE)GetWindowLongPtr(mainWindowHandle, GWLP_HINSTANCE),
        NULL);
}

void WriteFPS()
{
    auto canvasDC = GetWindowDC(fImage);
    RECT rect;
    rect.left = 10;
    rect.top = 10;
    rect.right = 60;
    rect.bottom = 60;
    std::stringstream buff;
    buff << "FPS: " << fLastFPS;
    auto s = buff.str();
    std::wstring ws = std::wstring(s.begin(), s.end());
    SetTextColor(canvasDC, 0x0000FF00);
    SetBkMode(canvasDC, TRANSPARENT);
    DrawText(canvasDC, ws.c_str(), -1, &rect, DT_CENTER);
    DeleteDC(canvasDC);
}

void ImageCallback(char* imgBuffer, int width, int height, long long position)
{
    memcpy(fImgBuffer, imgBuffer, height * (width * 3));
    SendMessage(fImage, STM_SETIMAGE, IMAGE_BITMAP, LPARAM(fBmp));
    fFPS++;
    WriteFPS();
    if (fCanChangeSlider)
    {
        SendMessage(fTrackBar, TBM_SETPOS, TRUE, position * fVideoPlayer->GetStreamTimeBase());
        SetWindowText(fPositionLabel,
            FormatVideoDate(FromSeconds(position * fVideoPlayer->GetStreamTimeBase())).c_str());
    }
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance;

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 1024, 768, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
      return FALSE;
 
   fImageWidth = 640;
   fImageHeight = 480;
   fFPS = 0;
   fLastFPS = 0;

   fVideoPlayer = new VideoPlayer();
   fVideoPlayer->SetOutputHeight(fImageHeight);
   fVideoPlayer->SetOutputWidth(fImageWidth);
   fVideoPlayer->SetImageCallback(ImageCallback);

   fFPSTimer = new WinTimer();
   fFPSTimer->SetInterval(1000);
   fFPSTimer->SetTimerCallback(FPSCount);
   fFPSTimer->Start();

   CreateBitmap(640, 480);
   CreateLoadFileButton(hWnd);
   CreateStartButton(hWnd);
   CreateStopButton(hWnd);
   CreateUnloadFileButtion(hWnd);
   CreateCheckBox(hWnd);
   CreateCanvas(hWnd);
   CreateTrackBar(hWnd);
   CreateFileInputEdit(hWnd);
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
                if (!fVideoPlayer->IsPlaying())
                    fVideoPlayer->Play();
            }
            else if ((HWND)lParam == fStopButton)
            {
                if (fVideoPlayer->IsPlaying())
                    fVideoPlayer->Stop();
            }
            else if ((HWND)lParam == fLoadButton)
            {
                int len = GetWindowTextLengthA(fFileEdit) + 1;
                char* text = (char*)malloc(sizeof(char) * len);
                #pragma warning(suppress : 6387)
                GetWindowTextA(fFileEdit, text, len);
                auto ret = fVideoPlayer->LoadFile(text);
                free(text);
                if (ret != 0)
                {
                    MessageBox(nullptr, L"Unable to load video file.", L"", MB_OK);
                    return 0;
                }
                auto duration = fVideoPlayer->GetStreamDuration();
                SendMessageW(fTrackBar, TBM_SETRANGE, TRUE, MAKELONG(0, duration));
                auto date = FromSeconds(duration);
                auto formateddate = FormatVideoDate(date);
                SetWindowText(fRightTrackbarLabel, formateddate.c_str());
            }
            else if ((HWND)lParam == fUnloadButton)
            {
                fVideoPlayer->Flush();
            }
            else if ((HWND)lParam == fLoopCheckBox)
            {
                auto checked = IsDlgButtonChecked(hWnd, 1);
                CheckDlgButton(hWnd, 1, !checked);
                fVideoPlayer->SetAutoReset(!checked);
            }
            else
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
        case WM_HSCROLL:
        {
            if ((HWND)lParam == fTrackBar && LOWORD(wParam) == 5)
            {
                fCanChangeSlider = false;
                SetWindowText(fPositionLabel, FormatVideoDate(FromSeconds(HIWORD(wParam))).c_str());
            }
            if ((HWND)lParam == fTrackBar && LOWORD(wParam) == 8)
            {
                auto pos = SendMessageW(fTrackBar, TBM_GETPOS, 0, 0);
                fVideoPlayer->Seek(pos);
                fCanChangeSlider = true;
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
            delete fVideoPlayer;
            delete fFPSTimer;
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}