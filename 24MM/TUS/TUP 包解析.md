<div align='center'>TUP 包解析</div><div align='center'></
##  1、TUP 总体结构 
## 1、整体结构

<font color='red'>TODO：大小端信息还未确定怎么拿</font>

　![image-20231201170909855](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201170909855.png)

![image-20231201162611302](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201162611302.png)

## 1.1 数据类型

![image-20231204180738696](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231204180738696.png)

"Byte string"（字节字符串）是一种数据类型，用于表示一串二进制数据。它可以看作是字节数组，其中每个元素都是一个字节。在不同的编程语言和数据格式中，字节字符串的表示方式可能会有所不同。

在Java中，字节字符串可以使用`byte[]`类型来表示，即一个字节数组。可以通过将每个字节的值存储在数组中的不同位置来表示整个字节字符串。

在其他编程语言中，字节字符串可能以不同的形式表示，如Python中的`bytes`类型、C++中的`std::string`类型等。这些类型通常提供了一些方法和函数来操作字节字符串，例如访问、修改、拼接、比较等。

字节字符串通常用于处理二进制数据，例如文件读写、网络通信、加密解密等。由于字节字符串直接表示二进制数据，因此可以更灵活地处理各种格式的数据，而不仅限于文本数据。

## 1.2　TLV格式

TLV（Type-Length-Value）是一种常用的数据编码格式，用于在计算机系统中表示和传输结构化数据。它由三个字段组成：

1. **Type（类型）**：用于标识数据的类型或者含义。通常使用一个固定长度的数值或者字节来表示。不同的类型可以代表不同的数据结构或者操作。
2. **Length（长度）**：表示后面 Value 字段的长度。长度可以是固定的，也可以是可变的，具体取决于所传输的数据。
3. **Value（值）**：存储实际的数据内容。它的长度由 Length 字段指定。

TLV 格式的优点在于它的灵活性和可扩展性。它<font color='red'>允许将不同类型的数据组织在一起，并且可以根据需要进行扩展，以适应不同的数据结构和需求</font>。

例如，考虑一个简单的 TLV 数据结构，用于表示一个人的信息：

+---------+--------+--------------+
|  Type   | Length |    Value     |
+---------+--------+--------------+
|  Name   |   6    |   Alice     |
+---------+--------+--------------+
|  Age    |   1    |     25      |
+---------+--------+--------------+
| Address |   10   | 123 Main St |
+---------+--------+--------------+

　![image-20231214203855224](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231214203855224.png)

#### 1.2.1　TLV格式中type的取值：

　　![image-20231214204038863](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231214204038863.png)



## 2、FH 

从FH中解析获得VH　ＶＦ　ＦＦ　的位置，然后再分别解析VH　ＶＦ　ＦＦ



当前FH只有<font color='red'>FixeHeader</font>类型

![image-20231201162517971](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231201162517971.png)

<font color='red'>TODO：数据类型没有具体定义</font>

![image-20231204184500996](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231204184500996.png)



## 3、VH

<font color='red'>从VH中解析获得所有内置包的偏移量和大小</font>

VH有两种结构：

1、Fixed　order

2、TLV　

　![image-20231214203204075](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231214203204075.png)

#### 3.1、Fixed　order

![image-20231214203139013](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231214203139013.png)

<font color='cornflowerblue'>按照offset　size　成对顺序排列，顺序解析即可</font>



#### 3.2、TLV格式

目前我的理解是：

index索引数据块记录了所有“Location　information　ｏｆ　Inner　Package”数据块的位置



先解析type类型为index的TLV数据块，从中获取所有的type类型为“Location　information　ｏｆ　Inner　Package”数据块的位置。拿到后逐个解析“Location　information　ｏｆ　Inner　Package”数据块<font color='red'>从而获得所有包的offset和size（即所有包的位置）</font>

![image-20231214203436572](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231214203436572.png)

　![image-20231214203513113](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231214203513113.png)

　![image-20231214203534729](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231214203534729.png)



## 4、VF

VF的数据结构由FH中定义的类型决定。目前，<font color='red'>TLV格式只能用作VH类型</font>

VF 中的index索引指明了VF中InnerPackage的位置，先解析index拿到所有的InnerPackage后再逐个解析

 ![image-20231205161200432](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231205161200432.png)

该类型包含0或多个带长度范围的TLV格式数据，<font color='red'>内包的顺序和VH type: Fixed Order(Version 1)相同</font>，

<font color='red'>TODO：这些值的类型都没定义</font>

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

## 5、FF

 ![image-20231226204945141](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231226204945141.png)



## 6、完整性检查  ICV（Integrity Check Value）

### 6.1 背景知识

