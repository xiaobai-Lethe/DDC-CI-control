#include "ddc_controller.h"
#include <QDebug>
#include <QDateTime>

#ifdef Q_OS_WIN
#include <windows.h>
/* 添加更多Windows头文件以解决缺少函数声明的问题 */
#include <wingdi.h>
#include <winuser.h>
#include <physicalmonitorenumerationapi.h>
#include <highlevelmonitorconfigurationapi.h>
/* 注意：MinGW不支持pragma comment，改为在项目文件中添加库依赖 */
/* #pragma comment(lib, "Dxva2.lib") */
#endif

/* 初始化静态实例为nullptr */
DdcController* DdcController::instance = nullptr;

/* 获取DdcController单例 */
DdcController* DdcController::getInstance()
{
    if (instance == nullptr) {
        instance = new DdcController();
    }
    return instance;
}

/* 构造函数 */
DdcController::DdcController(QObject *parent) : QObject(parent)
{
    forcedSupport = false;
    isForceSet = false;
    optimizeMode = true;  // 默认启用优化模式
    initialize();
}

/* 初始化并检测显示器 */
void DdcController::initialize()
{
    qDebug() << "正在初始化DDC/CI控制器";
    detectDdcSupport();
}

/* 检测显示器是否支持DDC/CI */
bool DdcController::detectDdcSupport()
{
#ifdef Q_OS_WIN
    return detectDdcSupportWin();
#elif defined(Q_OS_LINUX)
    return detectDdcSupportLinux();
#else
    qDebug() << "当前平台不支持DDC/CI检测";
    return false;
#endif
}

/* 获取支持DDC/CI的显示器数量 */
int DdcController::getSupportedDisplayCount() const
{
    int count = 0;
    for (const auto& display : displays) {
        if (display.supportsDdc) {
            count++;
        }
    }
    return count;
}

/* 设置优化模式 */
void DdcController::setOptimizeMode(bool optimize)
{
    optimizeMode = optimize;
    qDebug() << "DDC/CI优化模式已" << (optimize ? "启用" : "禁用");
}

/* 设置显示器亮度 */
bool DdcController::setBrightness(int displayIndex, int brightness)
{
    /* 检查亮度值是否在有效范围内 */
    if (brightness < 0 || brightness > 100) {
        qDebug() << "亮度值无效，应在0-100范围内";
        return false;
    }

    /* 检查显示器索引是否有效 - 使用优化的查找方式 */
    static int lastSupportedIndex = -1;  // 缓存上次找到的索引，减少查找次数
    int supportedDisplayIndex = -1;
    
    // 先尝试使用缓存的索引
    if (lastSupportedIndex >= 0 && lastSupportedIndex < displays.size() && 
        displays[lastSupportedIndex].supportsDdc) {
        int count = 0;
        for (int i = 0; i <= lastSupportedIndex; i++) {
            if (displays[i].supportsDdc) {
                count++;
            }
        }
        if (count-1 == displayIndex) {  // 索引匹配
            supportedDisplayIndex = lastSupportedIndex;
        }
    }
    
    // 如果缓存的索引不匹配，则重新查找
    if (supportedDisplayIndex == -1) {
        int currentIndex = 0;
        for (int i = 0; i < displays.size(); i++) {
            if (displays[i].supportsDdc) {
                if (currentIndex == displayIndex) {
                    supportedDisplayIndex = i;
                    lastSupportedIndex = i;  // 更新缓存
                    break;
                }
                currentIndex++;
            }
        }
    }

    if (supportedDisplayIndex == -1) {
        qDebug() << "显示器索引无效:" << displayIndex;
        return false;
    }
    
    /* 在优化模式下，检查时间间隔和上次亮度是否相同 */
    if (optimizeMode) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 lastTime = displays[supportedDisplayIndex].lastUpdateTime;
        int lastBrightness = displays[supportedDisplayIndex].currentBrightness;
        
        /* 如果时间间隔很短且亮度相同，则跳过实际设置 */
        if (lastTime > 0 && 
            (currentTime - lastTime) < MIN_UPDATE_INTERVAL && 
            brightness == lastBrightness) {
            return true;  // 亮度相同，无需更新
        }
        
        /* 更新最后设置时间 */
        displays[supportedDisplayIndex].lastUpdateTime = currentTime;
    }

