homeassistant推荐使用docker容器安装，无需考虑复杂的依赖关系树。

其次在docker中运行时，其网络需要指定为“host”模式，否则局域网内无法找到设备，其他设备也无法发现homeassistant。

```dockerfile
docker run --init -d --name="home-assistant" --restart=always -e "TZ=Asia/Shanghai" -v /PATH:/config --net=host homeassistant/home-assistant:latest
```

另外，如果已经配置过mqtt并成功连接后，mqtt服务中断会导致homeassistant所有实例不可用。

