<div align='center'>Gbook接口QA</div>

1、notifyServiceFlagChanged 通知服务断开？

 ![image-20230829163450280](C:\Users\jiangzhuangzhuang\study\study\24MM\TLS\Gbook接口使用QA.assets\image-20230829163450280.png)

2、 ![image-20230829171526456](C:\Users\jiangzhuangzhuang\study\study\24MM\TLS\Gbook接口使用QA.assets\image-20230829171526456.png)

应答Code详细指的是云端返回来的错误码吗？  参考 Server Push Spec.pdf 表4：レスポンスコードとリトライ要否 / Talbe4 : Response code and retry 是这本式样书中的errocode吗？

3、Gbook请求云端时多长时间是超时。以下场景是否存在：

我以id=1发起请求，并设置了超时时间30s，超时后我以id=2重新发起请求，此时Gbook返回了id=1的回复？

4、

 ![image-20230829181343585](C:\Users\jiangzhuangzhuang\study\study\24MM\TLS\Gbook接口使用QA.assets\image-20230829181343585.png)

type说明中没看到提及ota，ota用哪个？

seat当前席位什么意思？