#ifdef Q_OS_WIN
    bool result = setBrightnessWin(supportedDisplayIndex, brightness);
    if (result) {
        displays[supportedDisplayIndex].currentBrightness = brightness;
    }
    return result;
#elif defined(Q_OS_LINUX)
    bool result = setBrightnessLinux(supportedDisplayIndex, brightness);
    if (result) {
        displays[supportedDisplayIndex].currentBrightness = brightness;
    }
    return result;
#else
    qDebug() << "当前平台不支持设置亮度";
    return false;
#endif
}

/* 获取显示器当前亮度 */
int DdcController::getCurrentBrightness(int displayIndex) const
{
    /* 检查显示器索引是否有效 */
    int supportedDisplayIndex = -1;
    int currentIndex = 0;

    for (int i = 0; i < displays.size(); i++) {
        if (displays[i].supportsDdc) {
            if (currentIndex == displayIndex) {
                supportedDisplayIndex = i;
                break;
            }
            currentIndex++;
        }
    }

    if (supportedDisplayIndex == -1) {
        qDebug() << "显示器索引无效:" << displayIndex;
        return -1;
    }

#ifdef Q_OS_WIN
    return getCurrentBrightnessWin(supportedDisplayIndex);
#elif defined(Q_OS_LINUX)
    return getCurrentBrightnessLinux(supportedDisplayIndex);
#else
    qDebug() << "当前平台不支持获取亮度";
    return -1;
#endif
}

#ifdef Q_OS_WIN
/* Windows平台检测DDC/CI支持 */
bool DdcController::detectDdcSupportWin()
{
    displays.clear();
    
    /* 枚举所有显示设备 */
    DISPLAY_DEVICE displayDevice;
    displayDevice.cb = sizeof(DISPLAY_DEVICE);
    
    for (DWORD deviceIndex = 0; EnumDisplayDevices(NULL, deviceIndex, &displayDevice, 0); deviceIndex++) {
        /* 只检查连接的设备 */
        if (!(displayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
            continue;
        }
        
        /* 获取设备的句柄 */
        DEVMODE devMode;
        devMode.dmSize = sizeof(DEVMODE);
        if (!EnumDisplaySettings(displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &devMode)) {
            continue;
        }
        
        /* 创建一个DC用于MonitorFromWindow */
        HDC hdc = CreateDC(NULL, displayDevice.DeviceName, NULL, NULL);
        if (!hdc) {
            continue;
        }
        
        /* 获取监视器句柄 - 使用MonitorFromWindow代替MonitorFromDC */
        HWND desktopHwnd = GetDesktopWindow();
        HMONITOR hMonitor = MonitorFromWindow(desktopHwnd, MONITOR_DEFAULTTOPRIMARY);
        DeleteDC(hdc);
        
        if (!hMonitor) {
            continue;
        }
        
        /* 获取物理监视器数量 */
        DWORD physicalMonitorCount = 0;
        if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &physicalMonitorCount) || physicalMonitorCount == 0) {
            continue;
        }
        
        /* 分配物理监视器数组 */
        PHYSICAL_MONITOR* physicalMonitors = new PHYSICAL_MONITOR[physicalMonitorCount];
        
        /* 获取物理监视器 */
        if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, physicalMonitorCount, physicalMonitors)) {
            delete[] physicalMonitors;
            continue;
        }
        
        /* 检查每个物理监视器是否支持DDC/CI */
        for (DWORD monitorIndex = 0; monitorIndex < physicalMonitorCount; monitorIndex++) {
            HANDLE hPhysicalMonitor = physicalMonitors[monitorIndex].hPhysicalMonitor;
            
            /* 直接尝试获取亮度来检测是否支持DDC/CI */
            DWORD minimumBrightness = 0, currentBrightness = 0, maximumBrightness = 0;
            bool canReadBrightness = GetMonitorBrightness(hPhysicalMonitor, &minimumBrightness, &currentBrightness, &maximumBrightness);
            
            DisplayInfo info;
            info.index = displays.size();
            info.supportsDdc = canReadBrightness;
            info.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();  // 初始化更新时间
            
            if (canReadBrightness) {
                /* 将亮度值转换为0-100范围 */
                double normalizedBrightness = ((double)(currentBrightness - minimumBrightness) / 
                                             (maximumBrightness - minimumBrightness)) * 100.0;
                info.currentBrightness = qRound(normalizedBrightness);
                
                qDebug() << "检测到显示器:" << deviceIndex << "支持DDC/CI，当前亮度:" << info.currentBrightness;
                displays.append(info);
            } else {
                qDebug() << "显示器:" << deviceIndex << "不支持DDC/CI或无法读取亮度";
            }
            
            /* 关闭物理监视器句柄 */
            DestroyPhysicalMonitor(hPhysicalMonitor);
        }
        
        delete[] physicalMonitors;
    }
    
    return getSupportedDisplayCount() > 0;
}

