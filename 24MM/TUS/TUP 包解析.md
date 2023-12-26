<div align='center'>TUP 包解析</div><div align='center'></
##  1、TUP 总体结构 

<font color='red'>TODO：大小端信息还未确定怎么拿</font>

　![image-20231201170909855](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201170909855.png)

![image-20231201162611302](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201162611302.png)

### 1.1 数据类型

![image-20231204180738696](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231204180738696.png)

"Byte string"（字节字符串）是一种数据类型，用于表示一串二进制数据。它可以看作是字节数组，其中每个元素都是一个字节。在不同的编程语言和数据格式中，字节字符串的表示方式可能会有所不同。

在Java中，字节字符串可以使用`byte[]`类型来表示，即一个字节数组。可以通过将每个字节的值存储在数组中的不同位置来表示整个字节字符串。

在其他编程语言中，字节字符串可能以不同的形式表示，如Python中的`bytes`类型、C++中的`std::string`类型等。这些类型通常提供了一些方法和函数来操作字节字符串，例如访问、修改、拼接、比较等。

字节字符串通常用于处理二进制数据，例如文件读写、网络通信、加密解密等。由于字节字符串直接表示二进制数据，因此可以更灵活地处理各种格式的数据，而不仅限于文本数据。

## 2、FH 

从FH中解析获得VH　ＶＦ　ＦＦ　的位置



当前FH只有<font color='red'>FixeHeader</font>类型

![image-20231201162517971](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201162517971.png)

<font color='red'>TODO：数据类型没有具体定义</font>

![image-20231204184500996](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231204184500996.png)



## 3、VH



![image-20231201162929521](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201162929521.png)





## 4、VF

VF的数据结构由FH中定义的类型决定。目前，<font color='red'>TLV格式只能用作VH类型</font>

VF 中的index索引指明了VF中InnerPackage的位置，先解析index拿到所有的InnerPackage后再逐个解析

 ![image-20231205161200432](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231205161200432.png)

该类型包含0或多个带长度范围的TLV格式数据，<font color='red'>内包的顺序和VH type: Fixed Order(Version 1)相同</font>，
TLV 格式定义如下：

- Index
- Name
- Domain
- Compression(whole)
- Compression(fixed size block)
- Compression(variable size block)
- Encryption method
- ICV-TREE
- ICV-RRAY
- Process order(under consideration)
- Update flow(under consideration)
- DELTA-PATCH
- ICV-AFTER-DATA
- ICV-BLOCK

　![image-20231201172547143](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201172547143.png)

```


可变页脚（VF）
存储元数据的区域（例如用于解压缩或验证每个区域的数据以及TUP文件本身的元数据）。
VF由TLV格式的记录组成。
类型和偏移量对的列表可以作为TLV包含在VF的第一部分，其类型为索引。
即使有许多lnner包，也不需要检查所有现有的
TLV按顺序排列，因为存在lnner Packagemetadata的索引信息。
这一结构有助于实现以下目标。
[目标数据泄露预防
TUp文件中的数据的解密和解压缩信息基本上作为TLV记录包含在VF中。并且，在被加密的情况下，VF本身的解密信息包含在FF中
[明显损坏和抗篡改性
VF区域包含在ICV树的范围内，可以是专用的，也可以与其他人共享。
并且受到保护而不被篡改。 
```



此格式包含0个或多个TLV格式数据，其大小定义在FH(Fixed Header)中。
请参阅类型详细信息：
•索引：与VH的Index TLV格式相同
•内包信息



　![image-20231201165334954](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201165334954.png)

　![image-20231201170516972](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201170516972.png)

对于VF的解析，先拿到ｉｎｄｅｘ，ｉｎｄｅｘ中告知了ｎａｍｅ，ｖｅｒｓｉｏｎ，Ｄｏｍａｉｎ．．．．．ＩＣＶ－ＢＬＯＣＫ的偏移量和大小，即告知了每个小包的所有信息

<font color='red'>TODO需确认：</font>VF中记录的包的顺序和VH中记录包的顺序是否一直，如果不一致，该如何将ＶＦ中记录的包的名字等信息与VH中记录的包的偏移量和大小对应起来



　![image-20231201165443959](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201165443959.png)