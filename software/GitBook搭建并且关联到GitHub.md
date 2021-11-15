# 概览
--
记录一次使用gitbook写电子书,并且将源码推送到github仓库。最后通过github action自动构建电子书并且发布的过程。

最终效果: 本地编辑文章,生成md文件。push到github仓库，就可以在线通过自定义域名浏览文章。如: [我的笔记本](https://note.tyduyong.com/)

# 自动发布书籍的方式

> 我使用的是第二种方式,这里介绍一下两种方式

## 1. gitbook在线构建

> gitbook 允许你使用md文档的语法，构建出精美的电子书。

gitbook是一个文档或者是电子书托管平台，官方站点就是 [gitbook](gitbook.com)。是一个线上环境。

gitbook也是一个基于node.js的命令行工具。gitbook工具允许我们在本地快速构建书籍结构，下载插件，构建电子书并且支持启动本地web服务来浏览本地的电子书。
这里说一下使用gitbook线上环境自动发布书籍的逻辑:

首先gitbook线上环境允许你在线创建自己的文档,使用md的方式对文档进行编辑。

允许对外发布,分两种方式 1.允许搜索引擎爬取 2.不允许搜索引擎爬取。
对外发布的Space或者是Collection可以绑定自定义的域名,让用户访问。

 在线的gitbook仓库也可以和github仓库关联,互相同步。

所以,如果使用gitbook线上环境自动发布书籍,可以这样做:  

1. 使用gitbook-cli创建本地书籍
2. 登录gitbook账号,创建Space或者是Collection
3. 创建你的github仓库,用来存储数据源文件。
4. gitbook仓库与github仓库绑定,授权同步。

这样我们就可以本地写文章,然后推送到github,绑定的仓库有更新时,gitbook会自动从github同步你的源文件。gitbook属于线上环境,会自动构建,安装插件。然后就可以访问了。

这是**一种**自动发布的方式,但自己的部署的时候碰到一些问题：

1. 网站有时候不能访问，需要fq。
2. 本地构建书籍时,插件可用。推送到线上后,gitbook构建出来的书籍格式很多插件没有被使用。导致书籍体验很不好。例如：目录插件不生效等...。目前还没发现哪里配置的有问题。

所以没有使用这种自动发布的方式,不使用gitbook在线环境，使用的是github的 pages。
 
## 2. github pages

> GitHub Pages是免费的静态站点，三个特点：免费托管、自带主题、支持自制页面和Jekyll。

静态页面我们依然使用gitbook工具生成。然后按照以下步骤完成书籍自动发布:

1. 安装gitbook工具,本地生成电子书。
2. 安装一些gitbook插件,本地启动服务查看效果。
3. 关联github仓库
4. 创建github pages
5. 使用github action自动构建电子书,并且发布到github pages
6. github pages绑定自定义域名

这种自动发布方式与上面的方式不同点是: 我们相当于是将"本地"构建好的完整书籍项目推送到了gh-pages分支。而不是推送源码后在线上构建。

"本地"加了引号是因为，当我们使用了github action时, github会提供CI环境为我们进行构建并且将构建好的项目推送到指定分支。所以并不需要你真的每次都在本地构建,然后推送。其实,你每次推送的还是源码文件,剩下的全部都是自动执行。

#  GitBook命令工具

## 安装

```bash
npm install -g gitbook-cli

gitbook -V 

gitbook ls 

# 按照提示装好版本

```

## gitook命令使用

```bash
gitbook help # 查看帮助

gitbook init [book]。 初始化电子书。会在你指定目录下生成README和SUMMARY

gitbook install 安装book.json指定的插件

gitbook build  构建书籍。生成的静态文件在_book中

gitbook serve 启动本地服务,查看电子书

# 还有一些导出书籍命令，可通过帮助文档查看。
```

##  书籍构建结构

这里介绍一下构建书籍的目录结构和文件说明

```bash
├── .bookignore    # gitbook忽略文件。主要用来指定 gitbook build时不打包到_book的文件
├── .git 
├── .github  # github action 工作流目录
├── .gitignore # 提交到github仓库忽略的文件
├── README.md  # 书籍介绍
├── SUMMARY.md # 书籍目录
├── _book   # 构建出来的静态文件。此文件夹内就是我们要发布到pages的静态资源
├── book.json # gitbook配置文件
├── golang  # 自定义的书籍目录
├── img  # 文章图片
├── node_modules # 插件目录
└── sortware # 自定义的书籍目录
```

* 目录文件构建方式

```bash
# Summary

* [Introduction](README.md)
* [Golang](golang/README.md)
  * [Go语言核心](golang/kernel/README.md)
    * [Go语言如何测试](golang/kernel/Go语言测试.md)
  * [Go语言三方库](golang/lib/README.md)
    * [Test包](golang/lib/aa.md)
  * [Go经典面试题](golang/question/README.md)
* [软件安装和使用](software/README.md)
  * [GitBook搭建并且关联到GitHub Pages](software/GitBook搭建并且关联到GitHub.md)
```

* 每个文件件下要有自己的README文件。
* 目录文件夹下不能再有目录。不支持书籍嵌套。


* 插件配置

```json
   "plugins": [
        "back-to-top-button",
        "chapter-fold",
        "expandable-chapters-small",
        "code",
        "copy-code-button",
        "-lunr",
        "-search",
        "search-pro",
        "advanced-emoji",
        "github",
        "splitter",
        "page-toc-button",
        "alerts",
        "flexible-alerts",
        "pageview-count",
        "auto-scroll-table",
        "popup",
        "tbfed-pagefooter"

    ],
    "pluginsConfig": {
        "github": {
            "url": "https://github.com"
        },
        "tbfed-pagefooter": {
            "copyright":"Copyright &copy Du Yong ",
            "modify_label": "该文件修订时间：",
            "modify_format": "YYYY-MM-DD HH:mm:ss"
        },
        "page-toc-button": {
            "maxTocDepth": 2,
            "minTocSize": 2
        }
    }
```
插件可以参考这边文章 [GitBook插件整理](https://www.jianshu.com/p/427b8bb066e6)

# github pages配置

官网介绍 [关于 GitHub Pages](https://docs.github.com/cn/pages/getting-started-with-github-pages/about-github-pages#)

> GitHub Pages 是一项静态站点托管服务，它直接从 GitHub 上的仓库获取 HTML、CSS 和 JavaScript 文件，（可选）通过构建过程运行文件，然后发布网站。 您可以在 GitHub Pages 示例集合中查看 GitHub Pages 站点的示例。
> 

站点类型:

* 组织 要发布组织站点，必须创建名为 <organization>.github.io 的组织所拥有的仓库
* 用户 要发布用户站点，必须创建用户帐户所拥有的名为 <username>.github.io
* 项目 项目站点源文件和项目本身存储在一个仓库中。在不使用自定义域名的情况下,项目站点访问http(s)://<username>.github.io/<repository>

我采用项目站点类型。因为项目站点可以有多个。而个人或者组织站点只能有一个。

1. 新建仓库 gitbook-note
2. 关联本地仓库

```bash
git init 
git remote add origin git@github.com:xiaodulala/gitbook-note.git
# 推送到github仓库 注意,要将_book  node_modules两个文件夹的内容忽略掉。只提交书籍的源文件。不要提交构建出来的静态资源文件和下载的插件。
git push origin master 
```
3. 构建书籍

```bash
# 使用build 构建书籍。默认目录为_book. 注意我们不希望构建出来的静态目录有一些其他的文件。要选择忽略掉。 编写.bookignore 忽略一些文件：
.gitignore
.github
.bookignore

#构建 
gitbook build
```

4. 创建远程分支gh-pages分支.并与本地静态资源仓库matser分支关联

```bash
cd _book
git init
git remote add origin git@github.com:xiaodulala/gitbook-note.git
git push --force --quiet "git@github.com:xiaodulala/gitbook-note.git" master:gh-pages
```

以上操作后,我们可以在仓库设置中的page页面查看访问地址。

到目前为止,是我们手动构建的。接着要把构建并发布自动化。使用github actions.



# github actions配置

> github actions 是github上用来持续集成和部署的功能。 每次持续继承偶读需要拉取代码、跑测试用例、合并分支、服务部署和发布等操作。github就把这些操作称为actions.
> 

github允许开发者把每个操作写成独立的脚本，存放到代码仓库里。供其他人引用。所以，当我们的项目需要使用github actions时，就不需要编写复杂的从0开始的脚本。我们可以直接引用别人写好的actions. [github actions 官方市场](https://github.com/marketplace?type=actions)

gitbook的发布我选用了 这一个插件。

工作流文件如下: 

```yaml
name: Build and Publish My GitBook

on:
  workflow_dispatch:
  push:
    branches:
      - master

jobs:
  build:
    name: Build Gitbook
    runs-on: ubuntu-latest
    steps:
      # Check out the repo first
      - name: Checkout code
        uses: actions/checkout@v2
      # Run this action to publish gitbook
      - name: Publish
        uses: tuliren/publish-gitbook@v1.0.0
        with:
          # specify either github_token or personal_token
          github_token: ${{ secrets.GITHUB_TOKEN }}
          # personal_token: ${{ secrets.PERSONAL_TOKEN }}
```


这样，我们就可以在本地推送源文件后，自动发布了。你可以在github的action选项中查看构建过程和结果。