/* Windows平台设置亮度 */
bool DdcController::setBrightnessWin(int displayIndex, int brightness)
{
    if (displayIndex < 0 || displayIndex >= displays.size() || !displays[displayIndex].supportsDdc) {
        return false;
    }
    
    // 使用静态变量存储上次使用的监视器句柄信息，减少重复获取句柄的开销
    static struct {
        int displayIndex = -1;
        HMONITOR hMonitor = NULL;
        HANDLE hPhysicalMonitor = NULL;
        DWORD minimumBrightness = 0;
        DWORD maximumBrightness = 100;
        qint64 lastUseTime = 0;
        DWORD lastBrightness = 0;  // 缓存上次设置的亮度值
    } cachedMonitorInfo;
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    bool useCachedHandle = false;
    
    // 检查是否可以使用缓存的句柄（相同显示器且在10秒内）
    if (cachedMonitorInfo.displayIndex == displayIndex && 
        cachedMonitorInfo.hPhysicalMonitor != NULL &&
        (currentTime - cachedMonitorInfo.lastUseTime) < 10000) {
        useCachedHandle = true;
        
        // 如果亮度值相同，则直接返回成功，无需发送DDC命令
        if (cachedMonitorInfo.lastBrightness == brightness) {
            return true;
        }
    }
    
    if (useCachedHandle) {
        // 使用缓存的监视器句柄和亮度范围，避免重复获取
        DWORD targetBrightness = cachedMonitorInfo.minimumBrightness + 
            (DWORD)((cachedMonitorInfo.maximumBrightness - cachedMonitorInfo.minimumBrightness) * (brightness / 100.0));
        
        // 设置亮度
        BOOL result = SetMonitorBrightness(cachedMonitorInfo.hPhysicalMonitor, targetBrightness);
        
        // 更新上次使用时间和亮度值
        cachedMonitorInfo.lastUseTime = currentTime;
        cachedMonitorInfo.lastBrightness = brightness;
        
        return result != FALSE;
    } else {
        // 需要重新获取监视器句柄
        if (cachedMonitorInfo.hPhysicalMonitor != NULL) {
            // 关闭之前缓存的句柄
            DestroyPhysicalMonitor(cachedMonitorInfo.hPhysicalMonitor);
            cachedMonitorInfo.hPhysicalMonitor = NULL;
        }
        
        /* 重新获取显示器句柄 - 优化版本 */
        DISPLAY_DEVICE displayDevice;
        displayDevice.cb = sizeof(DISPLAY_DEVICE);
        
        // 获取主显示器句柄 - 对于单显示器系统会更快
        HWND desktopHwnd = GetDesktopWindow();
        HMONITOR hMonitor = MonitorFromWindow(desktopHwnd, MONITOR_DEFAULTTOPRIMARY);
        
        // 如果是多显示器系统且不是主显示器
        if (displayIndex > 0) {
            int foundIndex = 0;
            for (DWORD deviceIndex = 0; EnumDisplayDevices(NULL, deviceIndex, &displayDevice, 0); deviceIndex++) {
                if (!(displayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
                    continue;
                }
                
                /* 创建一个DC用于MonitorFromWindow */
                HDC hdc = CreateDC(NULL, displayDevice.DeviceName, NULL, NULL);
                if (!hdc) {
                    continue;
                }
                
                // 使用MonitorFromWindow代替MonitorFromDC，因为MonitorFromDC在某些Windows头文件版本中可能未定义
                HMONITOR currentMonitor = MonitorFromWindow(desktopHwnd, MONITOR_DEFAULTTOPRIMARY);
                DeleteDC(hdc);
                
                if (!currentMonitor) {
                    continue;
                }
                
                if (foundIndex == displayIndex) {
                    hMonitor = currentMonitor;
                    break;
                }
                
                foundIndex++;
            }
        }
        
        if (!hMonitor) {
            return false;
        }
        
        /* 获取物理监视器数量 */
        DWORD physicalMonitorCount = 0;
        if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &physicalMonitorCount) || physicalMonitorCount == 0) {
            return false;
        }
        
        /* 分配物理监视器数组 */
        PHYSICAL_MONITOR* physicalMonitors = new PHYSICAL_MONITOR[physicalMonitorCount];
        
        /* 获取物理监视器 */
        if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, physicalMonitorCount, physicalMonitors)) {
            delete[] physicalMonitors;
            return false;
        }
        
        // 通常第一个物理监视器就是我们需要的
        HANDLE hPhysicalMonitor = physicalMonitors[0].hPhysicalMonitor;
        
        /* 获取亮度范围 */
        DWORD minimumBrightness = 0, currentBrightness = 0, maximumBrightness = 100;
        GetMonitorBrightness(hPhysicalMonitor, &minimumBrightness, &currentBrightness, &maximumBrightness);
        
        /* 映射亮度到监视器实际范围 */
        DWORD targetBrightness = minimumBrightness + 
            (DWORD)((maximumBrightness - minimumBrightness) * (brightness / 100.0));
        
        /* 设置亮度 */
        BOOL result = SetMonitorBrightness(hPhysicalMonitor, targetBrightness);
        
        // 缓存这个监视器句柄和亮度范围信息
        cachedMonitorInfo.displayIndex = displayIndex;
        cachedMonitorInfo.hMonitor = hMonitor;
        cachedMonitorInfo.hPhysicalMonitor = hPhysicalMonitor;
        cachedMonitorInfo.minimumBrightness = minimumBrightness;
        cachedMonitorInfo.maximumBrightness = maximumBrightness;
        cachedMonitorInfo.lastUseTime = currentTime;
        cachedMonitorInfo.lastBrightness = brightness;
        
        // 关闭其他不需要的句柄
        for (DWORD i = 1; i < physicalMonitorCount; i++) {
            DestroyPhysicalMonitor(physicalMonitors[i].hPhysicalMonitor);
        }
        
        delete[] physicalMonitors;
        
        return result != FALSE;
    }
}

