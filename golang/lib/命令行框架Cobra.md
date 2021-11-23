# 概述

> Cobra既是一个用于创建强大的现代CLI应用程序的库，也是一个用于生成应用程序和命令文件的程序。

* [cobra](https://github.com/spf13/cobra)

>  Cobra建立在commands(命令) arguments(参数) 和flags(标志)的结构之上。

 好的应用程序被使用时就像在读一个句子。用户可以直观的知道如何使用他。  
一般有一下两种模式  `APPNAME VERB NOUN --ADJECTIVE` 或者 `APPNAME COMMAND ARG --FLAG`  

* APPNAME 应用程序
* VERB或者COMMAND 动词。程序要执行的操作。
* NOUN或者ARG。 名词。非选项参数。可以代表操作作用的资源。
* ADJECTIVE或者FALG。形容词。选项参数。操作资源的修饰符。

如:

```
# server是cmd --port 是flag
hugo server --port=1313
```

```
# clone是命令，url是选项参数 --bare是非选项参数
git clone URL --bare
```

# 安装

* 命令行安装

```go
go install github.com/spf13/cobra/cobra@latest
```

* 安装库

```go
go get -u github.com/spf13/cobra

import "github.com/spf13/cobra"
```


# 目录结构

使用Cobra命令行工具时初始化的项目模板。同样在使用Cobra库时，我们最好也按照此模板来定义我们的目录格式。

```bash
 ▾ appName/
    ▾ cmd/
        add.go
        your.go
        commands.go
        here.go
      main.go
```

```go
package main

import (
  "{pathToYourApp}/cmd"
)

func main() {
  cmd.Execute()
}
```


# Cobra库的使用

手动实现Cobra命令框架时,我们需要一个main文件和rootcmd文件。你也可以根据自己的需要增加其他命令。

## 创建rootCmd

我们通常将rootCmd放在cmd/root.go中

```go
var rootCmd = &cobra.Command{
	Use:   "hugo",
	Short: "Hugo is a very fast static site generator", Long: `A Fast and Flexible Static Site Generator built with love by spf13 and friends in Go. Complete documentation is available at http://hugo.spf13.com`,
	Run: func(cmd *cobra.Command, args []string) {
		// Do Stuff Here
	},
}

func Execute() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}

```

我们可以通过viper和pflag(已经集成到cobra中)定义标志和配置绑定。

```go
package cmd

import (
	"fmt"
	"os"

	homedir "github.com/mitchellh/go-homedir"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
)

var rootCmd = &cobra.Command{
	Use:   "hugo",
	Short: "Hugo is a very fast static site generator", Long: `A Fast and Flexible Static Site Generator built with love by spf13 and friends in Go. Complete documentation is available at http://hugo.spf13.com`,
	Run: func(cmd *cobra.Command, args []string) {
		// Do Stuff Here
	},
}

var (
	cfgFile     string
	projectBase string
	userLicense string
)

func init() {
	cobra.OnInitialize(initConfig)
	rootCmd.PersistentFlags().StringVar(&cfgFile, "config", "", "config file (default is $HOME/.cobra.yaml)")
	rootCmd.PersistentFlags().StringVarP(&projectBase, "projectbase", "b", "", "base project directory eg. github.com/spf13/")
	rootCmd.PersistentFlags().StringP("author", "a", "YOUR NAME", "Author name for copyright attribution")
	rootCmd.PersistentFlags().StringVarP(&userLicense, "license", "l", "", "Name of license for the project (can provide `licensetext` in config)")
	rootCmd.PersistentFlags().Bool("viper", true, "Use Viper for configuration")
	viper.BindPFlag("author", rootCmd.PersistentFlags().Lookup("author"))
	viper.BindPFlag("projectbase", rootCmd.PersistentFlags().Lookup("projectbase"))
	viper.BindPFlag("useViper", rootCmd.PersistentFlags().Lookup("viper"))
	viper.SetDefault("author", "NAME HERE <EMAIL ADDRESS>")
	viper.SetDefault("license", "apache")
}

func initConfig() {
	// Don't forget to read config either from cfgFile or from home directory!
	if cfgFile != "" {
		// Use config file from the flag.
		viper.SetConfigFile(cfgFile)
	} else {
		// Find home directory.
		home, err := homedir.Dir()
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}

		// Search config in home directory with name ".cobra" (without extension).
		viper.AddConfigPath(home)
		viper.SetConfigName(".cobra")
	}

	if err := viper.ReadInConfig(); err != nil {
		fmt.Println("Can't read config:", err)
		os.Exit(1)
	}
}

func Execute() {
	if err := rootCmd.Execute(); err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
```

## 添加命令

可以定义其他命令，并且通常在cmd/目录中给每个命令各自的文件

假设添加一个version命令

```go
// cmd/version.go
package cmd

import (
	"fmt"

	"github.com/spf13/cobra"
)

var versionCmd = &cobra.Command{
	Use:   "version",
	Short: "Print the version number of Hugo",
	Long:  `All software has versions. This is Hugo's`,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println("Hugo Static Site Generator v0.9 -- HEAD")
	},
}

