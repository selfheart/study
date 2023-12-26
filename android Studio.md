<center>Android studio </center>

## 1.快捷键





## 2.error

2.1 Gradle 相关问题

gradel相关问题可在这里修改

 ![image-20230711121607603](C:\Users\jiangzhuangzhuang\study\study\android Studio.assets\image-20230711121607603.png)

 ![image-20230711112300530](C:\Users\jiangzhuangzhuang\study\study\android Studio.assets\image-20230711112300530.png)

## 3.配置签名

在您的项目的 `build.gradle` 文件中，您需要进行以下配置来使用签名密钥：

1. 在 `android` 块中添加 `signingConfigs` 部分，定义您的签名配置。例如：

```groovy
android {
    // ...

    signingConfigs {
        iauto24 {
            storeFile file('keystore/24mm/platform.keystore')
            storePassword 'android'
            keyAlias 'platform'
            keyPassword 'android'
        }
    }

    // ...
}
```

请注意，您需要将 `storeFile` 的值设置为正确的密钥存储文件的路径，并相应地填写 `storePassword`、`keyAlias` 和 `keyPassword`。

2. 在 `buildTypes` 中选择要使用的构建类型，并将其关联到您的签名配置。例如，将签名配置 `iauto24` 关联到 `release` 构建类型：

```groovy
android {
    // ...

    buildTypes {
        release {
            // ...
            signingConfig signingConfigs.iauto24
        }
    }

    // ...
}
```

通过以上配置，当您构建 `release` 版本时，Gradle 将使用指定的签名配置来对 APK 进行签名。

确保在进行实际构建之前，将正确的密钥存储文件和密码放置在指定的位置，并确认密钥别名和密码与您的签名配置一致。

3. Androidmanifest中加上android:sharedUserId="android.uid.system"