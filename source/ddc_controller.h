#ifndef DDC_CONTROLLER_H
#define DDC_CONTROLLER_H

#include <QObject>
#include <QVector>

/* 定义亮度控制相关宏，如果Windows API中未定义 */
#ifdef Q_OS_WIN
#ifndef MC_CAPS_BRIGHTNESS
#define MC_CAPS_BRIGHTNESS 0x00000001
#endif
#endif

/**
 * @brief DDC/CI 控制器类
 * 用于检测显示器是否支持DDC/CI功能并控制显示器亮度
 */
class DdcController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取DdcController单例
     * @return DdcController实例
     */
    static DdcController* getInstance();

    /**
     * @brief 检测显示器是否支持DDC/CI
     * @return 如果至少有一个显示器支持则返回true
     */
    bool detectDdcSupport();

    /**
     * @brief 获取支持DDC/CI的显示器数量
     * @return 支持DDC/CI的显示器数量
     */
    int getSupportedDisplayCount() const;

    /**
     * @brief 设置显示器亮度
     * @param displayIndex 显示器索引
     * @param brightness 亮度值 (0-100)
     * @return 是否设置成功
     */
    bool setBrightness(int displayIndex, int brightness);

    /**
     * @brief 获取显示器当前亮度
     * @param displayIndex 显示器索引
     * @return 当前亮度值 (0-100)，失败时返回-1
     */
    int getCurrentBrightness(int displayIndex) const;
    
    /**
     * @brief 强制设置支持状态（仅用于调试）
     * @param supported 是否支持
     */
    void setForcedSupportStatus(bool supported);
    
    /**
     * @brief 设置优化模式
     * @param optimize 是否启用优化模式（减少DDC/CI命令频率，但可能影响精确度）
     */
    void setOptimizeMode(bool optimize);

private:
    /* 私有构造函数，防止外部创建实例 */
    explicit DdcController(QObject *parent = nullptr);
    
    /* 单例实例 */
    static DdcController* instance;
    
    /* 存储支持DDC/CI的显示器信息 */
    struct DisplayInfo {
        int index;
        bool supportsDdc;
        int currentBrightness;
        qint64 lastUpdateTime;  // 上次更新时间
    };
    
    QVector<DisplayInfo> displays;
    
    /* 初始化并检测显示器 */
    void initialize();
    
    /* 是否已强制设置了支持状态 */
    bool forcedSupport;
    bool isForceSet;
    
    /* 优化模式标志 */
    bool optimizeMode;
    
    /* 最小更新间隔（毫秒） */
    static const int MIN_UPDATE_INTERVAL = 20;

#ifdef Q_OS_WIN
    /* Windows平台特定函数 */
    bool detectDdcSupportWin();
    bool setBrightnessWin(int displayIndex, int brightness);
    int getCurrentBrightnessWin(int displayIndex) const;
#elif defined(Q_OS_LINUX)
    /* Linux平台特定函数 */
    bool detectDdcSupportLinux();
    bool setBrightnessLinux(int displayIndex, int brightness);
    int getCurrentBrightnessLinux(int displayIndex) const;
#endif
};

#endif // DDC_CONTROLLER_H 