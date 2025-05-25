# minimp3 player

采用 [https://github.com/lieff/minimp3](https://github.com/lieff/minimp3) 解码 MP3， 并使用 alsa 播放输出

## 编译

```shell
$ cmake -S. -B build -G "Ninja" && cmake --build build
```

## 运行
```shell
$ ./build/minimp3_player LAST_DANCE.mp3
```