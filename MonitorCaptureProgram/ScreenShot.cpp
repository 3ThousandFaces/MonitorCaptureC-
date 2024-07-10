#include <Windows.h>
#include <iostream>
#include <fstream>

void CaptureScreen(int monitorIndex, const char* bmpFileName) {
    // Get handle to the device context for the entire screen
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMemDC = CreateCompatibleDC(hdcScreen);

    // Get monitor info
    MONITORINFO mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &mi);

    // Create a compatible bitmap to hold the screenshot
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top);
    SelectObject(hdcMemDC, hbmScreen);

    // Perform the bit-block transfer from screen to memory device context
    if (!BitBlt(hdcMemDC, 0, 0,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        hdcScreen,
        mi.rcMonitor.left, mi.rcMonitor.top,
        SRCCOPY)) {
        std::cerr << "Failed to capture screen" << std::endl;
        return;
    }

    // Save the bitmap to a file
    BITMAP bmp;
    GetObject(hbmScreen, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;

    // Add bitmap file header
    bmfHeader.bfType = 0x4D42; // BM
    bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    std::ofstream file(bmpFileName, std::ios::out | std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file for writing" << std::endl;
        DeleteObject(hbmScreen);
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);
        return;
    }

    file.write(reinterpret_cast<char*>(&bmfHeader), sizeof(BITMAPFILEHEADER));
    file.write(reinterpret_cast<char*>(&bi), sizeof(BITMAPINFOHEADER));

    BYTE* lpbitmap = new BYTE[dwBmpSize];
    GetBitmapBits(hbmScreen, dwBmpSize, lpbitmap);

    // Write bitmap data to file
    file.write(reinterpret_cast<char*>(lpbitmap), dwBmpSize);

    // Clean up resources
    file.close();
    DeleteObject(hbmScreen);
    DeleteDC(hdcMemDC);
    ReleaseDC(NULL, hdcScreen);
    delete[] lpbitmap;
}

// Callback function for EnumDisplayMonitors
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    static int monitorIndex = 0;
    CHAR bmpFileName[MAX_PATH];
    sprintf_s(bmpFileName, "monitor%d.bmp", monitorIndex++);

    CaptureScreen(monitorIndex - 1, bmpFileName);

    return TRUE; // Continue enumerating monitors
}

int main() {
    // Enumerate through all monitors
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, NULL);

    return 0;
}
