<div align='center'>  http通信</div>

## 1.http基础知识

### 1.1 简介

HTTP（Hypertext Transfer Protocol）是一种用于传输超文本的应用层协议。它是构建万维网（World Wide Web）的基础，并在客户端和服务器之间进行通信。

1. 客户端-服务器模型：
   HTTP 是一种经典的客户端-服务器模型的协议。客户端发送 HTTP 请求，而服务器接收并响应该请求。客户端可以是浏览器、移动应用或其他发送 HTTP 请求的应用程序；而服务器则是存储和提供资源的计算机。

2. 无状态协议：
   HTTP 是一种无状态协议，即服务器不会保存之前请求的相关信息。每个请求都是相互独立的，服务器仅根据当前请求来处理，并返回相应的响应。但为了实现状态管理，可以使用 Cookie、Session 等机制。

3. 请求-响应模式：
   每个 HTTP 交互由一个请求和一个响应组成。客户端发送一个请求给服务器，服务器处理该请求并返回一个响应。请求包括请求方法、URI、协议版本、请求头部和请求体；响应则包括状态码、响应头部和响应体。

4. URI（Uniform Resource Identifier）：
   URI 是一个唯一标识符，用于标识互联网上的资源。在 HTTP 请求中，URI 指定了要访问的资源的位置和名称。它由三部分组成：协议（如 HTTP、HTTPS）、主机名（如 www.example.com）和资源路径（如 /index.html）。

5. 请求方法：
   请求方法指示客户端要对资源执行的操作。常见的请求方法有 GET、POST、PUT、DELETE。例如，GET 用于获取资源，POST 用于提交数据，PUT 用于上传文件，DELETE 用于删除资源。

6. 状态码：
   响应中的状态码表示服务器对请求的处理结果。常见的状态码有 200 OK（请求成功）、404 Not Found（未找到资源）、500 Internal Server Error（服务器错误）等。

7. 请求头部和响应头部：
   请求头部包含了与请求相关的信息，如 User-Agent（客户端身份标识）、Content-Type（请求体的数据类型）。响应头部则包含了与响应相关的信息，如 Content-Type（响应体的数据类型）、Content-Length（响应体的长度）。

8. 持久连接和管道化：
   HTTP/1.1 引入了持久连接的概念，允许多个 HTTP 请求和响应通过同一个 TCP 连接传输，以减少连接建立和关闭的开销。HTTP/1.1 还支持管道化，即在没有等待前一个响应的情况下发送多个请求。

9. 加密通信：
   HTTP 可以使用安全套接层（SSL）或传输层安全（TLS）来加密通信，形成 HTTPS。HTTPS 使用公钥加密和私钥解密的方式，确保通信的机密性和数据的完整性。

HTTP 协议提供了简单、灵活和可扩展的方式来传输超文本。它广泛用于万维网中，支持浏览器访问网页、传输数据和与服务器进行交互。在开发 Web 应用程序时，了解 HTTP 协议是非常重要的，因为它涉及到网络通信和数据传输的基本原理。

### 1.2 http请求

#### 1.2.1 报文组成

 ![image-20230802144831280](C:\Users\jiangzhuangzhuang\study\study\24MM\TLS\http通信.assets\image-20230802144831280.png)

HTTP 请求报文是客户端向服务器发送的请求的格式。它由请求行、请求头部和请求体组成。下面是对 HTTP 请求报文的详细介绍：

1. 请求行：
   请求行包含了请求方法、请求目标和 HTTP 版本。通常由三个部分组成，使用空格进行分隔：
   ```
   <Method> <Request-Target> <HTTP-Version>
   ```
   - <font color='red'>Method：表示请求的 HTTP 方法，如 GET、POST、PUT、DELETE 等</font>。

   ```
   GET：用于获取资源，在服务器端不会对资源进行修改。
   
   POST：用于向服务器提交数据，并在服务器端创建一个新的资源。
   
   PUT：用于向服务器上传数据，并替换服务器端指定的资源。该请求需要客户端提供完整的资源表示，并将其替换服务器上对应的资源。
   
   DELETE：用于删除服务器上的资源。
   ```

   - Request-Target：表示请求的目标资源的路径，可以是绝对路径或相对路径。
   - HTTP-Version：表示使用的 HTTP 协议的版本号，如 HTTP/1.1。

2. 请求头部：
   请求头部包含了关于请求的附加信息，以键值对的形式表示。每个键值对占据一行，以冒号进行分隔，如：
   
   ```
   <Header-Name>: <Header-Value>
   ```
   常见的请求头部包括：
   - Host：表示服务器的主机名和端口号。
   - User-Agent：表示客户端的身份标识。
   - Content-Type：表示请求体的数据类型。
