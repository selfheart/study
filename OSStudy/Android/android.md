<div align='center'>android</div>

1.



2.





3.





4.





## 5.编译

### 5.1build.gradle

#### 5.1.1 简介

build.gradle 是一个配置文件，用于定义和配置 Gradle 构建系统的构建过程。在 Android Studio 中，你会看到两个 build.gradle 文件：一个是项目级别的 build.gradle，位于项目根目录下；另一个是模块级别的 build.gradle，位于每个模块的目录中。

1. 项目级别的 build.gradle（Project-level build.gradle）：
   - 这个文件定义了整个项目的全局配置。
   - 它包含了构建脚本的一般设置，如项目名称、Gradle 版本和构建脚本的仓库依赖等。
   - 在这个文件中，你可以配置项目范围的构建过程，例如构建类型、签名配置、构建变体等。
2. 模块级别的 build.gradle（Module-level build.gradle）：
   - 这个文件定义了每个模块（如应用模块、库模块等）的特定配置。
   - 它包含了模块的依赖关系、编译选项以及其他构建相关的设置。
   - 这个文件通常包含了 Android 插件的配置，用于指定应用程序的构建类型、产品风味（flavor）和构建变体（variant）等。
   - 在这个文件中，你可以添加依赖库、自定义构建任务、配置 ProGuard 规则等。

一般来说，你可以通过编辑 build.gradle 文件来定制和配置你的项目构建过程。你可以添加插件、指定依赖关系、设置构建选项、配置构建任务等。这样，你就能够根据项目的需求进行个性化的构建设置。

需要注意的是，修改 build.gradle 文件时要小心，确保语法正确并理解你所做的更改的影响。不正确的配置可能会导致构建失败或不可预测的行为。建议在修改文件之前备份原始文件，以防需要恢复到先前的配置状态。





#### 5.1.2 语法











#### 5.1.3  jar包相关

##### 5.1.3.1 make jar

通常情况下，你需要在 `makeJar` 任务内部配置一些属性和操作以生成所需的 JAR 文件。以下是一些可能的配置选项和操作：

1. 指定输出文件名：
   ```
   archiveFileName = "iauto-keystore-api.jar"
   ```

2. 指定源文件或目录：
   ```
   from "src/main/java" // 假设源文件在该目录下
   ```

3. 添加依赖关系，将其他任务的输出作为源文件：
   ```
   dependsOn tasks.named('compileJava') // 假设有一个名为 'compileJava' 的编译任务
   from tasks.named('compileJava').outputs.files
   ```

4. 排除某些文件或目录：
   ```
   exclude 'com/example/excluded/*.java' // 排除 com.example.excluded 包下的所有 Java 文件
   ```

5. 自定义 MANIFEST 文件：
   ```
   manifest {
       attributes 'Main-Class': 'com.example.MainClass'
   }
   ```

6. 自定义过滤规则：
   ```
   exclude '**/Test*.class' // 排除以 "Test" 开头的类文件
   ```

请根据你的具体需求，在 `makeJar` 任务中添加适当的配置和操作以生成所需的 JAR 文件。

#### 5.1.3.2 copy jar

```java
task makeJar(type: Jar) {
}

// 定义 copyJar 任务
task copyJar(dependsOn: [makeJar]) {
    def jarName  = "iauto-keystore-api.jar"
    def srcDir = "build/intermediates/aar_main_jar/release/"

    def destDirs = [
            "../updatecomposer/libs/"
    ]

    // 复制 Jar 文件到每个目标目录
    destDirs.each { dir ->
        doLast {
            println("Copying $jarName to $dir")
            copy {
                from "$srcDir"
                into dir
                include('classes.jar')
                rename ('classes.jar', "iauto-keystore-api.jar")
            }
        }
    }
}
```

### 6.security

https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedKeyStoreKeys

#### 6.1Android 密钥库系统

利用 Android  密钥库系统，您可以在容器中存储加密密钥，从而提高从设备中提取密钥的难度。在密钥进入密钥库后，可以将它们用于加密操作，而密钥材料仍不可导出。此外，它提供了密钥使用的时间和方式限制措施，例如要求进行用户身份验证才能使用密钥，或者限制为只能在某些加密模式中使用。如需了解详情，请参阅[安全功能](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SecurityFeatures)部分。



