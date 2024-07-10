#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>

void CaptureScreen(const std::string& outputFileName, HMONITOR hMonitor) {
    // Get monitor information
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &monitorInfo);

    // Calculate width and height of the monitor
    int width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    int height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    // Create a device context (DC) for the monitor
    HDC hMonitorDC = CreateDC(TEXT("DISPLAY"), monitorInfo.szDevice, NULL, NULL);

    // Create a compatible bitmap for the screenshot
    HDC hMemoryDC = CreateCompatibleDC(hMonitorDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hMonitorDC, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // Copy the screen into the bitmap
    BitBlt(hMemoryDC, 0, 0, width, height, hMonitorDC, 0, 0, SRCCOPY);

    // Save the bitmap to a file (you can modify this part as needed)
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // Calculate the size of the bitmap
    DWORD dwBmpSize = ((width * bi.biBitCount + 31) / 32) * 4 * height;

    // Allocate enough memory for the bitmap data
    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
    char* lpbitmap = (char*)GlobalLock(hDIB);

    // Get the bitmap data
    GetDIBits(hMemoryDC, hBitmap, 0, (UINT)height, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Create and write the image file
    std::ofstream of(outputFileName, std::ios::out | std::ios::binary);
    BITMAPFILEHEADER bmfHeader;
    bmfHeader.bfType = 0x4D42; // 'BM'
    bmfHeader.bfSize = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    of.write((char*)&bmfHeader, sizeof(BITMAPFILEHEADER));
    of.write((char*)&bi, sizeof(BITMAPINFOHEADER));
    of.write(lpbitmap, dwBmpSize);

    // Clean up resources
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);
    of.close();

    // Clean up GDI objects
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    DeleteDC(hMonitorDC);
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    std::vector<std::string>* filenames = reinterpret_cast<std::vector<std::string>*>(dwData);

    // Generate a unique filename for each monitor
    char filename[MAX_PATH];
    sprintf_s(filename, "screenshot_%p.bmp", hMonitor); // Use hMonitor as part of the filename

    // Capture screen for this monitor
    CaptureScreen(filename, hMonitor);

    // Store the filename for further use
    filenames->push_back(filename);

    return TRUE;
}

int main() {
    std::vector<std::string> capturedFiles;

    // Enumerate all monitors and capture screens
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&capturedFiles));

    // Output the filenames of captured screenshots
    std::cout << "Screenshots captured:" << std::endl;
    for (const auto& file : capturedFiles) {
        std::cout << file << std::endl;
    }

    return 0;
}