- Content-Length：表示请求体的长度。
  
3. 空行：
   空行用于分隔请求头部和请求体，在请求头部结束后加上一个空行。

4. 请求体：
   请求体包含了需发送给服务器的数据。它位于空行之后，并根据请求头部中的 Content-Type 来解析数据格式。常见的格式有：
   - application/x-www-form-urlencoded：用于发送表单数据。
   - multipart/form-data：用于上传文件和二进制数据。
   - application/json：用于发送 JSON 格式的数据。

HTTP 请求的使用方法可以通过使用各种编程语言或框架来实现。一般来说，可以使用 HTTP 客户端库（如 Java 中的 HttpClient、JavaScript 中的 axios）来创建请求对象，设置请求方法、URL、请求头部和请求体等参数，然后发送请求并获取服务器的响应。

以下是一个示例的 HTTP 请求报文：

```
POST /api/users HTTP/1.1
Host: example.com
User-Agent: Mozilla/5.0
Content-Type: application/json
Content-Length: 56

{"username":"john_doe","email":"john.doe@example.com"}
```

这个示例使用 POST 方法向 `/api/users` 路径发送请求，请求头部包含了 Host、User-Agent、Content-Type 和 Content-Length 等信息，请求体中包含了一个 JSON 格式的数据。

需要根据具体的编程语言和框架来构建和发送 HTTP 请求，以满足自己的需求。

## 2.安卓http通信实现

### 2.1 简介

基于 OkHttp + Retrofit + RxJava 实现

OkHttp、Retrofit 和 RxJava 是在 Android 开发中经常同时使用的三个强大的库，它们可以协同工作，提供了一种高效、简洁和可扩展的方式来进行网络请求和数据处理。

1. OkHttp：OkHttp 是一个基于 HTTP/2 和 SPDY 的网络请求库，提供了强大而灵活的 API。它支持同步和异步请求，可以自动处理连接池、压缩、缓存等网络层细节，使得网络请求更加高效和稳定。

2. Retrofit：Retrofit 是一个基于 OkHttp 的 RESTful HTTP 网络请求库，它通过简洁的注解方式将 HTTP API 定义为独立的接口。它提供了自动序列化和反序列化支持，可以将 HTTP 响应数据转换为 Java 对象并以流式的方式处理。Retrofit 还支持多种数据转换器，如 Gson、Jackson 等，方便地处理不同类型的数据格式。

3. RxJava：RxJava 是一个基于反应式编程思想的库，它提供了异步编程的解决方案。结合 Retrofit 和 RxJava，可以将网络请求转换为 Observable 对象，并使用 RxJava 提供的操作符对这些 Observable 对象进行变换、过滤和组合。这样可以实现更强大、更灵活的异步流处理逻辑，使代码更加清晰和可维护。

使用 OkHttp + Retrofit + RxJava 的优点：

1. 简化网络请求：Retrofit 封装了很多网络请求的细节，提供了简洁而直观的 API，使得网络请求的编写更加方便和高效。

2. 异步处理：结合 RxJava，可以轻松地将网络请求转换为流式的 Observable 对象，并使用操作符对其进行异步处理，如切换线程、过滤数据等。

3. 错误处理：RxJava 提供了丰富的错误处理机制，可以灵活地处理网络请求中可能出现的错误情况，并提供相应的错误处理逻辑。

4. 可组合性：RxJava 提供了强大的操作符，可以将多个网络请求进行组合，实现复杂的请求逻辑，如请求依赖、并发请求等。

使用 OkHttp + Retrofit + RxJava 的基本流程如下：

1. 定义一个包含各种请求方法的接口，使用 Retrofit 的注解来描述请求的 URL、请求方式、请求参数等。

2. 使用 Retrofit 创建该接口的实例，并设置相应的配置，如 OkHttp 客户端、Gson 转换器等。

3. 在请求方法上添加适当的 RxJava 的操作符，如 `subscribeOn()`、`observeOn()`、`map()` 等，对请求进行配置和处理。

4. 创建观察者对象，用于处理请求结果。

5. 调用请求方法并订阅观察者，触发网络请求并处理响应结果。

总之，OkHttp + Retrofit + RxJava 是一套前后端交互的完整解决方案，能够简化和优化 Android 开发中的网络请求和数据处理的代码。它们提供了强大的功能和灵活的机制，帮助开发者提升开发效率，并保证了网络请求的可靠性和稳定性。

### 2.2 OkHttp + Retrofit + RxJava 

#### 2.2.1 OkHttp 





#### 2.2.2 Retrofit 

