## 建立仓库

### 一、创建 GitHub 仓库

1. **登录 GitHub**

   - 使用你的 GitHub 账号登录 [GitHub 官网](https://github.com)。

2. **新建仓库**

   - 在页面右上角点击个人头像旁边的 “+” 按钮，选择 “New repository”（新建仓库）。
   - 填写仓库名称和描述。
   - 选择仓库的公开性：
     - Public（公开）：所有人都能看到这个仓库。
     - Private（私有）：只有你授权的用户可以访问这个仓库。
   - 如果需要，可以选择添加 README 文件或 `.gitignore` 模板。
   - 点击 “Create repository” 创建仓库。

3. **初始化仓库**

   - 若没有选择初始化 README 文件，GitHub 会提供命令让你通过终端或 Git Bash 来初始化本地仓库并推送代码：

     ```bash
     git init
     git add README.md
     git commit -m "first commit"
     git branch -M main
     git remote add origin https://github.com/你的用户名/仓库名.git
     git push -u origin main
     ```

### 二、添加合作者

1. **进入仓库的 “Settings”**
   - 打开仓库后，点击页面右上角的 “Settings” 标签。

2. **设置合作者权限**
   - 在左侧菜单栏找到 “Collaborators and teams” 选项，点击。
   - 在 "Collaborators" 部分，输入你想邀请合作者的 GitHub 用户名或邮箱地址。
   - 点击 “Add collaborator” 按钮，GitHub 会给被邀请者发送合作邀请。

3. **合作者接受邀请**
   - 被邀请的合作者会收到邮件通知，他们需要点击邮件中的链接接受邀请才能加入合作。

### 三、多人协作代码管理流程

1. **克隆仓库**

   - 合作者需要克隆仓库到本地：

     ```bash
     git clone https://github.com/你的用户名/仓库名.git
     ```

2. **创建分支**

   - 为了避免直接在主分支（main 或 master）上操作，建议每个合作者在进行修改前创建自己的分支（如果已经建立了分支，则去掉`-b`：

     ```bash
     git checkout -b 新分支名
     ```

3. **提交更改**

   - 在完成代码修改后，将更改提交到本地仓库：

     ```bash
     git add .
     git commit -m "提交信息"
     ```

4. **推送分支**

   - 将本地分支推送到远程仓库：

     ```bash
     git push origin 分支名
     ```

5. **发起 Pull Request (PR)**

   - 在 GitHub 上，提交分支后可以发起一个 Pull Request，通知项目负责人或者其他合作者进行代码审查。
   - 在仓库的 “Pull requests” 标签中，点击 “New pull request”，选择目标分支和你的分支，填写说明后点击 “Create pull request”。

6. **代码审查和合并**

   - 仓库管理员或指定的审查者会审查代码，并在代码通过后合并到主分支：
     - 可以在 Pull Request 页面进行评论、提出修改建议，甚至拒绝或接受合并请求。
     - 如果同意合并，点击 “Merge pull request”。

7. **同步更新**

   - 合作者在本地开发时，需要确保主分支的代码保持最新：

     ```bash
     git checkout main
     git pull origin main
     ```

### 四、冲突解决

- 如果多个合作者同时修改了相同的文件或代码，可能会产生冲突。在合并前，需要解决这些冲突：
  1. Git 会提示冲突文件。
  2. 手动编辑文件，决定保留哪些更改。
  3. 解决冲突后，将修改提交并推送。

```bash
git add 冲突文件
git commit -m "解决冲突"
git push origin 分支名
```

这样，仓库的管理员可以再次合并 PR。

### 总结

1. 创建并初始化 GitHub 仓库。
2. 添加合作者并给予他们访问权限。
3. 合作者通过克隆、创建分支、提交代码、推送分支、发起 Pull Request 等流程进行协作开发。
4. 通过代码审查与合并来管理代码质量和进展。

这种操作流程能够确保多人协作时的代码版本控制有序且清晰。

