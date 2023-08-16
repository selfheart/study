<div align='center'>Linux</div>

# 1.文件操作





1. 文件夹不存在创建文件夹

    if [ ! -d "/data/" ];then
        mkdir /data
    else
        echo "文件夹已经存在"
    fi

2. 判断文件夹是否存在

    if [ -d "/data/" ];then
        echo "文件夹存在"
    else
        echo "文件夹不存在"
    fi

3. 判断文件是否存在

    if [ -f "/data/filename" ];then
        echo "文件存在"
    else
        echo "文件不存在"
    fi

4. 常用的文件比较符

    -e 判断对象是否存在
    -d 判断对象是否存在，并且为目录
    -f 判断对象是否存在，并且为常规文件
    -L 判断对象是否存在，并且为符号链接
    -h 判断对象是否存在，并且为软链接
    -s 判断对象是否存在，并且长度不为0
    -r 判断对象是否存在，并且可读
    -w 判断对象是否存在，并且可写
    -x 判断对象是否存在，并且可执行
    -O 判断对象是否存在，并且属于当前用户
    -G 判断对象是否存在，并且属于当前用户组
    -nt 判断file1是否比file2新  [ "/data/file1" -nt "/data/file2" ]
    -ot 判断file1是否比file2旧  [ "/data/file1" -ot "/data/file2" ]

# 2、权限

当涉及到 Linux 文件权限时，以下是一些常用的命令和相关概念的简要介绍：

1. `ls -l`：显示文件和目录的详细列表，包括权限、所有者、所属组、文件大小等信息。

2. `chmod`：用于更改文件或目录的权限。常用的选项包括：
   - `chmod +x file`：为文件添加可执行权限。
   - `chmod -x file`：去除文件的可执行权限。
   - `chmod u+r file`：允许文件所有者读取权限。
   - `chmod g+w file`：允许文件所属组写入权限。
   - `chmod o-rwx file`：禁止其他用户的读取、写入和执行权限。

3. `chown`：用于更改文件或目录的所有者和所属组。常用的选项包括：
   - `chown user:group file`：将文件的所有者更改为指定用户，同时将所属组设置为指定组。
   - `chown user file`：只更改文件的所有者，保持所属组不变。
   - `chown :group file`：将文件的所属组更改为指定组，保持所有者不变。

4. `chgrp`：用于更改文件或目录的所属组。
   - `chgrp group file`：将文件的所属组更改为指定组。

5. `umask`：设置新创建文件和目录的默认权限掩码。
   - `umask 022`：设置默认权限掩码为 022，表示新创建的文件权限为 644，新创建的目录权限为 755。

6. `sudo`：以超级用户（root）权限执行命令。

这些命令可以帮助您管理和调整文件的权限，确保合适的权限设置以满足安全需求和访问控制。请记住，在进行任何更改权限的操作之前，务必谨慎并核对输入的命令。







## docker

Docker 是一种容器化平台，可以将应用程序和其依赖项打包成一个可移植的容器。每个容器都是一个独立运行的单元，具有自己的文件系统、运行时环境、网络设置等。

以下是 Docker 的一些核心概念和特点：

1. **镜像（Image）**：镜像是容器的模板，可以看作是一个只读的文件系统快照。它包含了运行应用所需的操作系统、环境变量、库文件、依赖项等。镜像是构建容器的基础，可以通过 Dockerfile 或者从 Docker Hub 等镜像仓库中获取。

2. **容器（Container）**：容器是基于镜像创建的运行实例。每个容器都是相互隔离的，具有自己的文件系统、进程空间、网络接口等。容器可以快速启动、停止、删除，并且支持水平扩展。

3. **Dockerfile**：Dockerfile 是一个文本文件，用于定义构建 Docker 镜像所需的环境和步骤。通过在 Dockerfile 中编写一系列指令，可以自动化地构建镜像。例如，指定基础镜像、安装软件、复制文件、设置环境变量等。

4. **容器编排**：Docker 提供了容器编排工具，例如 Docker Compose、Kubernetes 等，用于管理和编排多个容器的部署。通过编排工具，可以定义容器之间的关系、资源限制、扩缩容规则等，从而构建复杂的应用架构。

5. **跨平台**：Docker 可以在不同的操作系统和云平台上运行，包括 Linux、Windows、macOS，以及各种公有云和私有云环境。这使得开发、测试和部署变得更加灵活和可移植。

