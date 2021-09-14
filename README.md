# 魅族遥控器蓝牙网关
[![Stable](https://img.shields.io/github/v/release/georgezhao2010/esp32_meizu_remoter_gateway)](https://github.com/georgezhao2010/esp32_meizu_remoter_gateway/releases/latest)

一个ESP3232固件，用于魅族遥控器的网关，可以将魅族遥控器的温湿度数据通过局域网进行推送，同时还支持通过局域网控制魅族遥控器发送和接收红外数据。

魅族遥控器如下图：

![remoter](https://user-images.githubusercontent.com/27534713/131553205-0a2ce801-df07-4943-ba96-bc730ce44635.jpg)

# 特点
- 零代码/零配置文件/零命令行操作
- 手机App配网，并支持重新配置，修改Wi-Fi不需要重新烧录固件
- 与遥控器智能绑定，可以随时向网关添加遥控器绑定或者移除遥控器，增减遥控器不需要烧录固件
- 通过通讯传送需要发送的红外码，改变红外码不需要烧录固件
- 可以绑定多个遥控器，轮询查询各遥控器的温湿度数据并通过网络推送
- 网络中可以存在多个蓝牙网关，每个绑定不同遥控器，实现全屋覆盖
- 固件基于[ESP-IDF](https://github.com/espressif/esp-idf)编写，TTL日志输出，方便定位问题

# 下载和烧录

克隆本仓库，或者下载最新的Release，在build目录下有一个esp32_meizu_remoter_gateway.bin文件，这就是ESP32的固件，请下载并使用[esphome-flasher](https://github.com/esphome/esphome-flasher/releases/latest)烧录进ESP32中。

由于烧录过程中，esphome-flasher可能会访问网络获取分区表和bootloder，如果是中国国内网络，可能会显示网络超时错误，关掉esphome-flasher并重开，多尝试几次即可。

# 基本概念定义
图中是比较常见的ESP32开发板NodeMCU-32S。
![esp32](https://user-images.githubusercontent.com/27534713/131534909-ef80cda9-1f67-4032-b2b1-15576f4d1030.jpg)
## 功能键
上方红圈是GPIO0的下拉微动开关，按住可以下拉GPIO0的电平，我们在下文中称为功能键，用于网关的操作。
## 状态灯
下方红圈的蓝色LED接在GPIO2，我们在下文中称为状态灯，用来表示网关的状态。状态灯有以下状态。
- **常亮**, 状态灯一直亮的状态，表示此时无线网络连接正常。
- **常灭**, 如果电源指示灯(红色LED)正常点亮，而状态灯一直为灭的状态，表示此时无线网无法连接，可以检查无线路由器或者重启网关。
- **慢闪**, 此时状态灯2秒钟闪烁一次，我们在下文中称为慢闪，具体含义下文会提及。
- **快闪**, 此时状态灯每秒钟闪烁5次，我们在下文中称为快闪，具体含义下文会提及。

*注意:使用非NodeMCU-32S的其它ESP32模块，也许需自行处理GPIO0/GPIO2的输入输出*


# 配网
魅族遥控器网关需要Espressif官方的无线网络配网工具，iOS手机请在App Store中搜索ESP SoftAP Provisioning，Android手机请搜索ESP SoftAP Provisioning并下载安装。

## 初次配网
- 刚烧录好固件并启动后，状态灯为慢闪状态，此时用手机搜索ESP_XXXXXX的无线网并加入，密码为"ESP32xAP"。
- 打开ESP SoftAP Provisioning手机App，点按"Provision Device"按钮。
- 在二维码配网步骤，直接点按"I don't have a QR code"
- PIN码输入界面，输入PIN码"p1d32a94"
- 在选择无线网界面，直接选择无线网，并输入密码。如果无法搜索无线网，点击"Join Other Network"，并在弹出的窗口输入需要连接的无线网SSID及密码，并点击"Connect"
- 如果前述步骤没有错误，网关状态灯进入快闪状态，此时网关设备将重启，重新启动后，状态灯如果成功连接上无线网，会显示为常亮状态。

## 重新配网
在任意状态下，按住功能键10秒钟，状态灯进入快闪状态，网关的配网信息将被清除，重启后又将进入配网状态，配置的步骤与初次配网操作相同。

*注意:按住功能键10秒，是初始化所有设置，除无线网配置信息被清除以外，绑定的全部遥控器也将都被移除。*


# 绑定遥控器
在网关工作时，需要知道采集哪些遥控器的信息，可以通过绑定与解绑操作，告知网关需要采集的遥控器的蓝牙地址集合。

## 绑定
- 在网关进入正常工作模式(非配网模式)时，按住功能键2秒钟，网关进入绑定模式，此时状态灯为慢闪状态。
- 拿起一个遥控器并尽量靠近网关，网关如果发现这个设备，状态灯会快闪2秒，并退出绑定模式，该遥控器绑定成功。

或 在[魅族遥控器网关集成](https://github.com/georgezhao2010/meizu_remoter_gateway)中调用bind服务指定进入绑定状态

## 解绑单个遥控器
此部分操作需要在[魅族遥控器网关集成](https://github.com/georgezhao2010/meizu_remoter_gateway)中进行操作，见[解除绑定](https://github.com/georgezhao2010/meizu_remoter_gateway#%E8%A7%A3%E9%99%A4%E7%BB%91%E5%AE%9A)部分的描述。

## 解绑全部遥控器
与重新配网相同，按住功能键10秒钟，状态灯进入快闪状态，所有绑定的遥控器均会被清除。

*注意:按住功能键10秒，是初始化所有设置，除绑定的全部遥控器会被移除外，无线网配置信息也将被清除。*
*注意:原则上不推荐不同网关绑定同一个遥控器*

# 数据轮询与更新

网关开始工作后，按照设定的时间间隔轮询所有绑定的遥控器设备，并将轮询的结果数据推送到连接的所有客户端。默认的轮询时间为5分钟，这一间隔可以在[魅族遥控器网关集成](https://github.com/georgezhao2010/meizu_remoter_gateway)中进行修改，见[选项](https://github.com/georgezhao2010/meizu_remoter_gateway#%E9%80%89%E9%A1%B9)部分的描述。

如果一个遥控器在轮询时失败，将会向客户端推送其不可用状态。如果连续5次轮询失败，该设备将会被移除出轮询列表（不会解除绑定关系），在下次重新启动网关之前，不再更新该设备的数据。这种情况可能是遥控器设别故障或电池耗尽。在排除故障后，将设备与网关重新绑定，可恢复数据的更新。

# 红外发送与接收
## 发送
支持按照传送的key与ircode,让遥控器发送指定红外信号，当遥控器发送完成后，网关会快闪1秒
## 接收(Beta)
可以使网关进入20秒的红外接收状态，收到的红外数据将以蓝牙Notify的形式输出在TTL。据观察，遥控器似乎并不能准确接收红外数据，时灵时不灵，接收到的数据也奇形怪状，有待验证。

# 网络服务与通讯
## mDNS
魅族遥控器网关支持mDNS服务,服务名称为`_meizu_remoter_gateway._tcp.local.`,使用zeroconf可获得以下格式信息
```
type='_meizu_remoter_gateway._tcp.local.', name='1C4BD9._meizu_remoter_gateway._tcp.local.', addresses=[b'\xc0\xa8\x01\x87'], port=8266, weight=0, priority=0, server='1C4BD9.local.', properties={b'version': b'0.1.0', b'serialno': b'1C4BD9'}
```

## TCP
魅族遥控器网关开放并监听8266端口，支持多用户连接，payload形式为JSON。

### 网关接收消息
- 获取网关配置信息
```
{
  "type": "config_info"
}
```
- 订阅消息推送，只有向网关发送该消息，才会收到消息推送
```
{
  "type": "subscribe"
}
```
- 使网关进入遥控器绑定状态(最长30秒)
```
{
  "type": "bind"
}
```
- 调整绑定操作信号阈值
```
{
  "type": "bindthrld",
  "data":{
    "threshold":-35
  }
}
```
- 移除已经绑定的遥控器
```
{
  "type":"removebind",
  "data":{
    "device":"ab:6d:12:34:56:ef"
  }
}
```
- 心跳, 如3分钟内无心跳或其它消息，客户端将被断开
```
{
  "type":"heartbeat"
}
```
- 设置更新间隔(分钟)
```
{
  "type":"setinterval",
  "data":{
    "update_interval":10
  }
}
```
- 发射红外
```
{
  "type":"irsend",
  "data":{
    "device":"ab:2d:12:34:56:ef",
    "key":"65001C63C68D8000C8",
    "ircode":"0123456789abcdef..."
  }
}
```
- 接收红外
```
{
  "type":"irrecv",
  "data":{
    "device":"ab:2d:12:34:56:ef"
  }
}
```

### 网关回应消息
- 获取网关配置信息回应
```
{
  "type":"config_info",
  "data":{
    "version":"0.3.0",
    "serialno":"1B923A",
    "update_interval":5
  }
}
```
- 绑定状态回应
```
{
  "type": "bind",
  "data":{
    "status":1
  }
}
```
- 心跳回应
```
{
  "type":"heartbeat"
}
```

### 网关推送消息
- 设备状态更新
```
{
  "type":"update",
  "data":{
    "device":"12:62:33:b5:9f",
    "available":1,
    "status":{
      "manufacturer":"Meizu",
      "model":"R16",
      "fireware":"v_3.0.10.23",
      "temperature":32.62,
      "humidity":75.33,
      "battery":90,
      "voltage":2.9,
      "rssi":-47
    }
  }
}
```
- 更新间隔变化推送
```
{
  "type":"setinterval",
  "data":{
    "update_interval":10
  }
}
```
- 设备移除推送
```
{
  "type":"removebind",
  "data":{
    "device":"ab:6d:12:34:56:ef"
  }
}
```

关于网关与网关集成的更多操作，请参阅[魅族遥控器网关集成](https://github.com/georgezhao2010/meizu_remoter_gateway/README.md)的说明。