ICV树和ICV数组用于检测数据的篡改和损坏，是用于生成和检查ICV以确保数据完整性的数据结构。

ICV树：
ICV树是通过以下步骤获得的ICV的分层集合：

1. 将原始数据分成一定大小的块。
2. 计算每个块的ICV。
3. 如果ICV集合的大小大于块的大小，则重复上述过程以计算集合的ICV。
4. ICV集合存储在数据的页眉/页脚之外的任何区域。
5. ICV集合的元数据以TLV（类型-长度-数值）格式存储在VF（验证字段）或页眉/页脚之外的其他区域，并且类型为“ICV树”。
6. 每个内部包的ICV树的元数据可以存储在VF中，从而可以最小化VF的大小和其验证成本，因为ICV集合本身不存储在VF中。

ICV数组：
ICV数组作为类型为“ICV数组”的TLV记录，存储在页眉/页脚之外的任何区域。TLV值包括三个元素：
1. TUp文件中的起始位置：指示TUp文件中受保护区域的起始位置。
2. TUp文件中的大小：指定受保护区域的大小。
3. ICV：针对TUp文件中特定区域计算的完整性检查值。

ICV树和ICV数组都提供了保护数据完整性并便于处理需要随机访问的连续和非连续区域的机制。

### 6.2 整体结构



 ![image-20231222115303256](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222115303256.png)



<font color='red'>我的理解：</font>

结合资料我的理解：这样设计目的是校验每个数据块的完整性（信息是否有被篡改）。具体做法是对每个inner-PKG 计算ICV值保存在ICV-TREE 中，将所有的ICV-TREE的信息合入到ICV-ARRAY中。对ICV-ARRAY再进行ICV计算，得到Root ICV，RooICV的信息存储在FF中。



<font color='red'>具体校验做法：</font>

对应这样的结构，完整性校验时，分三种情况

#### 1、VF中读到的Type details为ICV-BLOCK

<font color='red'>TODO：</font>
1）表中的数据类型未定义

2）表中IC V算法逻辑没定义范围（会用到哪些算法）

 ![image-20231222121702430](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222121702430.png)

从Offset和size组合获取到需要完整性检验的数据，根据ICV algorithm和Key information 计算该数据的ICV值，然后与ICV_BLOCK数据块中的ICV值（这里的ICV值大小是由ICV算法决定的）对比，确定数据是否完整。

#### 2、VF中读到的Type details为ICV-TREE

<font color='red'>TODO：表中的数据类型未定义</font>

 ![image-20231222150747793](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222150747793.png)

​    ![image-20231222152412289](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222152412289.png)

 1)、通过offset和size获得被检测的数据，将其按照blocksize分块计算出每一块的icv值

2）、将所有icv值组合在一起，再按照blocksize分块，计算出每一块的icv值。

3）、重复2）直到获得最后一个icv，即Top ICV

4）、通过ICV-TREE offset获得TOP ICV的位置，与3）中计算出的Top ICV比较，确定数据的完整性和合法性

#### 3、VF中读到的Type details为ICV-ARRAY

<font color='red'>TODO：表中的数据类型未定义</font>

 ![image-20231222155650590](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222155650590.png)

 ![image-20231222155740292](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222155740292.png)

<font color='red'>重点关注offset</font>

 ![image-20231222155841940](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222155841940.png)

offset指明了被检测数据的位置，前16位是保留位，后48位是值。

参考下<font color='red'>6.2整体结构</font>

1、最高位（即第63位）为0时表示被检测的数据是原始数据，这时候直接计算icv值，　

例如由Offset1和size1获得被检测数据，计算出IC V值，然后对比ICV1，验证数据是否完整合法。



2、最高位（即第63位）为1时表示被检测的数据是ICV－TREE或者ICV-ARRAY，这时候就需要嵌套处理了。

就是说被检测的数据还是　![image-20231222161144231](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222161144231.png)

俗称<font color='red'>套娃</font>

<font color='red'>TODO:</font>ICV -TREE类型下，ICV-TREE结构中已经有一个top icv了，可以和我计算出的icv进行比较。为什么ICV-ARRAY中![image-20231226155737228](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231226155737228.png)还有一个ICV，这两个冲突吗。



#### 4、FF中校验Root ICV

![image-20231222171343707](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222171343707.png)

1、验证签名：利用SIGN algorithm ，SIGN block size， Key information3 ，SIGN来验证

2、验证Root ICV：利用![image-20231222172420887](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222172420887.png)

算出Root ICV并与![image-20231222172507069](C:\Users\jiangzhuangzhuang\study\study\24MM\TUS\TUP 包解析.assets\image-20231222172507069.png) 比较。