6. **镜像仓库**：镜像仓库是存储和分享镜像的地方。Docker Hub 是官方的公共镜像仓库，提供了大量的官方和社区维护的镜像。同时，您也可以搭建自己的私有镜像仓库，以便团队内部使用或者在受限网络环境下部署应用。

通过使用 Docker，您可以实现以下好处：

- **快速部署和扩展**：由于容器是轻量级的，启动速度快，并且可以根据需要快速扩展，能够更高效地响应流量变化。
- **环境一致性**：容器可以在不同的环境中运行，包括开发、测试、生产环境，保证应用在不同环境中行为一致。
- **资源利用率**：容器共享操作系统内核，可以更高效地利用硬件资源，提高服务器的利用率。
- **隔离性和安全**：容器之间相互隔离，每个容器都有自己的运行环境，避免应用之间的干扰和冲突，提供了一定程度的安全性。

总的来说，Docker 是一种强大的容器化平台，能够帮助开发人员和运维团队更好地管理和部署应用程序，提高开发效率和应用的可靠性。

sudo docker run -it --tmpfs /tmp:exec --entrypoint /bin/bash --rm -w $HOME -v $HOME:$HOME -e HOME=$HOME -v /etc/passwd:/etc/passwd -v /etc/group:/etc/group --device /dev/kvm -e DISPLAY=${DISPLAY} -e USER=$(whoami) -v /tmp/.X11-unix:/tmp/.X11-unix -u $(id -u ${USER}):$(id -g ${USER}) --dns=192.168.2.14 --device /dev/fuse --ulimit nofile=40960 --privileged iregistry.iauto.com/ci_members/iautoandroid-container/common/fullenv:feature-android12

这是一个使用 `docker run` 命令来启动一个容器的示例命令。该命令包含了很多参数和选项，用于配置容器的运行环境和挂载卷。

以下是对命令中各个参数的解释：

- `-it`: 分配一个伪终端并保持 STDIN 打开，以便可以与容器进行交互。
- `--tmpfs /tmp:exec`: 将临时文件系统 (`tmpfs`) 挂载到容器的 `/tmp` 目录，并设置其权限以允许执行文件。
- `--entrypoint /bin/bash`: 使用 `/bin/bash` 作为容器的入口点（启动命令），即登录到容器后直接进入 Bash Shell。
- `--rm`: 容器退出后自动删除容器及其文件系统。
- `-w $HOME`: 设置容器的工作目录为 `$HOME`，即宿主主机的用户主目录。
- `-v $HOME:$HOME`: 将宿主主机的用户主目录挂载到容器中的相同路径，实现宿主主机与容器之间的文件共享。
- `-e HOME=$HOME`: 设置环境变量 `$HOME` 为当前用户的主目录。
- `-v /etc/passwd:/etc/passwd -v /etc/group:/etc/group`: 将宿主主机的 `/etc/passwd` 和 `/etc/group` 文件挂载到容器中，用于用户和用户组的映射。
- `--device /dev/kvm`: 将宿主主机的 `/dev/kvm` 设备挂载到容器中，以便容器内的进程可以使用 KVM 虚拟化。
- `-e DISPLAY=${DISPLAY} -e USER=$(whoami) -v /tmp/.X11-unix:/tmp/.X11-unix`: 配置容器以显示图形界面程序，将 X11 套接字目录 `/tmp/.X11-unix` 挂载到容器中，并设置环境变量 `DISPLAY` 和 `USER`。
- `-u $(id -u ${USER}):$(id -g ${USER})`: 以当前用户的 UID 和 GID 运行容器，确保容器内外的文件访问权限一致。
- `--dns=192.168.2.14`: 设置容器使用的 DNS 服务器地址为 `192.168.2.14`。
- `--device /dev/fuse`: 将宿主主机的 `/dev/fuse` 设备挂载到容器中，以支持 FUSE 文件系统。
- `--ulimit nofile=40960`: 设置容器的文件描述符限制为 `40960`。
- `--privileged`: 赋予容器完全的特权级别，以便可以执行更高级别的操作。

最后，`iregistry.iauto.com/ci_members/iautoandroid-container/common/fullenv:feature-android12` 是镜像的名称和标签，用于指定要运行的容器镜像。

请根据您的实际需求对命令进行适当的修改和调整。