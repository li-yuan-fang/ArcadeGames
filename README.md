# ArcadeGames

中文 | [English](./README_en.md)

## 这是什么？

ArcadeGames是一个基于CH585开发的支持USB 2.0高速模式的USB HID游戏手柄demo，实现了可扩展的多按键功能，支持低延迟（USB Highspeed）、可扩展的游戏手柄（Gamepad）功能。

~~可以用来打音游~~

## 技术特征

- 支持USB 2.0协议

- 支持High-Speed和Full-Speed两种速率模式

- 基于定时器机制实现了软件消抖

- 支持标准HID Gamepad和HID Vendor Defined两种设备模式

- 理论延迟可低至125μs

## 测试环境

为了方便，测试使用了CH585 EVB评估板和第三方的按键板模块。

> 当然还是更推荐自己打板焊接，那样比评估板便宜更多。
> 
> 第三方的按键板外置了上拉电阻，因此在代码中没有配置芯片的内部上拉，可按需修改。

实际在Windows 11下无驱动测试了osu!lazer的mania 4K模式，即使按键板的手感很差，但是实际游戏体验还是比一般的USB键盘好很多。

## 如何构建

这是一个使用WCH官方IDE——[MounRiver Studio](https://www.mounriver.com/)创建的工程，直接使用MounRiver Studio打开即可编译和烧录。

> 踩坑提示
> 
> 文件使用UTF-8编码，而非MRS默认的GBK编码，请注意。
> 
> CH58X系列芯片默认开启USB和串口烧录（按住Boot并打开开关即可）
> 
> 但是WCH Link的二线烧录需要先通过WCH ISP工具按住Boot连接并解锁才可使用