Retrofit主要负责应用层面的封装，就是说主要面向开发者，方便使用，比如请求参数，响应数据的处理，错误处理
等等。
Retrofit封装了具体的请求，线程切换以及数据转换。
一般都推荐RxJava+Retrofit+OkHttp框架，Retrofit负责请求的数据和请求的结果，使用接口的方式呈现，
OkHttp负责请求的过程，RxJava负责异步，各种线程之间的切换，用起来非常便利

![img](C:\Users\jiangzhuangzhuang\study\study\24MM\TLS\http通信.assets\20210407144559955.png)

大体的网络流程是一致的，毕竟都是通过okhttp进行网络请求。主要的步骤都是：创建网络请求实体client->构建真
正的网络请求-> 将网络请求方案与真正的网络请求实体结合构成一个请求Call->执行网络请求->处理返回数据->处理
Android 平台的线程问题。
在上图中，我们看到的对比最大的区别是什么？
0）okhttp创建的是OkhttpClient，然而retrofit创建的是 Retrofit实例
1）构建蓝色的Requet的方案，retrofit是通过注解来进行的适配
2）配置Call的过程中，retrofit是利用Adapter适配的Okhttp 的Call
3）相对okhttp，retrofit会对responseBody进行 自动的Gson解析
4）相对okhttp，retrofit会自动的完成线程的切换。


#### 2.2.3  RxJava 的消息订阅和线程切换原理          

https://mp.weixin.qq.com/s/GBGlttLgQA2XrMcRTUUTOw



### 2.3 代码实操

这是一个使用 Retrofit + RxJava 的示例代码，其中包含一个名为 `campaignStatusNotification` 的方法。该方法接受一些参数，并返回一个 Observable<Boolean> 对象。

在该示例中，假设已经定义了一个 `RequestInterface` 接口，它包含一个 `campaignStatusNotification` 方法用于发送网络请求。`userAgent`、`vin`、`date` 和 `request` 是该方法的参数。

```java
public interface RequestInterface {
    @POST("campaign/status/notification")
    Observable<String> campaignStatusNotification(@Header("User-Agent") String userAgent,
                                                  @Query("vin") String vin,
                                                  @Query("date") String date,
                                                  @Body VsmDeviceEvents request);
}

public class RetrofitManager {
    private static final String LOG_ID_RETROFIT_MANAGER = "RetrofitManager";
    private RequestInterface requestInterface;

    // 初始化 RetrofitManager
    public RetrofitManager() {
        // 创建 OkHttpClient 实例
        OkHttpClient client = new OkHttpClient.Builder()
                .build();

        // 创建 Retrofit 实例
        Retrofit retrofit = new Retrofit.Builder()
                .baseUrl(BASE_URL)
                .client(client)
                .addCallAdapterFactory(RxJava2CallAdapterFactory.create())
                .addConverterFactory(GsonConverterFactory.create())
                .build();

        // 创建 RequestInterface 实例
        requestInterface = retrofit.create(RequestInterface.class);
    }

    public Observable<Boolean> campaignStatusNotification(String userAgent,
                                                          String vin,
                                                          String date,
                                                          VsmDeviceEvents request) {
        Log.d(LOG_ID_RETROFIT_MANAGER, "campaignStatusNotification");
        return requestInterface.campaignStatusNotification(userAgent, vin, date, request)
                .subscribeOn(Schedulers.io())
                .map(new Function<String, Boolean>() {
                    @Override
                    public Boolean apply(String s) {
                        try {
                            JsonObject jobj = checkError("campaignStatusNotification", s);
                            if (null == jobj) {
                                return false;
                            }

                            return true;
                        } catch (Exception e) {
                            e.printStackTrace();
                            return false;
                        }
                    }
                });
    }
}
```

在这个示例中，Retrofit 使用 RxJava 的适配器工厂 `RxJava2CallAdapterFactory`，以支持返回 Observable 对象。在 `campaignStatusNotification` 方法中，我们首先通过调用 `requestInterface.campaignStatusNotification()` 来发送网络请求，并获取到返回的 Observable<String> 对象。然后，我们使用 `subscribeOn(Schedulers.io())` 指定了请求运行的线程，以及 `map()` 方法将请求结果转换为一个 Boolean 值。

`map()` 方法中的函数将检查请求返回的字符串是否包含错误信息，并根据判断结果返回相应的 Boolean 值。如果出现异常，会打印堆栈跟踪信息，并返回 false。

注意：上述示例代码中的一些变量和方法是假设存在的，你需要根据实际情况进行调整和实现。

希望这个示例能够帮助你理解如何使用 Retrofit + RxJava 进行网络请求，并对返回结果进行流式处理。