func init() {
	rootCmd.AddCommand(versionCmd)
}
```

## 返回错误

使用`RunE`回调返回错误

```go
var versionCmd = &cobra.Command{
	Use:   "version",
	Short: "Print the version number of Hugo",
	Long:  `All software has versions. This is Hugo's`,
	RunE: func(cmd *cobra.Command, args []string) error {
		if err := someFunc(); err != nil {
			return err
		}
		return nil
	},
}

func init() {
	rootCmd.AddCommand(versionCmd)
}

func someFunc() error {
	return errors.New("version cmd is invalid")
}
```


## 同Flag一起使用

> Falg提供了修饰符来控制命令如何执行。

> 我们可以为命令分配标志。但是由于标Flag是在不同的位置定义和使用的，所以我们需要在外部定义一个具有正确作用域的变量，以便为标记赋值

### 持久标志

> 一个标志可以是'persistent'，这意味着这个标志将对它所分配的命令以及该命令下的每个命令可用。对于全局标志，在根上指定一个标志作为持久标志。


```go
rootCmd.PersistentFlags().BoolVarP(&Verbose, "verbose", "v", false, "verbose output")
```

###  本地标志

> 也可以在本地分配标志，该标志将只应用于该特定的命令。

```go
localCmd.Flags().StringVarP(&Source, "source", "s", "", "Source directory to read from")
```

### 父命令的本地标志

> 默认情况下，Cobra只解析目标命令上的本地标志，父命令上的任何本地标志都会被忽略。通过启用命令`Command.TraverseChildren`。Cobra将在执行目标命令之前解析每个命令的本地标志
> 

```go
command := cobra.Command{
  Use: "print [OPTIONS] [COMMANDS]",
  TraverseChildren: true,
}
```

## 同Vipier一起使用

```go
var author string

func init() {
  rootCmd.PersistentFlags().StringVar(&author, "author", "YOUR NAME", "Author name for copyright attribution")
  viper.BindPFlag("author", rootCmd.PersistentFlags().Lookup("author"))
}
```
在本例中，使用viper绑定持久标志author。注意:当user提供了——author标志时，变量author将不会被设置为config中的值。


## 标志必选

```go
// 持久标志
rootCmd.PersistentFlags().StringVarP(&Region, "region", "r", "", "AWS region (required)")
rootCmd.MarkPersistentFlagRequired("region")
// 本地标志
rootCmd.Flags().StringVarP(&Region, "region", "r", "", "AWS region (required)")
rootCmd.MarkFlagRequired("region")
```

## 非选项参数验证

在命令的过程中，经常会传入非选项参数，并且需要对这些非选项参数进行验证，Cobra 提供了机制来对非选项参数进行验证。可以使用 Command 的 Args 字段来验证非选项参数。Cobra 也内置了一些验证函数：

* NoArgs：如果存在任何非选项参数，该命令将报错。
* ArbitraryArgs：该命令将接受任何非选项参数。
* OnlyValidArgs：如果有任何非选项参数不在 Command 的 ValidArgs 字段中，该命令将报错。
* MinimumNArgs(int)：如果没有至少 N 个非选项参数，该命令将报错。
* MaximumNArgs(int)：如果有多于 N 个非选项参数，该命令将报错。
* ExactArgs(int)：如果非选项参数个数不为 N，该命令将报错。
* ExactValidArgs(int)：如果非选项参数的个数不为 N，或者非选项参数不在 Command 的 ValidArgs 字段中，该命令将报错。
* RangeArgs(min, max)：如果非选项参数的个数不在 min 和 max 之间，该命令将报错。

```go
var cmd = &cobra.Command{
  Short: "hello",
  Args: func(cmd *cobra.Command, args []string) error { 
    if len(args) < 1 {
      return errors.New("requires a color argument")
    }
    if myapp.IsValidColor(args[0]) {
      return nil
    }
    return fmt.Errorf("invalid color specified: %s", args[0])
  },
  Run: func(cmd *cobra.Command, args []string) {
    fmt.Println("Hello, World!")
  },
}
```

## 多个顶层命令例子

```go
package main

import (
	"fmt"
	"strings"

	"github.com/spf13/cobra"
)

