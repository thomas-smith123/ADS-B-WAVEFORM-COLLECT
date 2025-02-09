<!--
 * @Date: 2025-02-06 21:38:00
 * @LastEditors: thomas-smith123 thomas-smith@live.cn
 * @LastEditTime: 2025-02-07 09:40:19
 * @FilePath: \undefinedc:\jiangrd3\ADS-B-WAVEFORM-COLLECT\readme.md
-->
发现问题了，在构造函数上出现些问题，修改构造函数后就正常了。
目前是构造函数写在头文件里面是没有问题的，如下
```cpp
/*******header file*******/
adsb_decoder(TaskQueue *taskQueue, QObject *parent = nullptr): taskQueue(taskQueue){

};
```
而写成下面这样就不行
```cpp
/*******header file*******/
adsb_decoder(TaskQueue *taskQueue, QObject *parent = nullptr);
/*******source file*******/
adsb_decoder::adsb_decoder(TaskQueue *tasksQueue, QObject *parent): taskQueue(taskQueue)
{

}
```