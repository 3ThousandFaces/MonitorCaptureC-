#include <windows.h>
#include <wingdi.h>
#include <iostream>
#include <fstream>
#include <chrono> // For timing
#include <thread> // For std::this_thread::sleep_until
#include <vector>

// Function declarations
void CaptureMonitor(const char* filename, HMONITOR hMonitor, int fps);
HMONITOR MonitorFromIndex(int monitorIndex);
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

// Function to capture each monitor and save as bitmap
void CaptureAllMonitors(const char* baseFilename, int fps) {
    std::vector<HMONITOR> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));

    for (size_t i = 0; i < monitors.size(); ++i) {
        char filename[256];
        sprintf_s(filename, "%s_%zu.bmp", baseFilename, i);

        CaptureMonitor(filename, monitors[i], fps);
    }
}

// Function to capture a specific monitor and save as bitmap
void CaptureMonitor(const char* filename, HMONITOR hMonitor, int fps) {
    // Get monitor info
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &monitorInfo);

    // Calculate capture dimensions
    int captureWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    int captureHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    // Calculate time interval between frames
    auto frameDuration = std::chrono::milliseconds(1000 / fps);

    while (true) {
        auto startTime = std::chrono::steady_clock::now();

        HDC hMonitorDC = CreateDC(TEXT("DISPLAY"), monitorInfo.szDevice, NULL, NULL); // Get DC for monitor
        HDC hMemoryDC = CreateCompatibleDC(hMonitorDC); // Create a compatible DC for bitmap

        // Create bitmap compatible with the monitor DC
        HBITMAP hBitmap = CreateCompatibleBitmap(hMonitorDC, captureWidth, captureHeight);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

        // Copy monitor to the compatible DC
        BitBlt(hMemoryDC, 0, 0, captureWidth, captureHeight, hMonitorDC, 0, 0, SRCCOPY);

        // Clean up
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteDC(hMemoryDC);
        DeleteDC(hMonitorDC);

        // Save bitmap to file
        BITMAPINFOHEADER bmiHeader;
        memset(&bmiHeader, 0, sizeof(BITMAPINFOHEADER));
        bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmiHeader.biWidth = captureWidth;
        bmiHeader.biHeight = -captureHeight; // Negative height to ensure top-down bitmap
        bmiHeader.biPlanes = 1;
        bmiHeader.biBitCount = 24; // 24-bit bitmap
        bmiHeader.biCompression = BI_RGB;
        bmiHeader.biSizeImage = 0;

        BITMAPFILEHEADER bmfHeader;
        memset(&bmfHeader, 0, sizeof(BITMAPFILEHEADER));
        bmfHeader.bfType = 0x4D42; // 'BM'
        bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bmfHeader.bfSize = bmfHeader.bfOffBits + (captureWidth * captureHeight * 3); // 3 bytes per pixel (24-bit)

        std::ofstream file(filename, std::ios::out | std::ios::binary);
        file.write(reinterpret_cast<const char*>(&bmfHeader), sizeof(BITMAPFILEHEADER));
        file.write(reinterpret_cast<const char*>(&bmiHeader), sizeof(BITMAPINFOHEADER));

        // Get bitmap data
        char* lpPixels = new char[captureWidth * captureHeight * 3];
        GetDIBits(hMonitorDC, hBitmap, 0, captureHeight, lpPixels, (BITMAPINFO*)&bmiHeader, DIB_RGB_COLORS);

        // Write bitmap data to file
        file.write(lpPixels, captureWidth * captureHeight * 3);

        // Clean up
        delete[] lpPixels;
        file.close();
        DeleteObject(hBitmap);

        // Calculate time for next frame
        auto endTime = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        auto sleepTime = frameDuration - elapsedTime;

        if (sleepTime > std::chrono::milliseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
}

// Helper function to get monitor handle from index
HMONITOR MonitorFromIndex(int monitorIndex) {
    std::vector<HMONITOR> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));

    if (monitorIndex < 0 || monitorIndex >= static_cast<int>(monitors.size())) {
        return NULL;
    }

    return monitors[monitorIndex];
}

// Callback function for EnumDisplayMonitors
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    std::vector<HMONITOR>* monitors = reinterpret_cast<std::vector<HMONITOR>*>(dwData);
    monitors->push_back(hMonitor);
    return TRUE;
}

int main() {
    // Capture all monitors at 60 fps
    CaptureAllMonitors("monitor_capture", 60);
    return 0;
}