func main() {
	var echoTimes int

	var cmdPrint = &cobra.Command{
		Use:   "print [string to print]",
		Short: "Print anything to the screen",
		Long: `print is for printing anything back to the screen.
For many years people have printed back to the screen.`,
		Args: cobra.MinimumNArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			fmt.Println("Print: " + strings.Join(args, " "))
		},
	}

	var cmdEcho = &cobra.Command{
		Use:   "echo [string to echo]",
		Short: "Echo anything to the screen",
		Long: `echo is for echoing anything back.
Echo works a lot like print, except it has a child command.`,
		Args: cobra.MinimumNArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			fmt.Println("Echo: " + strings.Join(args, " "))
		},
	}

	var cmdTimes = &cobra.Command{
		Use:   "times [string to echo]",
		Short: "Echo anything to the screen more times",
		Long: `echo things multiple times back to the user by providing
a count and a string.`,
		Args: cobra.MinimumNArgs(1),
		Run: func(cmd *cobra.Command, args []string) {
			for i := 0; i < echoTimes; i++ {
				fmt.Println("Echo: " + strings.Join(args, " "))
			}
		},
	}

	cmdTimes.Flags().IntVarP(&echoTimes, "times", "t", 1, "times to echo the input")

	var rootCmd = &cobra.Command{Use: "app"}
	rootCmd.AddCommand(cmdPrint, cmdEcho)
	cmdEcho.AddCommand(cmdTimes)
	rootCmd.Execute()
}

```

## 帮助选项和版本选项


* 如果在根命令上设置了version字段，Cobra会添加一个顶级的'——version'标志。使用'——version'标志运行应用程序将使用版本模板将版本打印到标准输出。模板可以通过cmd自定义。SetVersionTemplate(字符串)函数。

## 命令执行的钩子函数

在运行 Run 函数时，我们可以运行一些钩子函数，比如 PersistentPreRun 和 PreRun 函数在 Run 函数之前执行，PersistentPostRun 和 PostRun 在 Run 函数之后执行。如果子命令没有指定Persistent\*Run函数，则子命令将会继承父命令的Persistent\*Run函数。这些函数的运行顺序如下：

* PersistentPreRun
* PreRun
* Run
* PostRun
* PersistentPostRun

```go
package main

import (
  "fmt"

  "github.com/spf13/cobra"
)

func main() {

  var rootCmd = &cobra.Command{
    Use:   "root [sub]",
    Short: "My root command",
    PersistentPreRun: func(cmd *cobra.Command, args []string) {
      fmt.Printf("Inside rootCmd PersistentPreRun with args: %v\n", args)
    },
    PreRun: func(cmd *cobra.Command, args []string) {
      fmt.Printf("Inside rootCmd PreRun with args: %v\n", args)
    },
    Run: func(cmd *cobra.Command, args []string) {
      fmt.Printf("Inside rootCmd Run with args: %v\n", args)
    },
    PostRun: func(cmd *cobra.Command, args []string) {
      fmt.Printf("Inside rootCmd PostRun with args: %v\n", args)
    },
    PersistentPostRun: func(cmd *cobra.Command, args []string) {
      fmt.Printf("Inside rootCmd PersistentPostRun with args: %v\n", args)
    },
  }

  var subCmd = &cobra.Command{
    Use:   "sub [no options!]",
    Short: "My subcommand",
    PreRun: func(cmd *cobra.Command, args []string) {
      fmt.Printf("Inside subCmd PreRun with args: %v\n", args)
    },
    Run: func(cmd *cobra.Command, args []string) {
      fmt.Printf("Inside subCmd Run with args: %v\n", args)
    },
    PostRun: func(cmd *cobra.Command, args []string) {
      fmt.Printf("Inside subCmd PostRun with args: %v\n", args)
    },
    PersistentPostRun: func(cmd *cobra.Command, args []string) {
      fmt.Printf("Inside subCmd PersistentPostRun with args: %v\n", args)
    },
  }

  rootCmd.AddCommand(subCmd)

  rootCmd.SetArgs([]string{""})
  rootCmd.Execute()
  fmt.Println()
  rootCmd.SetArgs([]string{"sub", "arg1", "arg2"})
  rootCmd.Execute()
}

```

## 当“未知命令”发生时的建议

默认开启。如果要关闭执行:

```go
command.DisableSuggestions = true
// 或者
command.SuggestionsMinimumDistance = 1
```
## 为命令生成文档

[文档](https://github.com/spf13/cobra/blob/master/doc/md_docs.md)

### 简单使用

```go
package main

import (
	"log"

	"github.com/spf13/cobra"
	"github.com/spf13/cobra/doc"
)

func main() {
	cmd := &cobra.Command{
		Use:   "test",
		Short: "my test program",
	}
	err := doc.GenMarkdownTree(cmd, "/tmp")
	if err != nil {
		log.Fatal(err)
	}
}
```

### 为整个命令树生成文档

```go
package main

import (
	"log"
	"io/ioutil"
	"os"

	"k8s.io/kubernetes/pkg/kubectl/cmd"
	cmdutil "k8s.io/kubernetes/pkg/kubectl/cmd/util"

	"github.com/spf13/cobra/doc"
)

func main() {
	kubectl := cmd.NewKubectlCommand(cmdutil.NewFactory(nil), os.Stdin, ioutil.Discard, ioutil.Discard)
	err := doc.GenMarkdownTree(kubectl, "./")
	if err != nil {
		log.Fatal(err)
	}
}
```

## shell自动补全

[文档](https://github.com/spf13/cobra/blob/master/shell_completions.md)