/* Windows平台获取当前亮度 */
int DdcController::getCurrentBrightnessWin(int displayIndex) const
{
    if (displayIndex < 0 || displayIndex >= displays.size() || !displays[displayIndex].supportsDdc) {
        return -1;
    }
    
    /* 使用缓存的亮度值 */
    return displays[displayIndex].currentBrightness;
}
#endif

#ifdef Q_OS_LINUX
/* Linux平台检测DDC/CI支持 */
bool DdcController::detectDdcSupportLinux()
{
    /* Linux平台实现代码 */
    qDebug() << "Linux平台检测DDC/CI支持尚未实现";
    return false;
}

/* Linux平台设置亮度 */
bool DdcController::setBrightnessLinux(int displayIndex, int brightness)
{
    /* Linux平台实现代码 */
    qDebug() << "Linux平台设置亮度尚未实现";
    return false;
}

/* Linux平台获取当前亮度 */
int DdcController::getCurrentBrightnessLinux(int displayIndex) const
{
    /* Linux平台实现代码 */
    qDebug() << "Linux平台获取亮度尚未实现";
    return -1;
}
#endif

/* 强制设置支持状态 */
void DdcController::setForcedSupportStatus(bool supported)
{
    /* 不再强制设置支持状态，仅输出调试信息 */
    qDebug() << "强制设置DDC/CI支持状态功能已禁用，忽略设置值:" << supported;
} 