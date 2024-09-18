
Jukey 目标构建一个具备实时音视频、直播、录制和点播能力为一体的全栈音视频平台，并具备与 SIP、WebRTC 等互联互通的能力。

# 基础库

## 组件库
- 类似 COM 组件模型
- 支持组件创建、跟踪、销毁等全生命周期管理
- 跨平台
- 组件动态加载

## 网络库
- 基于 Libevent 实现
- 支持多线程
- 提供网络层和会话层编程接口
- 会话层支持能力协商、保活、重连等
- 支持可靠会话和不可靠会话
- 可靠 UDP 会话支持 FEC 特性
- 可靠 UDP 会话支持拥塞控制和平滑发包


# 音视频引擎

## Streamer
- 基于 Element 和 Pin 的 Pipeline 框架
- 支持媒体能力协商
- 提供音视频同步机制
- 统一的线程管理
- 统一的通信机制
- 提供开箱即用的音视频采集、编解码、渲染等 Element

## Media Engine
- 基于 Streamer 封装完整音视频能力
- 支持本地摄像头预览
- 支持播放媒体文件
- 音视频传输
- 设备管理

## RTC Engine
- 基于 Media engine 封装实时音视频能力
- 支持设备注册
- 支持用户登录
- 支持加入分组
- 支持发布和订阅媒体流
- 支持分组内广播媒体

# 服务器
- 微服务架构
- 流式分发架构
- 信令和媒体分离
- 媒体转发和媒体处理分离
- 基于 ServiceBox 的服务加载机制
- 支持分离部署和单进程集中部署
- 支持配置中心
- 基于交换模型的信令转发架构

# 音视频特性
- 支持 H264
- 支持 OPUS
- 支持 FEC（RS）
- 支持 NACK
- 支持 WebRTC GCC
- 支持 SDL
- 支持 Media Foundation

# 项目结构
- build
    - window - windows 平台编译
    - linux - linux 平台编译
- src
    - base - 基础库
        - com-frame - 组件库
        - net-frame - 网络库
    - common
        - protocol - 协议定义
        - public - 公共依赖
        - util - 工具
    - component
        - amqp-client - RabbitMQ client
        - pinger - 服务存活检测
        - property - 组件化参数封装
        - reporter - 依赖上报
        - timer - 定时器
        - tracer - 调用链
    - demo
        - rtc-client-demo - 实时音视频测试客户端
    - media
        - common - 媒体处理公共依赖
        - congestion-control - 拥塞控制（GCC）
        - device-manager - 设备管理
        - element - 包含音视频采集、编解码、渲染等各种音视频处理元素
        - media-util - 媒体处理工具
        - public - 媒体处理公共依赖
        - streamer - Pipeline 实现
        - transport - 音视频传输实现
    - sdk
        - media-engine - 媒体引擎
        - media-player - 播放器
        - rtc-engine - RTC 引擎
    - service
        - common - 公共依赖
        - group-service - 分组服务
        - proxy-service - 代理服务
        - route-service - 路由服务
        - service-box - 服务加载与运行底座
        - stream-service - 流管理服务
        - terminal-service - 终端管理服务
        - transport-service - 音视频传输服务
        - user-service 用户服务
- test - 测试程序
- utest - 单元测试
- third-party - 第三方依赖

# 运行
### 服务器
- windows

```
service-box.exe
```
- linux
```
service-box
```
#### 配置

<details>
<summary>service-box 配置</summary>

<div style="width:100%;">

|配置项|解释|
|---|---|
| component-path | 组件路径，默认与可执行文件在相同路径 |
| load-config-interval | 配置中心相关，暂时不用关注 |
| name | 服务名称 |
| cid | 服务的 CLASS ID，service-box 基于 cid 加载并启动服务组件 |
| config | 服务的配置文件，service-box 向服务组件传入指定配置文件 |

</div>

```yaml
# load components from
component-path: ./

# interval of loading loop while service configure files are not ready, in second
load-config-interval: 3

services:
  -
    name: route-service
    cid: cid-route-service
    config: ./service-config/route-service.yaml
  -
    name: proxy-service
    cid: cid-proxy-service
    config: ./service-config/proxy-service.yaml
  -
    name: user-service
    cid: cid-user-service
    config: ./service-config/user-service.yaml
  -
    name: group-service
    cid: cid-group-service
    config: ./service-config/group-service.yaml
  -
    name: stream-service
    cid: cid-stream-service
    config: ./service-config/stream-service.yaml
  -
    name: transport-service
    cid: cid-transport-service
    config: ./service-config/transport-service.yaml
  -
    name: terminal-service
    cid: cid-terminal-service
    config: ./service-config/terminal-service.yaml
```

</details>

### 客户端

```
rtc-client-demo.exe
```

#### 配置
```xml
<?xml version="1.0" encoding="UTF-8" standalone="no"?>

<root>
  <app-id>123456</app-id>
  <client-id>12345</client-id>
  <!-- Service address -->
  <server-addr>TCP:192.168.28.201:8888</server-addr>
  <client-name>test-client</client-name>
</root>

```