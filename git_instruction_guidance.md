# GitHub 多人协作流程

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

   - 为了避免直接在主分支（main 或 master）上操作，建议每个合作者在进行修改前创建自己的分支：

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


-----

## SSH连接

### 一、生成 SSH 密钥

在本地机器上生成 SSH 密钥是 SSH 连接的第一步。以下是步骤：

1. **打开终端（Terminal）**

   - Windows 用户可以使用 Git Bash 或者 WSL（Windows Subsystem for Linux）。
   - macOS 和 Linux 用户可以直接使用系统自带的终端工具。

2. **生成 SSH 密钥**
   在终端输入以下命令，开始生成 SSH 密钥：

   ```bash
   ssh-keygen -t rsa -b 4096 -C "your_email@example.com"
   ```

   - `-t rsa` 表示使用 RSA 加密算法。
   - `-b 4096` 表示生成 4096 位的密钥，确保较高的安全性。
   - `-C "your_email@example.com"` 是你在 GitHub 上注册的邮箱地址，用作标识。

3. **设置密钥文件保存位置**

   - 系统会提示你选择保存密钥的路径，默认路径是：`~/.ssh/id_rsa`（建议使用默认路径，直接按回车）。
   - 如果该路径下已有密钥文件，可以选择覆盖或指定一个新的文件名。

4. **设置密码短语（可选）**

   - 接下来会提示你输入密码短语。这是一个**可选步骤**，给你的 SSH 密钥增加一层安全性。如果不想设置密码短语，**直接按回车跳过。**

5. **查看生成的 SSH 公钥**
   生成密钥后，你需要获取公钥内容，将其添加到 GitHub。可以使用以下命令查看：

   ```bash
   cat ~/.ssh/id_rsa.pub
   ```

   这将输出你生成的公钥内容，形如：

   ```
   ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAACAQD3VexamplepublickeyABC== your_email@example.com
   ```

### 二、将 SSH 公钥添加到 GitHub

1. **复制 SSH 公钥**
   使用上一步的命令获取公钥后，复制输出的所有内容（从 `ssh-rsa` 到邮箱地址）。

2. **登录 GitHub**
   - 打开 [GitHub](https://github.com) 并登录。

3. **添加 SSH 公钥**
   - 点击页面右上角的头像，选择 **Settings（设置）**。
   - 在左侧菜单中，选择 **SSH and GPG keys**。
   - 点击 **New SSH key** 按钮。
   - 在 **Title** 中为这把公钥取个名称（如“个人笔记本”），在 **Key** 中粘贴刚才复制的公钥内容。
   - 点击 **Add SSH key** 完成操作。

### 三、测试 SSH 连接

在添加 SSH 公钥到 GitHub 之后，可以通过以下命令测试是否成功连接：

```bash
ssh -T git@github.com
```

如果 SSH 连接成功，你会看到类似以下的输出：

```bash
Hi username! You've successfully authenticated, but GitHub does not provide shell access.
```

这说明你的 SSH 连接已经配置成功，并且可以用来与 GitHub 进行交互。

### 四、使用 SSH 克隆仓库

在成功配置 SSH 之后，可以使用 SSH URL 来克隆 GitHub 仓库。克隆命令的格式如下：

```bash
git clone git@github.com:用户名/仓库名.git
```

例如，如果你要克隆 `FPGA_r_op` 仓库，命令将是：

```bash
git clone git@github.com:worldline22/FPGA_r_op.git
```

使用 SSH 克隆仓库后，所有 Git 操作（如 `pull`、`push` 等）都将使用 SSH 进行身份验证，无需每次输入用户名和密码。

### 五、常见问题和解决方法

1. **SSH 密钥找不到或无法使用**

   - 如果遇到找不到 SSH 密钥的问题，确保密钥存放在 `~/.ssh/` 目录下，并且文件名是 `id_rsa` 和 `id_rsa.pub`。

   - 也可以通过以下命令手动添加 SSH 密钥到 SSH 代理：

     ```bash
     eval "$(ssh-agent -s)"
     ssh-add ~/.ssh/id_rsa
     ```

2. **权限问题**

   - 确保 SSH 密钥文件的权限设置正确，特别是私钥 `id_rsa` 的权限应当是 600（仅用户可读写），可以用以下命令修改权限：

     ```bash
     chmod 600 ~/.ssh/id_rsa
     ```

3. **使用多个 SSH 密钥**

   - 如果你有多个 GitHub 账户或需要在不同项目中使用不同的 SSH 密钥，可以通过配置 `~/.ssh/config` 文件来管理多个 SSH 密钥。例如：

     ```
     Host github.com
       HostName github.com
       User git
       IdentityFile ~/.ssh/id_rsa
       
     Host github-work
       HostName github.com
       User git
       IdentityFile ~/.ssh/id_rsa_work
     ```

### 总结

1. **生成 SSH 密钥**：通过 `ssh-keygen` 生成 SSH 密钥对。
2. **添加公钥到 GitHub**：将生成的公钥添加到 GitHub 账户的 **SSH and GPG keys** 设置中。
3. **测试连接**：通过 `ssh -T git@github.com` 测试是否成功连接。
4. **克隆仓库**：使用 SSH URL 克隆仓库，避免使用用户名和密码进行身份验证。

通过 SSH，你可以更安全、高效地管理 GitHub 仓库，并且不再需要每次输入密码。



:warning:记住在GitHub上要用ssh方法连接如：

```bash
git clone git@github.com:worldline22/FPGA_r_op.git
```

然后在local操作



## 基本的git协作指令

- 打开要下载操作仓库的文件夹（例如Desktop）
- 下载仓库到本地：

```bash
git clone git@github.com:worldline22/FPGA_r_op.git
```

- 建立自己的branch，如果已经建立的branch，切换branch直接去掉`-b`即可

```bash
git checkout -b <your branch name>
```

- 显示所有本地branch

```bash
git branch
```

- 配置标识

```bash
git config user.name "xxx"  # 配置用户名
git config user.email "xxx@xxx.com"  # 配置邮箱
```

- 在本地编写代码
- 暂存到本地缓存区

```bash
git add <文件名/文件夹名>

example:
git add 赛题要求.pdf
git add Appendix
```

- 提交

```bash
git commit -m "随便写，就是给我们看的一个信息或声明"
```

- 正式提交

```bash
git push -u origin <branch> # 将本地仓库的分支提交至相应的远程仓库分支
```







