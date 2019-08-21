### 镜头定义

```
enum RS_SCENE
{
    TEA_FEA = 0,        //教师特写      0
    STU_FEA,            //学生特写      1
    TEA_FULL,           //教师全景      2
    STU_FULL,           //学生全景      3
    BB_FEATURE,         //板书特写      4
    PC_CAPTURE,         //电脑画面      5
    MAIN,               //主画面        6
};

```

### 开始录制
***GET /start_record***
###### 映射关系 key:RS_SCRENE value:录制参数
```
//request body,资源模式下
[
    {
        "6": {
            "filename": "./main.mp4",
            "need_to_segment": false,
            "segment_duration": 0
        }
    },
    {
        "0": {
            "filename": "./tea_fea.mp4",
            "need_to_segment": false,
            "segment_duration": 0
        }
    },
    {
        "1": {
            "filename": "./stu_fea.mp4",
            "need_to_segment": false,
            "segment_duration": 0
        }
    },
    {
        "2": {
            "filename": "./tea_full.mp4",
            "need_to_segment": false,
            "segment_duration": 0
        }
    },
    {
        "3": {
            "filename": "./stu_full.mp4",
            "need_to_segment": false,
            "segment_duration": 0
        }
    },
    {
        "4": {
            "filename": "./black_board.mp4",
            "need_to_segment": false,
            "segment_duration": 0
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


//request body,普通模式,只有MAIN(RS_SCENE)有效
[
    {
        "6": {
            "filename": "./main.mp4",
            "need_to_segment": false,
            "segment_duration": 0
        }
    }
]
```

### 停止录制
***GET /stop_record***

### 开始本地直播
***GET /start_local_live***
###### 映射关系 key:RS_SCRENE value:直播参数
```
[
    {
        "6": {
            "url": "rtmp://127.0.0.1/live/main"
        }
    },
    {
        "0": {
            "url": "rtmp://127.0.0.1/live/tea_fea"
        }
    },
    {
        "1": {
            "url": "rtmp://127.0.0.1/live/stu_fea"
        }
    },
    {
        "2": {
            "url": "rtmp://127.0.0.1/live/tea_full"
        }
    },
    {
        "3": {
            "url": "rtmp://127.0.0.1/live/stu_full"
        }
    },
    {
        "4": {
            "url": "rtmp://127.0.0.1/live/black_board"
        }
    },
    {
        "5": {
            "url": "rtmp://127.0.0.1/live/pc"
        }
    }
]
```
### 停止本地直播
***GET /stop_local_live***

### 开始远程直播
***GET /start_remote_live***

```
{
    "url" : "rtmp://127.0.0.1/live/main2"
}
```
### 停止远程直播
***GET /stop_remote_live***

### 主画面拼接定义(通过此参数判断是否资源模式)

```
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


### 切换拼接模式(直播开启时,设置完成后会重启直播,录制则不会自动重启)
***GET /start_remote_live***
###### 映射关系 key:对应显示位置,例如FIVE具有0,1,2,3,4位置 value:RS_SCENE
```
{
    "mapping": [
        {
            "0": 3
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
    "mode": 6
}
```
 
### PC采集模式定义
```
typedef enum ADV7842_CMODE_E
{
	MODE_HDMI,  //HDMI 0
	MODE_VGA    //VGA  1
} ADV7842_MODE;
```


### 切换PC采集模式
***GET /change_pc_capture***
```
{
"mode":0
}
```


