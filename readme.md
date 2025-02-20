<!--
 * @Date: 2025-02-06 21:38:00
 * @LastEditors: thomas-smith123 thomas-smith@live.cn
 * @LastEditTime: 2025-02-07 09:40:19
 * @FilePath: \undefinedc:\jiangrd3\ADS-B-WAVEFORM-COLLECT\readme.md
-->
- 库的获取
```
git clone https://github.com/analogdevicesinc/libiio.git
cd libiio
git checkout 2023_R2
```
For linux
```
mkdir build && cd build
cmake ..
make -j1
```
windows同理
linux下保留.so文件，windows保留lib和dll，然后在qt中添加就行