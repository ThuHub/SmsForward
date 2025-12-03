# SmsForward

# 一、项目概述

本项目使用ESP32 C3 SuperMini开发板配合搭载中移物联ML307A-GSLN Cat.1模块的**核心板**，使用DAPlink作为调试和烧录工具，VSCode + PlatformIO作为开发环境，实现短信自动转发到企业微信机器人的功能。  

- 亦可使用USB转TTL作为调试和烧录工具，使用Arduino IDE作为开发环境  
- 核心板ML307A-GSLN不是唯一选择，也不是性价比最高的选择，模组选型请移步[**中国移动 OneMo**](http://onemo10086.com/#/product/menu?id=18)
- 本项目不是成本最低的方案，低成本成品方案参见[**双卡槽4G短信转发器**](https://e.tb.cn/h.SzFXl1JT7RTe6Sz?tk)

# 二、硬件准备

### 1. 所需材料清单：
| 序号 | 名称                    | 数量  |   成本   | 备注           |
|:--:|:----------------------|:---:|:------:|:-------------|
| 1  | ESP32 C3 SuperMini开发板 | 1个  |  9.5   | 主控制器         |
| 2  | ML307A-GSLN核心板        | 1个  |   42   | Cat.1通信模块    |
| 3  | DAPlink调试器            | 1个  | ~~18~~ | 烧录和调试        |
| 4  | 已激活的SIM卡              | 1张  |   /    | 移动/联通/电信     |
| 5  | 5V/2A电源适配器            | 1个  |   /    | 长期供电         |
| 6  | 母对母杜邦线                | 10根 |   1    | 连接线          |
| 7  | USB Type-C数据线         | 1条  |   /    |  DAPlink连接电脑 |  

### 2. 设备引脚说明：
#### ESP32 C3 SuperMini引脚：
- 3.3V：3.3V电源输出
- 5V：5V电源输入
- GND：接地
- GPIO4：SWDIO接口
- GPIO5：SWCLK接口
- GPIO6：UART接收
- GPIO7：UART发送
- GPIO20：串口接收
- GPIO21：串口发送  

#### ML307A-GSLN核心板引脚：
- VCC：电源输入（5V-16V）
- GND：接地
- TX：串口发送
- RX：串口接收
- ~~PWRKEY：电源键（可选）~~

#### DAPlink引脚：
- 3.3V：3.3V电源输出
- GND：接地
- SWDIO：SWD数据线
- SWCLK：SWD时钟线
- TX：串口发送
- RX：串口接收

# 三、硬件连接步骤

### 第一步：连接DAPlink与ESP32 C3
1. 使用杜邦线连接DAPlink的3.3V引脚到ESP32 C3的3.3V引脚
2. 连接DAPlink的GND引脚到ESP32 C3的GND引脚
3. 连接DAPlink的SWDIO引脚到ESP32 C3的GPIO4引脚
4. 连接DAPlink的SWCLK引脚到ESP32 C3的GPIO5引脚
5. 连接DAPlink的TX引脚到ESP32 C3的GPIO20引脚
6. 连接DAPlink的RX引脚到ESP32 C3的GPIO21引脚

### 第二步：连接ESP32 C3与ML307A核心板
1. 连接ESP32 C3的GPIO6引脚到ML307A模块的TX引脚
2. 连接ESP32 C3的GPIO7引脚到ML307A模块的RX引脚
3. 连接ESP32 C3的5V引脚到LDO稳压器的输入端
4. 连接LDO稳压器的输出端（调整为4.0V）到ML307A模块的VCC引脚
5. 连接ESP32 C3的GND引脚到ML307A模块的GND引脚

### 第三步：连接电源
1. 用USB Type-C连接DAPlink到电脑

### 第四步：插入SIM卡
1. 关闭所有电源
2. 将已激活的SIM卡插入ML307A模块的卡槽
3. 确保SIM卡插入方向正确  

# 四、软件环境安装

### 第一步：安装Visual Studio Code
1. 访问 https://code.visualstudio.com/
2. 点击"Download for Windows"（根据你的操作系统选择）
3. 运行下载的安装程序
4. 全部选择默认设置，点击"下一步"直到安装完成
5. 安装完成后启动VSCode

### 第二步：安装PlatformIO扩展
1. 打开VSCode，点击左侧的扩展图标（四个方块）
2. 在搜索框中输入"PlatformIO IDE"
3. 找到"PlatformIO IDE"扩展，点击"安装"按钮
4. 等待安装完成（约2-5分钟）  
_安装完成后，VSCode左下角会出现一个蚂蚁头图标_

### 第三步：安装DAPlink驱动
1. 将DAPlink连接到电脑的USB接口
2. 等待Windows自动安装驱动（如果失败则继续下一步）
3. 下载Zadig驱动工具：https://zadig.akeo.ie/
4. 运行Zadig工具
5. 点击菜单"Options" → "List All Devices"
6. 在下拉菜单中选择"DAPlink CMSIS-DAP"
7. 点击"Install Driver"按钮
8. 等待驱动安装完成

### 第四步：检查DAPlink是否识别成功
1. 打开设备管理器（Win+X，选择设备管理器）
2. 展开"端口(COM和LPT)"选项
3. 应该看到"DAPlink CMSIS-DAP"设备，记住COM号（如COM3）
4. 同时应该看到一个"DAPlink Drive"的磁盘驱动器

# 五、创建PlatformIO项目

### 第一步：创建新项目
1. 点击VSCode左下角的蚂蚁图标（PlatformIO）
2. 点击"Open" → "PIO Home" → "New Project"
3. 填写项目信息：
4. Name: SMS_Forwarder
5. Board: 搜索框中输入"ESP32-C3"，选择"Espressif ESP32-C3-DevKitM-1"
6. Framework: 选择"Arduino"
7. Location: 使用默认位置
8. 点击"Finish"按钮  
_等待项目创建完成（会自动下载相关文件，需要几分钟）_

### 第二步：配置项目文件
1. 在VSCode左侧文件浏览器中，找到项目文件夹
2. 用本项目中的"platformio.ini"文件替换本地同名文件

### 第三步：修改串口端口号
1. 打开设备管理器，查看DAPlink的COM端口号
2. 将platformio.ini文件中的monitor_port值改为实际的COM端口号
3. 保存

# 六、编写程序代码

### 第一步：编写主程序代码
1. 在VSCode左侧文件浏览器中，展开项目文件夹
2. 展开"src"文件夹
3. 用本项目中的“main.cpp”文件替换本地同名文件

### 第二步：修改配置信息
在代码中找到以下三行，修改为你的实际信息并保存

```C++
// 第22行：改为你的WiFi名称
const char* wifi_ssid = "你的WiFi名称";

// 第23行：改为你的WiFi密码
const char* wifi_password = "你的WiFi密码";

// 第26行：改为你的企业微信机器人Webhook地址
const char* wechat_webhook = "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=你的机器人key";
```

> #### 如何获取企业微信机器人Webhook地址：
> 1. 打开企业微信电脑版
> 2. 创建一个群聊或选择已有群聊
> 3. 点击右上角"..."添加群机器人
> 4. 点击"新创建一个机器人"
> 5. 输入机器人名称
> 6. 点击"添加"按钮
> 7. 复制生成的Webhook地址  

# 七、编译和烧录程序

### 第一步：编译程序
1. 确保VSCode左下角显示"esp32-c3-devkitm-1"
2. 点击VSCode左下角的"→"图标（在蚂蚁图标旁边）
3. 选择"Build"选项
4. 等待编译完成，输出窗口显示：

```text
Building .pio/build/esp32-c3-devkitm-1/firmware.bin
========================= [SUCCESS] Took 5.12 seconds =========================
```

### 第二步：烧录到ESP32 C3
1. 确保DAPlink已连接到电脑
2. 确保ESP32 C3已通电（连接5V电源）
3. 点击VSCode左下角的"→"图标（上传按钮）
4. 等待烧录过程，输出窗口显示：

```text
Uploading .pio/build/esp32-c3-devkitm-1/firmware.bin
========================= [SUCCESS] Took 8.45 seconds =========================
```

> #### 如果烧录失败：
> 1. 按住ESP32 C3的BOOT按钮不放
> 2. 点击上传按钮开始烧录
> 3. 当看到烧录进度开始时，松开BOOT按钮  

### 第三步：打开串口监视器
1. 点击VSCode左下角的插头图标（串口监视器）
2. 串口监视器会打开并显示程序输出
3. 应该看到类似以下信息：

```text
========================================
   ESP32 C3 短信转发系统启动
========================================
[1/4] 初始化ML307A模块串口...
[2/4] 连接WiFi网络...
正在连接WiFi: 你的WiFi名称
....
WiFi连接成功!
IP地址: 192.168.1.100
[3/4] 初始化ML307A模块...
发送AT指令: AT
超时，收到: 
错误：ML307A模块无响应，请检查接线！
[4/4] 系统初始化完成！
等待接收短信...
========================================
```

# 八、测试和验证

### 第一步：检查硬件连接
如果看到"ML307A模块无响应"，请检查：
1. ML307A模块的VCC引脚是否为4.0V
2. TX和RX接线是否正确（ESP32 GPIO6→ML307A TX，ESP32 GPIO7→ML307A RX）
3. 重新插拔SIM卡
4. 重启系统

### 第二步：发送测试短信
1. 用手机向ML307A模块中的SIM卡发送一条短信
2. 内容可以是："测试短信123"
3. 观察串口监视器输出，应该看到：

```text
收到模块数据: +CMTI: "SM",1
检测到新短信通知！
正在读取新短信...
收到短信数据
开始解析短信...
短信索引: 1
发送者: +8613800138000
时间: 24/01/01,12:00:00+32
内容: 测试短信123
准备转发到企业微信...
发送JSON数据: {"msgtype":"text","text":{"content":"📱 收到新短信\n👤 发件人: +8613800138000\n🕐 时间: 24/01/01,12:00:00+32\n📄 内容: 测试短信123"}}
转发成功，HTTP代码: 200
删除短信，索引: 1
```

### 第三步：检查企业微信
1. 打开企业微信
2. 进入机器人所在的群聊
3. 应该看到机器人发送的短信通知

### 第四步：验证系统稳定性
1. 发送多条测试短信
2. 检查每条都能正确转发
3. 保持系统运行1小时，检查是否稳定

# 九、长期运行设置

### 第一步：固定硬件连接
1. 将杜邦线换成焊接连接
2. 使用铜柱固定双板
3. 使用热熔胶固定连接处
4. 将板子放入合适的亚克力盖板中
5. 远离热源

### 第二步：优化电源
1. 使用质量好的5V/2A电源适配器
2. 插在长期闲置的插头上