# alsa player & capture

## 编译
编译前先确认设备名称，并修改 `ALSA_DEVICE` 为对应的设备名称

```sh
$ gcc -o alsa_player alsa_player.c -lasound
$ gcc -o alsa_capture alsa_capture.c -lasound
```

## 测试
```sh
$ ./alsa_capture test.pcm
$ ./alsa_player test.pcm
```
