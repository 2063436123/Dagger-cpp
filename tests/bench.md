有一个问题，如果不重启服务器，那么webbench会越测越慢。开始时16000多susceed，第二次就1200多次，第三次就只有97次了。 暂时先取第一次测试的结果作为记录。

命令：`webbench -c 1 -t 5  http://10.211.55.4:12345/`
结果：`Speed=196260 pages/min, 206073 bytes/sec. Requests: 16355 susceed, 0 failed.`

命令：`webbench -c 2 -t 5  http://10.211.55.4:12345/`
结果：`Speed=180096 pages/min, 189100 bytes/sec. Requests: 15008 susceed, 0 failed.`
