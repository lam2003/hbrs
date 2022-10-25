# 海思录播系统

此系统基于海思hi3531/hi3532 PCIE级联板开发，用于课堂录播场景，软件由以下两部分组成，分别运行在两种SOC之上。

* [hi3531](https://github.com/lam2003/hbrs)
* [hi3532](https://github.com/lam2003/hbrs_3532)


## 硬件规格

| 名称    | 数量 | 用途                  |
| ------- | ---- | --------------------- |
| HI3531  | 1    | 主片                  |
| HI3532  | 2    | 从片                  |
| ADV7842 | 1    | 一路HDMI/VGA信号采集  |
| TW6874  | 3    | 六路SDI摄像头信号采集 |
| AIC3106 | 1    | 麦克风信号采集        |

## 主要功能模块

* 云台控制
* 接收导播（跟踪主机）信号
* OSD字幕
* 多布局画面（画中画、多屏拼接等）
* RTMP直播推流
* MP4录制

## 编译方法

1. 准备交叉工具链(arm-hisiv400-linux-gcc)
2. 在源码根目录下准备3rdparty目录，存放交叉编译后的第三方库
3. 使用cmake生成编译脚本进行编译

## HTTP接口定义

本系统提供HTTP接口，供web端调用

### 画面编号

```cpp
enum RS_SCENE
{
    TEA_FEA = 0,        //教师特写      0
    STU_FEA,            //学生特写      1
    TEA_FULL,           //教师全景      2
    STU_FULL,           //学生全景      3
    BB_FEA,             //板书特写      4
    PC_CAPTURE,         //电脑画面      5
    MAIN,               //主画面        6
};
```

### 开始录制

> GET /start_record

```json
    [
                                          
        {                                 // 可同时录制多个画面
            "6": {                        // 6代表RS_SCENE中定义的画面
                "filename": "./main.mp4", // 录制文件名，只需填写相对路径
                "need_to_segment": false, // 是否需要对长视频进行分段
                "segment_duration": 0     // 分段时长，单位秒
            }
        },
        {
            "5": {
                "filename": "./pc.mp4",
                "need_to_segment": false,
                "segment_duration": 0
            }
        }
    ]
```

### 停止录制

> GET /stop_record

### 开始直播

> GET /start_local_live

```json
[
    {                                           // 可同时直播多个画面
        "6": {                                  // 6代表RS_SCENE中定义的画面
            "url": "rtmp://127.0.0.1/live/main" // 直播推流地址
        }
    },
    {
        "5": {
            "url": "rtmp://127.0.0.1/live/pc"
        }
    }
]
```

### 停止直播

> GET /stop_local_live

### 画面布局定义

```cpp
enum Mode
{
    NORMAL_MODE, //单画面       0   普通模式
    PIP_MODE,    //画中画       1   普通模式
    TWO,         //双屏拼接     2   资源模式
    THREE,       //三屏拼接     3   资源模式
    FOUR,        //四屏拼接     4   资源模式
    FOUR1,       //四屏拼接1    5   资源模式
    FIVE,        //五屏拼接     5   资源模式
    SIX,         //六屏拼接     6   资源模式
    SIX1         //六屏拼接1    7   资源模式
};
```

### 改变画面绑定关系

> GET /change_display_screen

```json
{
    "mapping": [
        {
            "0": 3  // 0代表六屏拼接中的0号区域，3代表RS_SCENE中的画面类型
        },
        {
            "1": 5
        },
        {
            "2": 2
        },
        {
            "3": 0
        },
        {
            "4": 4
        },
        {
            "5": 1
        }
    ],
    "mode": 6       // 对应Mode中的六屏拼接
}
```
