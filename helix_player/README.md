# minimp3 player

采用 [https://github.com/ultraembedded/libhelix-mp3](https://github.com/ultraembedded/libhelix-mp3) 解码 MP3， 并使用 alsa 播放输出

## 编译

```shell
$ rm -rf build && cmake -S. -B build -G "Ninja" && cmake --build build
```

或

```shell
$ rm -rf build && mkdir build && cd build && cmake .. && make
```

或

```shell
$ rm -rf build && cmake -S. -B build && cmake --build build
```


## 运行
```shell
$ ./build/minimp3_player LAST_DANCE.mp3
```