密钥库系统由 Android 4.0（API 级别 14）中引入的 `KeyChain` API、Android 4.3（API 级别 18）中引入的 Android 密钥库提供程序功能以及作为 Jetpack 的一部分提供的 [Security 库](https://android-dot-google-developers.gonglchuangl.net/topic/security/data?hl=zh-cn)使用。本文介绍何时以及如何使用 Android 密钥库提供程序。

##### 安全功能

Android 密钥库系统可以保护密钥材料免遭未经授权的使用。首先，Android 密钥库可以防止从应用进程和 Android  设备中整体提取密钥材料，从而避免了在 Android 设备之外以未经授权的方式使用密钥材料。其次，Android  密钥库可以让应用指定密钥的授权使用方式，并在应用进程之外强制实施这些限制，从而避免了在 Android 设备上以未经授权的方式使用密钥材料。

##### 提取防范

Android 密钥库密钥使用两项安全措施来避免密钥材料被提取：

- <font color='red'>密钥材料永不进入应用进程</font>。通过 Android  密钥库密钥执行加密操作时，应用会在后台将待签署或验证的明文、密文和消息馈送到执行加密操作的系统进程。如果应用进程受到攻击，攻击者也许能使用应用密钥，但无法提取密钥材料（例如，在 Android 设备以外使用）。
- 您可以将密钥材料绑定至 Android 设备的安全硬件，例如可信执行环境 (TEE) 和安全元件  (SE)。为密钥启用此功能时，其密钥材料永远不会暴露于安全硬件之外。如果 Android  操作系统受到攻击或者攻击者可以读取设备内部存储空间，攻击者也许能在 Android 设备上使用任意应用的 Android  密钥库，但无法从设备上提取这些数据。只有设备的安全硬件支持密钥算法、分块模式、填充方案和密钥有权配合使用的摘要的特定组合时，才可启用此功能。要检查是否为密钥启用了此功能，请获取密钥的 `KeyInfo` 并检查 `KeyInfo.isInsideSecurityHardware()` 的返回值。



##### 硬件安全模块

运行 Android 9（API 级别 28）或更高版本的受支持设备可拥有 StrongBox Keymaster，它是位于硬件安全模块中的 Keymaster HAL 的一种实现。该模块包含以下组成部分：

- 自己的 CPU。
- 安全存储空间。
- 真实随机数生成器。
- 可抵御软件包篡改和未经授权旁加载应用的附加机制。

检查存储在 StrongBox Keymaster 中的密钥时，系统会通过可信执行环境 (TEE) 证实密钥的完整性。

为支持低能耗的 StrongBox 实现，为一部分算法和密钥大小提供了支持：

- RSA 2048
- AES 128 和 256
- ECDSA P-256
- HMAC-SHA256（支持 8-64 字节密钥大小，含首末值）
- Triple DES 168

使用 [`KeyStore`](https://android-dot-google-developers.gonglchuangl.net/reference/java/security/KeyStore?hl=zh-cn) 类生成或导入密钥时，您需要通过将 `true` 传递给 [`setIsStrongBoxBacked()`](https://android-dot-google-developers.gonglchuangl.net/reference/android/security/keystore/KeyGenParameterSpec.Builder?hl=zh-cn#setIsStrongBoxBacked(boolean)) 方法，指示在 StrongBox Keymaster 中存储密钥的偏好。

**注意**：如果 StrongBox Keymaster 不适用于密钥的给定算法和关联的密钥大小，框架会抛出 [`StrongBoxUnavailableException`](https://android-dot-google-developers.gonglchuangl.net/reference/android/security/keystore/StrongBoxUnavailableException?hl=zh-cn)。

##### 密钥使用授权

为了避免在 Android 设备上以未经授权的方式使用密钥，在生成或导入密钥时，Android  密钥库会让应用指定密钥的授权使用方式。一旦生成或导入密钥，其授权将无法更改。然后，每次使用密钥时，都会由 Android  密钥库强制执行授权。这是一项高级安全功能，通常仅用于有以下要求的情形：在生成/导入密钥后（而不是之前或当中），应用进程受到攻击不会导致密钥以未经授权的方式使用。

支持的密钥使用授权分为以下几类：    

- 加密：授权密钥算法、运算或目的（加密、解密、签署、验证）、填充方案、分块模式以及可与密钥搭配使用的摘要；
- 时间有效性间隔：密钥获得使用授权的时间间隔；
- 用户身份验证：密钥只能在用户最近进行身份验证时使用。请参阅[要求进行用户身份验证才能使用密钥](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#UserAuthentication)。

作为一项额外的安全措施，对于密钥材料位于安全硬件内的密钥（请参阅 `KeyInfo.isInsideSecurityHardware()`），某些密钥使用授权可能会由安全硬件强制执行，具体取决于 Android 设备。加密和用户身份验证授权可能由安全硬件强制执行。由于安全硬件一般不具备独立的安全实时时钟，时间有效性间隔授权不可能由其强制执行。

您可以使用 `KeyInfo.isUserAuthenticationRequirementEnforcedBySecureHardware()` 查询密钥的用户身份验证授权是否由安全硬件强制执行。

## 选择密钥链或 Android 密钥库提供程序

如果您需要系统全局凭据，请使用 `KeyChain` API。在应用通过 `KeyChain` API 请求使用任何凭据时，用户需要通过系统提供的界面选择应用可以访问已安装的哪些凭据。因此，在用户同意的情况下多个应用可以使用同一套凭据。

使用 Android 密钥库提供程序让各个应用存储自己的凭据，并且只允许应用自身访问。通过这种方式，应用可以管理仅可供自身使用的凭据，同时所提供的安全优势还可媲美 `KeyChain` API 为系统全局凭据提供的安全优势。这一方法不需要用户选择凭据。

## 使用 Android 密钥库提供程序

​    如需使用此功能，请使用标准的 `KeyStore` 和 `KeyPairGenerator` 或 `KeyGenerator` 类，以及在 Android 4.3（API 级别 18）中引入的 `AndroidKeyStore` 提供程序。

`AndroidKeyStore` 在与`KeyStore.getInstance(type)` 方法搭配使用时注册为 `KeyStore` 类型，在与 `KeyPairGenerator.getInstance(algorithm, provider)` 和 `KeyGenerator.getInstance(algorithm, provider)` 方法搭配使用时注册为提供程序。

### 生成新私钥

生成新的 `PrivateKey` 还需要指定自签名证书具备的初始 X.509 属性。

[Security 库](https://android-dot-google-developers.gonglchuangl.net/topic/security/data?hl=zh-cn)为生成有效的对称密钥提供了默认实现，如以下代码段所示：

[Kotlin](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#kotlin)[Java](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#java)

```
  // Although you can define your own key generation parameter specification, it's  // recommended that you use the value specified here.  KeyGenParameterSpec keyGenParameterSpec = MasterKeys.AES256_GCM_SPEC;  String masterKeyAlias = MasterKeys.getOrCreate(keyGenParameterSpec);  
```

或者，您也可以稍后使用 `KeyStore.setKeyEntry` 将证书替换为由证书授权机构 (CA) 签名的证书。

如需生成密钥，请使用 `KeyPairGenerator` 和 `KeyPairGeneratorSpec`。

[Kotlin](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#kotlin)[Java](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#java)

```
  /*   * Generate a new EC key pair entry in the Android Keystore by   * using the KeyPairGenerator API. The private key can only be   * used for signing or verification and only with SHA-256 or   * SHA-512 as the message digest.   */  KeyPairGenerator kpg = KeyPairGenerator.getInstance(      KeyProperties.KEY_ALGORITHM_EC, "AndroidKeyStore");  kpg.initialize(new KeyGenParameterSpec.Builder(      alias,      KeyProperties.PURPOSE_SIGN | KeyProperties.PURPOSE_VERIFY)      .setDigests(KeyProperties.DIGEST_SHA256,        KeyProperties.DIGEST_SHA512)      .build());  KeyPair kp = kpg.generateKeyPair();  
```

### 生成新密钥

如需生成密钥，请按照与[生成新私钥](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#GeneratingANewPrivateKey)相同的流程进行操作。在每种情况下，您都需要使用 [Security 库](https://android-dot-google-developers.gonglchuangl.net/topic/security/data?hl=zh-cn)。

### 更安全地导入加密密钥

借助 Android 9（API 级别 28）及更高版本，您能够利用 ASN.1 编码密钥格式将已加密密钥安全导入密钥库。Keymaster 随后会在密钥库中将密钥解密，因此密钥的内容永远不会以明文形式出现在设备的主机内存中。此过程提高了密钥解密的安全性。

**注意**：只有搭载 Keymaster 4 或更高版本的设备才支持该功能。

如需支持以安全方式将已加密密钥导入密钥库，请完成以下步骤：

1. 生成目的为 [`PURPOSE_WRAP_KEY`](https://android-dot-google-developers.gonglchuangl.net/reference/android/security/keystore/KeyProperties?hl=zh-cn#PURPOSE_WRAP_KEY) 的密钥对。建议也为该密钥对添加认证。

2. 在您信任的服务器或机器上，生成 `SecureKeyWrapper` 应包含的 ASN.1 消息。

   该封装容器包含以下架构：

1. `KeyDescription ::= SEQUENCE {    keyFormat INTEGER,    authorizationList AuthorizationList  }  SecureKeyWrapper ::= SEQUENCE {    wrapperFormatVersion INTEGER,    encryptedTransportKey OCTET_STRING,    initializationVector OCTET_STRING,    keyDescription KeyDescription,    secureKey OCTET_STRING,    tag OCTET_STRING  }  `
2. 创建 [`WrappedKeyEntry`](https://android-dot-google-developers.gonglchuangl.net/reference/android/security/keystore/WrappedKeyEntry?hl=zh-cn) 对象，传入字节数组形式的 ASN.1 消息。
3. 将该 `WrappedKeyEntry` 对象传入接受 [`Keystore.Entry`](https://android-dot-google-developers.gonglchuangl.net/reference/java/security/KeyStore.Entry?hl=zh-cn) 对象的 [`setEntry()`](https://android-dot-google-developers.gonglchuangl.net/reference/java/security/KeyStore?hl=zh-cn#setEntry(java.lang.String, java.security.KeyStore.Entry, java.security.KeyStore.ProtectionParameter)) 的重载。

### 使用密钥库条目

`AndroidKeyStore` 提供程序的使用通过所有标准 `KeyStore` API 加以实现。

#### 列出条目

通过调用 `aliases()` 方法列出密钥库中的条目：

[Kotlin](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#kotlin)[Java](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#java)

```
  /*   * Load the Android KeyStore instance using the   * "AndroidKeyStore" provider to list out what entries are   * currently stored.   */  KeyStore ks = KeyStore.getInstance("AndroidKeyStore");  ks.load(null);  Enumeration<String> aliases = ks.aliases();  
```

#### 对数据进行签名和验证

通过从密钥库提取 `KeyStore.Entry` 并使用 `Signature` API（例如 `sign()`）来为数据签名：

[Kotlin](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#kotlin)[Java](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#java)

```
  /*   * Use a PrivateKey in the KeyStore to create a signature over   * some data.   */  KeyStore ks = KeyStore.getInstance("AndroidKeyStore");  ks.load(null);  KeyStore.Entry entry = ks.getEntry(alias, null);  if (!(entry instanceof PrivateKeyEntry)) {    Log.w(TAG, "Not an instance of a PrivateKeyEntry");    return null;  }  Signature s = Signature.getInstance("SHA256withECDSA");  s.initSign(((PrivateKeyEntry) entry).getPrivateKey());  s.update(data);  byte[] signature = s.sign();  
```

类似地，请使用 `verify(byte[])` 方法验证数据：

[Kotlin](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#kotlin)[Java](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#java)

```
  /*   * Verify a signature previously made by a PrivateKey in our   * KeyStore. This uses the X.509 certificate attached to our   * private key in the KeyStore to validate a previously   * generated signature.   */  KeyStore ks = KeyStore.getInstance("AndroidKeyStore");  ks.load(null);  KeyStore.Entry entry = ks.getEntry(alias, null);  if (!(entry instanceof PrivateKeyEntry)) {    Log.w(TAG, "Not an instance of a PrivateKeyEntry");    return false;  }  Signature s = Signature.getInstance("SHA256withECDSA");  s.initVerify(((PrivateKeyEntry) entry).getCertificate());  s.update(data);  boolean valid = s.verify(signature);  
```

### 要求进行用户身份验证才能使用密钥

生成密钥或将密钥导入到 `AndroidKeyStore` 时，您可以指定仅授权经过身份验证的用户使用密钥。用户使用安全锁定屏幕凭据（图案/PIN 码/密码、指纹）的子集进行身份验证。

这是一项高级安全功能，通常仅用于有以下要求的情形：在生成/导入密钥后（而不是之前或当中），应用进程受到攻击不会导致密钥被未经身份验证的用户使用。

如果仅授权经过身份验证的用户使用密钥，可将密钥配置为以下列两种模式之一运行：

- 经过身份验证的用户可以在一段时间内使用密钥。在用户解锁安全锁定屏幕或使用 `KeyguardManager.createConfirmDeviceCredentialIntent` 流确认其安全锁定屏幕凭据后，即授权其使用此模式中的所有密钥。每个密钥的授权持续时间各不相同，并由 `setUserAuthenticationValidityDurationSeconds` 在密钥生成或导入时指定。此类密钥只有在启用了安全锁定屏幕的情况下才能生成或导入（请参阅 `KeyguardManager.isDeviceSecure()`）。在安全锁定屏幕停用（重新配置为“无”、“滑动”或不验证用户身份的其他模式）或被强制重置（例如由设备管理员执行）时，这些密钥将永久失效。
- 用户身份验证会授权与某一密钥关联的特定加密操作。在此模式中，涉及此类密钥的每个操作都必须由用户单独授权。目前，此类授权的唯一方式是指纹身份验证：`FingerprintManager.authenticate`。此类密钥只有在至少注册一个指纹的情况下才能生成或导入（请参阅 `FingerprintManager.hasEnrolledFingerprints`）。一旦注册新指纹或取消注册所有指纹，这些密钥将永久失效。

## 支持的算法

- [`Cipher`](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedCiphers)
- [`KeyGenerator`](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedKeyGenerators)
- [`KeyFactory`](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedKeyFactories)
- [`KeyPairGenerator`](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedKeyPairGenerators)
- [`Mac`](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedMacs)
- [`Signature`](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedSignatures)
- [`SecretKeyFactory`](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedSecretKeyFactories)

### Cipher

| 算法                                  | 提供支持的 API 级别 | 备注                    |
| ------------------------------------- | ------------------- | ----------------------- |
| AES/CBC/NoPadding                     | 23+                 |                         |
| AES/CBC/PKCS7Padding                  | 23+                 |                         |
| AES/CTR/NoPadding                     | 23+                 |                         |
| AES/ECB/NoPadding                     | 23+                 |                         |
| AES/ECB/PKCS7Padding                  | 23+                 |                         |
| AES/GCM/NoPadding                     | 23+                 | 仅支持 12 字节长的 IV。 |
| RSA/ECB/NoPadding                     | 18+                 |                         |
| RSA/ECB/PKCS1Padding                  | 18+                 |                         |
| RSA/ECB/OAEPWithSHA-1AndMGF1Padding   | 23+                 |                         |
| RSA/ECB/OAEPWithSHA-224AndMGF1Padding | 23+                 |                         |
| RSA/ECB/OAEPWithSHA-256AndMGF1Padding | 23+                 |                         |
| RSA/ECB/OAEPWithSHA-384AndMGF1Padding | 23+                 |                         |
| RSA/ECB/OAEPWithSHA-512AndMGF1Padding | 23+                 |                         |
| RSA/ECB/OAEPPadding                   | 23+                 |                         |

### KeyGenerator

| 算法       | 提供支持的 API 级别 | 备注                                                         |
| ---------- | ------------------- | ------------------------------------------------------------ |
| AES        | 23+                 | 支持的大小：128、192、256                                    |
| HmacSHA1   | 23+                 | 支持的大小：8-1024（含），必须是 8 的倍数              默认大小：160 |
| HmacSHA224 | 23+                 | 支持的大小：8-1024（含），必须是 8 的倍数              默认大小：224 |
| HmacSHA256 | 23+                 | 支持的大小：8-1024（含），必须是 8 的倍数              默认大小：256 |
| HmacSHA384 | 23+                 | 支持的大小：8-1024（含），必须是 8 的倍数              默认大小：384 |
| HmacSHA512 | 23+                 | 支持的大小：8-1024（含），必须是 8 的倍数              默认大小：512 |

### KeyFactory

| 算法 | 提供支持的 API 级别 | 备注                                                         |
| ---- | ------------------- | ------------------------------------------------------------ |
| EC   | 23+                 | 支持的密钥规范：`KeyInfo`（仅私钥）、`ECPublicKeySpec`（仅公钥）、`X509EncodedKeySpec`（仅公钥） |
| RSA  | 23+                 | 支持的密钥规范：`KeyInfo`（仅私钥）、`RSAPublicKeySpec`（仅公钥）、`X509EncodedKeySpec`（仅公钥） |

### KeyStore

KeyStore 支持的密钥类型与 [`KeyPairGenerator`](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedKeyPairGenerators) 和 [`KeyGenerator`](https://android-dot-google-developers.gonglchuangl.net/training/articles/keystore?hl=zh-cn#SupportedKeyGenerators) 支持的相同。

### KeyPairGenerator

| 算法 | 提供支持的 API 级别 | 备注                                                         |
| ---- | ------------------- | ------------------------------------------------------------ |
| DSA  | 19-22               |                                                              |
| EC   | 23+                 | 支持的大小：224、256、384、521              支持的命名曲线：P-224 (secp224r1)、P-256（又称为 secp256r1 和 prime256v1）、P-384（又称为 secp384r1）、P-521（又称为 secp521r1）                         在 API 级别 23 前，EC 密钥可使用经“RSA”算法初始化的 `KeyPairGeneratorSpec` 的 KeyPairGenerator 生成，其密钥类型需使用 `setKeyType(String)` 设为“EC”。无法使用此方法指定 EC 曲线名称。系统会根据请求的密钥大小自动选择 NIST P 曲线。 |
| RSA  | 18+                 | 支持的大小：512、768、1024、2048、3072、4096              支持的公开指数：3、65537              默认公开指数：65537 |

### Mac

| 算法       | 提供支持的 API 级别 | 备注 |
| ---------- | ------------------- | ---- |
| HmacSHA1   | 23+                 |      |
| HmacSHA224 | 23+                 |      |
| HmacSHA256 | 23+                 |      |
| HmacSHA384 | 23+                 |      |
| HmacSHA512 | 23+                 |      |

### Signature

| 算法              | 提供支持的 API 级别 | 备注 |
| ----------------- | ------------------- | ---- |
| MD5withRSA        | 18+                 |      |
| NONEwithECDSA     | 23+                 |      |
| NONEwithRSA       | 18+                 |      |
| SHA1withDSA       | 19-22               |      |
| SHA1withECDSA     | 19+                 |      |
| SHA1withRSA       | 18+                 |      |
| SHA1withRSA/PSS   | 23+                 |      |
| SHA224withDSA     | 20-22               |      |
| SHA224withECDSA   | 20+                 |      |
| SHA224withRSA     | 20+                 |      |
| SHA224withRSA/PSS | 23+                 |      |
| SHA256withDSA     | 19-22               |      |
| SHA256withECDSA   | 19+                 |      |
| SHA256withRSA     | 18+                 |      |
| SHA256withRSA/PSS | 23+                 |      |
| SHA384withDSA     | 19-22               |      |
| SHA384withECDSA   | 19+                 |      |
| SHA384withRSA     | 18+                 |      |
| SHA384withRSA/PSS | 23+                 |      |
| SHA512withDSA     | 19-22               |      |
| SHA512withECDSA   | 19+                 |      |
| SHA512withRSA     | 18+                 |      |
| SHA512withRSA/PSS | 23+                 |      |

### SecretKeyFactory

| 算法       | 提供支持的 API 级别 | 备注                      |
| ---------- | ------------------- | ------------------------- |
| AES        | 23+                 | 支持的密钥规范：`KeyInfo` |
| HmacSHA1   | 23+                 | 支持的密钥规范：`KeyInfo` |
| HmacSHA224 | 23+                 | 支持的密钥规范：`KeyInfo` |
| HmacSHA256 | 23+                 | 支持的密钥规范：`KeyInfo` |
| HmacSHA384 | 23+                 | 支持的密钥规范：`KeyInfo` |
| HmacSHA512 | 23+                 | 支持的密钥规范：`KeyInfo` |

#### 总结

不指定路径load