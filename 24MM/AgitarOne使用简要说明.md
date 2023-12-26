# 介绍

AgitarOne,单体测试工具，帮助测试人员自动生成测试代码，减少工作量。

# 环境

开发网络，Ubuntu操作系统。

只在特定网址下载特定的eclipse配置，不要在其他地方自主下载。

JRE11

服务端信息，地址、端口等都要设置好

相关资料地址:

* [最新手顺](http://wikijs.ci.iauto.com/zh/home/ci/codequality/unittest/agitarone)
* [往期手册](\\uranus.storm\iAuto-share\24MM\97_培训教育\Agitar One工具)
* [早期依赖包](\\uranus.storm\iAuto-share\24MM\97_培训教育\Agitar One工具\工具说明资料)

# 使用步骤

其实基本完全参照**最新手顺**中的介绍(以sysctrlpowermanager为例)按步执行即可，如不清楚，可进一步参照往期手册1.2中的图示。

只是有以下实际操作过程中的要点**需要注意**:

1. 项目名称依照需求必须是agitarOneConfig
2. 依照实际操作，该agitarOneConfig目录应该创建在package/apps/<package-name>目录下，例如otacomposer服务分为packages/apps/otacomposer和packages/service/otacomposer两个目录，但我在service/otacomposer目录下创建agitarOneConfig项目实测生成dashboard时会报错，导致无法生成。而在packages/apps/otacomposer目录下实测是可行的。
3. 应当依据需要将所有的依赖包添加进来，例如otacomposer需要iauto-composer-api.jar,那么就需要生成这个jar包之后拷贝到agitarOneConfig/libs目录下，然后依照手顺添加配置
4. 实测android-default-values.jar和framework-minus-apex.jar高度重复，只需要后者即可(前者是早期包)
5. agitarOneConfig导入或者关闭后再次打开时记得激活，激活方式在手顺中也有。
6. 实测service/otacomposer/目录下(不包含子目录)的代码文件无法被agitarOne正确识别，需要再创建一个子package,把文件放进去，例如原先在service/otacomposer目录下有一个application文件OTAComposer.java,但是就是会弹出error,进而影响测试代码生成，因此可以再创建一个package,把OTAComposer文件扔进去，记得修改Manifest.xml文件中application的名称。
7. 单体测试报告和集成测试报告的最新模板在[模板](https://svn.ci.iauto.com/svn/smartauto/Rhine/01_开发库/01_项目管理/05_质量保证/02_流程模板/工作产品&议事录&检查单模板),Update目录下的现有文件应该是过时的
8. dashboards界面上可以看到是有target要求的，最终我们应当修改测试代码或源代码，使得该要求能够被通过。
9. agitarOneConfig最终需要提交的文件也有要求，这在手顺中也有说明，相关配置可以参照OTAComposer下的agitarOneConfig目录下的gitignore文件。

# 参考

OTAComposer相关提交地址，以供参考:

[OtaComposer-Service目录下代码](http://gerrit.iauto.com/#/c/Src/iAutoDroid/packages/service/otacomposer/+/210521/)
[OtaComposer-app目录下实际测试代码](http://gerrit.iauto.com/#/c/Src/iAutoDroid/packages/apps/otacomposer/+/210494/)
[OtaComposer-dashboard当前结果](http://agitarone.ci.iauto.com:8080/agitar-server/dashboards/developer/F21492E336D7AE0B9C5AB5552807F85F/20230920-114807/index.html)


# 其他

手顺最后有一个提升覆盖率的方法，但是本人没有实验过，因此列出，但无法说明。

