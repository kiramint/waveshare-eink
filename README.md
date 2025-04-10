# Waveshare E-INK Xredirect

适用于Waveshare 2.13inch Touch e-Paper HAT与Orangpi3b的XServer重定向驱动

* 将墨水瓶的输入输出重定向到了XServer，可直接运行Qt等GUI框架运行的程序
* 采用Xvfb虚拟X11，并使用libx11获取Xvfb内图像，使用libxtst虚拟鼠标点击

## 编译此程序

* 安装依赖（可能不全）

  ```shell
  sudo apt install build-essential cmake ninja-build libopencv-dev libx11-dev libxtst-dev liblgpio-dev 
  ```
  

> 克隆时可能需要`--recursive`，此项目依赖：https://github.com/kiramint/waveshare_eink_2in13_v4_opi3b

* 编译

  ```shell
  mkdir build
  cd build
  cmake .. -G Ninja
  cmake --build . -j4
  ```

## 使用方式

* 直接带上要启动的X11App

	```shell
	~ -> ./E-Paper 
	Usage: ./E-Paper <x11-Apps> [args...]
	```

