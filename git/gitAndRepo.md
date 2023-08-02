<div align='center'>git And repo</div>

## 1.git 

### 1.1 git checkout 

1.1.1 git 将某个目录下的所有文件回退到上个版本

如果您想要将Git仓库中某个目录下的所有文件回退到上一个版本，可以按照以下步骤进行操作：

1. 确保您在Git仓库所在的目录中打开终端或命令提示符。

2. 首先，使用`git log`命令查看提交历史，并找到您想要回退的上一个版本对应的提交哈希值（commit hash）。

3. 使用`cd`命令进入要回退的目录。例如，如果要回退的目录是`src/`，则可以执行`cd src/`。

4. 运行以下命令来撤销该目录下的所有更改并回退到上一个版本：

   ```shell
   git checkout HEAD~ -- .
   ```

   这将使该目录下的所有文件回滚到上一个版本。

5. 运行`git status`命令来确认回退是否成功，工作目录中的文件应显示为已修改但未暂存的状态。

6. 最后，使用`git add .`将回退的更改添加到暂存区，然后使用`git commit`提交这些更改。

请注意，此方法会更改Git仓库中的历史记录。如果您已经共享了这些提交或其他人正在基于这些提交进行工作，使用此方法可能会导致问题。在执行操作之前，请确保与团队合作并遵循相应的Git工作流程。



### 1.2 git 不提交本地代码，拉代码

如果你在本地有未提交的代码修改，但想要拉取最新的代码而不提交你的本地修改，可以使用以下命令：

1. 先使用 `git stash` 命令将本地修改保存到一个临时栈中：
```bash
git stash
```
这会将你的本地修改保存为一个新的存储项，使你的工作目录变回干净状态。

2. 然后执行 `git pull` 命令来拉取最新的代码：
```bash
git pull
```
这会从远程仓库拉取最新的代码更新到你的本地分支。

3. 如果你之前使用了 `git stash` 命令暂存了本地修改，现在可以使用 `git stash pop` 命令将之前的修改应用回来：
```bash
git stash pop
```
这会将之前的修改从临时栈中取出并应用到你的工作目录中。

请注意，这种操作可能会导致冲突，因为你将尝试将未提交的修改与从远程仓库拉取的最新代码合并。在处理冲突时，请按照 Git 提示进行解决。

记得在执行任何操作前，先备份重要的代码，以免意外数据丢失。

### 1.3 git branch

在 Git 中，分支 (branch) 是用于在版本控制中管理代码不同版本的重要概念。下面是一些常见的 Git 分支相关操作介绍：

1. 创建分支：使用 `git branch` 命令可以创建一个新的分支。例如，要创建名为 `feature` 的新分支，可以运行以下命令：
   ```
   git branch feature
   ```

2. 切换分支：使用 `git checkout` 命令可以切换到已存在的分支。例如，要切换到 `feature` 分支，可以运行以下命令：
   ```
   git checkout feature
   ```

   <font color='red'>注意：当前分支内容没有commit的话，切换分支会删除当前分支未保存内容</font>
   
   或者，您可以使用 `git switch` 命令进行切换（Git 2.23+ 版本）。例如：
   
```
   git switch feature
   ```
   
3. 创建并切换分支：使用 `git checkout -b` 命令可以同时创建并切换到一个新分支。例如，要创建并切换到名为 `feature` 的新分支，可以运行以下命令：
   ```
   git checkout -b feature
   ```

   或者，使用 `git switch -c` 命令来实现相同的效果（Git 2.23+ 版本）。例如：
   ```
   git switch -c feature
   ```

4. 查看分支：使用 `git branch` 命令可以查看仓库中所有的本地分支。运行 `git branch -r` 可以查看远程分支。添加 `-a` 参数可以同时查看本地和远程分支。

5. 删除分支：使用 `git branch -d` 命令可以删除已合并的本地分支。例如，要删除名为 `feature` 的分支，可以运行以下命令：
   ```
   git branch -d feature
   ```

   如果要强制删除一个尚未合并的分支，可以使用 `git branch -D` 命令：
   ```
   git branch -D feature
   ```

6. 追踪远程分支：使用 `git branch --set-upstream-to` 或 `git branch --track` 命令可以将本地分支与远程分支关联起来，并跟踪远程分支的更改。例如，要将本地分支 `feature` 关联到远程分支 `origin/feature`，可以运行以下命令：
   ```
   git branch --set-upstream-to=origin/feature feature
   ```

7. 合并分支：使用 `git merge` 命令可以将一个分支合并到当前所在的分支中。例如，如果希望将 `feature` 分支合并到当前分支，可以先切换到目标分支（例如 `main` 或 `master`），然后运行以下命令：
   ```
   git merge feature
   ```

8. 重命名分支：使用 `git branch -m` 命令可以对分支进行重命名。例如，要将当前分支重命名为 `new-feature`，可以运行以下命令：
   ```
   git branch -m new-feature
   ```

上述只是 Git 中的一些常见分支相关操作介绍，Git 还有更多用于管理和操作分支的命令和技巧。使用这些分支操作可以帮助您在开发过程中更好地组织和管理不同版本的代码。



## 2.repo