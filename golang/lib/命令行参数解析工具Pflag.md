# 概述

> pflag是一个更加高级的命令行参数解析工具。  
> pflag是一个完全兼容goflag的包。但其功能更加强大。按照POSIX/ gnu风格的 --flags实现。

在Go项目中，我们经常会在实现各种服务或者命令行工具时使用命令行参数解析来控制应用行为。Pflag包可以为我们提供更加方便的命令行参数解析。

Kubernetes、Helm、Docker、Etcd等项目使用的也是Pflag包来解析命令行参数的。


# Pflag包安装

```bash
go get github.com/spf13/pflag
```

# Pflag包的使用

* 导入包

```go
import flag "github.com/spf13/pflag"
```

*  pflag.type()方式: 使用长选项的方式将标志解析到指针变量中。 可设置默认值和帮助信息.

```go
func main() {
	var ip *int = pflag.Int("flagname", 123, "help message for flagname")
	pflag.Parse()
	fmt.Printf("%d\n", *ip)
}
```

```bash
☁  pflagDemo  ./main 
123
☁  pflagDemo  ./main --flagname 234
234
☁  pflagDemo  ./main -h
Usage of ./main:
      --flagname int   help message for flagname (default 123)
pflag: help requested
```

*  pflag.typeVar()方式: 先声名接收变量，使用长选项的方式将falg的值`绑定`到指定的变量中。

```go
var flagvar int
var flagbool bool

func main() {
	pflag.IntVar(&flagvar, "flagname", 1234, "help message")
	pflag.BoolVar(&flagbool, "flagbool", true, "flagbool help message")
	pflag.Parse()
	fmt.Printf("%v,%v\n", flagvar, flagbool)
}
```

```bash
☁  pflagDemo  ./main 
1234,true

☁  pflagDemo  ./main --flagbool=false --flagname 2345
2345,false
```

*  pflag.typeP() 或者 pflag.typeVarP(). 在以上两种方式的基础上,增加命令行段选项支持。

```go
func main() {
	var ip = pflag.IntP("flagname", "f", 1234, "help message")
	var flagBool bool
	pflag.BoolVarP(&flagBool, "boolname", "b", true, "help message")
	pflag.Parse()
	fmt.Printf("%d,%v\n", *ip, flagBool)
}
```


```bash
./main -b=false -f 3333
```

*  指定了选项，但是没有指定选项值的默认值

```go
func main() {
	var ip = pflag.IntP("flagname", "f", 1234, "help message")
	pflag.Lookup("flagname").NoOptDefVal = "4321"
	pflag.Parse()

	fmt.Println(*ip)

	// ./main          输出 1234  使用默认值
	// ./main  -f      输出 4321  执行了选项,没有指定值。使用NoOptDefVal的值
	// ./main -f=3333  输出 3333
}
```

*   命令行语法

```bash
--flag    // boolean flags, or flags with no option default values
--flag x  // only on flags without a default value
--flag=x
```

```bash
# example1:
bool类型的flag, 或者设置了 noOptDefVal的flag
-f  // ok true或者是noOptDefVal的值
-f=true -f=3333 // ok
-f true  // invalid
-f 3333 // invalid

//example2:
非bool类型的flag, 或者没有设置noOptDefVal的值
-n 1234
-n=1234
-n1234 
都ok
```

*  flag的"规范化"

允许自定义函数.让你的标志名称在代码中使用时更加规范化，且方便用于比较。在命令行中的输入与代码中的标志等价。

例子:

```go
func main() {
	fset := pflag.NewFlagSet("test", pflag.ExitOnError)
	fset.SetNormalizeFunc(wordSepNormalizeFunc)
}

// You want -, _, and . in flags to compare the same. aka --my-flag == --my_flag == --my.flag
func wordSepNormalizeFunc(f *pflag.FlagSet, name string) pflag.NormalizedName {
	from := []string{"-", "_"}
	to := "."
	for _, sep := range from {
		name = strings.Replace(name, sep, to, -1)
	}
	return pflag.NormalizedName(name)
}

// You want to alias two flags. aka --old-flag-name == --new-flag-name
func aliasNormalizeFunc(f *pflag.FlagSet, name string) pflag.NormalizedName {
	switch name {
	case "old-flag-name":
		name = "new-flag-name"
	}
	return pflag.NormalizedName(name)
}
```

*  弃用标志或者标志的简写

```go
func main() {
	v := pflag.String("badflag", "hello", "help message")
	v1 := pflag.IntP("flagname", "f", 123, "help message")
	pflag.CommandLine.MarkDeprecated("badflag", "please use --good-flag instead")
	pflag.CommandLine.MarkShorthandDeprecated("flagname", "please --flagname only")
	pflag.Parse()

	println(*v, *v1)
}
```

```bash
# 不显示 badflag
 ./main -h
Usage of ./main:
      --flagname int   help message (default 123)

# 可以用 badflag 但会出现提示
./main --badflag world
Flag --badflag has been deprecated, please use --good-flag instead

# 使用flagname的短选项会有提示
./main -f 100         
Flag shorthand -f has been deprecated, please --flagname only

```

*  影藏标志

```go
func main() {

	var adminAct string
	pflag.StringVar(&adminAct, "admin", "admin", "help message")

	pflag.CommandLine.MarkHidden("admin")
	pflag.Parse()

}
```

使用 help 无法看到admin 标识


* pflag包数据结构


每个一个命令行参数都会被解析成一个pflag.Flag类型的变量

```go

type Flag struct {
    Name                string // flag长选项的名称
    Shorthand           string // flag短选项的名称，一个缩写的字符
    Usage               string // flag的使用文本
    Value               Value  // flag的值
    DefValue            string // flag的默认值
    Changed             bool // 记录flag的值是否有被设置过
    NoOptDefVal         string // 当flag出现在命令行，但是没有指定选项值时的默认值
    Deprecated          string // 记录该flag是否被放弃
    Hidden              bool // 如果值为true，则从help/usage输出信息中隐藏该flag
    ShorthandDeprecated string // 如果flag的短选项被废弃，当使用flag的短选项时打印该信息
    Annotations         map[string][]string // 给flag设置注解
}
```
Flag的值是一个Value类型的接口，Value的定义如下:

```go
type Value interface {
    String() string // 将flag类型的值转换为string类型的值，并返回string的内容
    Set(string) error // 将string类型的值转换为flag类型的值，转换失败报错
    Type() string // 返回flag的类型，例如：string、int、ip等
}
```

