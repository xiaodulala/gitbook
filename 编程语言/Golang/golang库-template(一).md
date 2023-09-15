# 引用

摘自 [Go Web 编程之 模板（一）](https://darjun.github.io/2019/12/31/goweb/template1/)

> 在Go 语言中，text/template和html/template两个库实现模板功能。

# 初体验

使用模板引擎一般有 3 个步骤：
1. 定义模板（直接使用字符串字面量或文件）；
2. 解析模板（使用text/template或html/template中的方法解析）；
3. 传入数据生成输出。

```go
package main

import (
	"log"
	"os"
	"text/template"
)

type User struct {
	Name string
	Age  int
}

func stringLiteralTemplate() {
	s := "My name is {{ .Name }}. I am {{ .Age }} years old.\n"
	t, err := template.New("test").Parse(s)
	if err != nil {
		log.Fatal("Parse string literal template error:", err)
	}

	u := User{Name: "darjun", Age: 28}
	err = t.Execute(os.Stdout, u)
	if err != nil {
		log.Fatal("Execute string literal template error:", err)
	}
}

func fileTemplate() {
	t, err := template.ParseFiles("/Users/duyong/workspace/gotest/test-template/test.tmpl")
	if err != nil {
		log.Fatal("Parse file template error:", err)
	}

	u := User{Name: "dj", Age: 18}
	err = t.Execute(os.Stdout, u)
	if err != nil {
		log.Fatal("Execute file template error:", err)
	}
}

func main() {
	stringLiteralTemplate()

	fileTemplate()
}

```

# 动作

Go 模板中的动作就是一些嵌入在模板里面的命令。动作大体上可以分为以下几种类型：

* 点动作；
* 条件动作；
* 迭代动作；
* 设置动作；
* 包含动作。

## 点动作

点动作（{{ . }}）。它其实代表是传递给模板的数据，其他动作或函数基本上都是对这个数据进行处理，以此来达到格式化和内容展示的目的。

```go

type User struct {
	Name string
	Age  int
}

func main() {
	// 定义模板
	s := `user is {{ . }}`
	t, err := template.New("test").Parse(s)
	if err != nil {
		log.Fatal("Parse error:", err)
	}
	// 生成模板数据
	u := User{Name: "duyong", Age: 18}

	// 生成输出
	err = t.Execute(os.Stdout, u)
	if err != nil {
		log.Fatal("Parse error:", err)
	}
}

# The user is {darjun 28}.
```

注意：为了使用的方便和灵活，在模板中不同的上下文内，.的含义可能会改变，下面在介绍不同的动作时会进行说明。

## 条件动作

在介绍动作的语法时，我采用 Go 标准库中的写法。我觉得这样写更严谨。 其中pipeline表示管道，后面会有详细的介绍，现在可以将它理解为一个值。 T1/T2等形式表示语句块，里面可以嵌套其它类型的动作。最简单的语句块就是不包含任何动作的字符串。

条件动作的语法与编程语言中的if语句语法类似，有几种形式：

形式一：


`{{ if pipeline }} T1 {{ end }}`


如果管道计算出来的值不为空，执行T1。否则，不生成输出。下面都表示空值：

* false、0、空指针或接口；
* 长度为 0 的数组、切片、map或字符串

形式二：

`{{ if pipeline }} T1 {{ else }} T2 {{ end }}`

如果管道计算出来的值不为空，执行T1。否则，执行T2。

形式三：

`{{ if pipeline1 }} T1 {{ else if pipeline2 }} T2 {{ else }} T3 {{ end }}`

如果管道pipeline1计算出来的值不为空，则执行T1。反之如果管道pipeline2的值不为空，执行T2。如果都为空，执行T3。

举例：

模板文件：

```
Your age is: {{ .Age }}
{{ if .GreaterThan60 }}
Old People!
{{ else if .GreaterThan40 }}
Middle Aged!
{{ else }}
Young!
{{ end }}
```

```go
type AgeInfo struct {
	Age           int
	GreaterThan60 bool
	GreaterThan40 bool
}

func main() {
	t, err := template.ParseFiles("/Users/duyong/workspace/gotest/test-template/test.tmpl")
	if err != nil {
		log.Fatal("Parse error:", err)
	}

	rand.Seed(time.Now().Unix())
	age := rand.Intn(100)
	info := AgeInfo{
		Age:           age,
		GreaterThan60: age > 60,
		GreaterThan40: age > 40,
	}
	err = t.Execute(os.Stdout, info)
	if err != nil {
		log.Fatal("Execute error:", err)
	}
}
```

这个程序有一个问题，会有多余的空格！我们之前说过，除了动作之外的任何文本都会原样保持，包括空格和换行！针对这个问题，有两种解决方案。第一种方案是删除多余的空格和换行，test文件修改为：

```
Your age is: {{ .Age }}
{{ if .GreaterThan60 }}Old People!{{ else if .GreaterThan40 }}Middle Aged!{{ else }}Young!{{ end }}
```

显然，这个方法会导致模板内容很难阅读，不够理想。为此，Go 提供了针对空白符的处理。如果一个动作以{{- （注意有一个空格），那么该动作与它前面相邻的非空文本或动作间的空白符将会被全部删除。类似地，如果一个动作以 -}}结尾，那么该动作与它后面相邻的非空文本或动作间的空白符将会被全部删除。例如：


```
Your age is: {{ .Age }}
{{ if .GreaterThan60 -}}
Old People!
{{- else if .GreaterThan40 -}}
Middle Aged!
{{- else -}}
Young!
{{- end }}
```

## 迭代动作

迭代其实与编程语言中的循环遍历类似。有两种形式：

形式一：

`{{ range pipeline }} T1 {{ end }}`

管道的值类型必须是数组、切片、map、channel。如果值的长度为 0，那么无输出。否则，.被设置为当前遍历到的元素，然后执行T1，即在T1中.表示遍历的当前元素，而非传给模板的参数。如果值是 map 类型，且键是可比较的基本类型，元素将会以键的顺序访问。

形式二：

`{{ range pipeline }} T1 {{ else }} T2 {{ end }}`

例子：

```
Apple Products:
{{ range . }}
{{ .Name }}: ￥{{ .Price }}
{{ else }}
No Products!!!
{{ end }}
```

```go
type Item struct {
	Name	string
	Price	int
}

func main() {
	t, err := template.ParseFiles("test")
	if err != nil {
		log.Fatal("Parse error:", err)
	}

	items := []Item {
		{ "iPhone", 5499 },
		{ "iPad", 6331 },
		{ "iWatch", 1499 },
		{ "MacBook", 8250 },
	}

	err = t.Execute(os.Stdout, items)
	if err != nil {
		log.Fatal("Execute error:", err)
	}
}
```

在range语句循环体内，.被设置为当前遍历的元素，可以直接使用{{ .Name }}或{{ .Price }}访问产品名称和价格

## 设置动作

设置动作使用with关键字重定义.。在with语句内，.会被定义为指定的值。一般用在结构嵌套很深时，能起到简化代码的作用。

形式一：

`{{ with pipeline }} T1 {{ end }}`

如果管道值不为空，则将.设置为pipeline的值，然后执行T1。否则，不生成输出。

形式二：

`{{ with pipeline }} T1 {{ else }} T2 {{ end }}`

与前一种形式的不同之处在于当管道值为空时，不改变.执行T2。举个栗子：


```go
type User struct {
	Name string
	Age  int
}

type Pet struct {
	Name  string
	Age   int
	Owner User
}

func main() {
	t, err := template.ParseFiles("test")
	if err != nil {
		log.Fatal("Parse error:", err)
	}

	p := Pet {
		Name:  "Orange",
		Age:   2,
		Owner: User {
			Name: "dj",
			Age:  28,
		},
	}

	err = t.Execute(os.Stdout, p)
	if err != nil {
		log.Fatal("Execute error:", err)
	}
}
```

```
Pet Info:
Name: {{ .Name }}
Age: {{ .Age }}
Owner:
{{ with .Owner }}
  Name: {{ .Name }}
  Age: {{ .Age }}
{{ end }}
```

```
Pet Info:
Name: Orange
Age: 2
Owner:

  Name: dj
  Age: 28
```

## 包含动作

包含动作可以在一个模板中嵌入另一个模板，方便模板的复用

形式一：

`{{ template "name" }}`

形式二：

`{{ template "name" pipeline }}`

其中name表示嵌入的模板名称。第一种形式，将使用nil作为传入内嵌模板的参数。第二种形式，管道pipeline的值将会作为参数传给内嵌的模板。举个栗子：

```go

package main

import (
	"log"
	"os"
	"text/template"
)

func main() {
	t, err := template.ParseFiles("test1", "test2")
	if err != nil {
		log.Fatal("Parse error:", err)
	}

	err = t.Execute(os.Stdout, "test data")
	if err != nil {
		log.Fatal("Execute error:", err)
	}
}
```

ParseFiles方法接收可变参数，可将任意多个文件名传给该方法

模板test1:

```
This is in test1.
{{ template "test2" }}

{{ template "test2" . }}
```

模板test2:

```
This is in test2.
Get: {{ . }}.
```

输出：

```
This is in test1.
This is in test2.
Get: <no value>.

This is in test2.
Get: test data.
```

# 其它元素

在介绍了几种动作之后，我们回过头来看几种基本组成部分。

## 注释

注释只有一种语法：

`{{ /* 注释 */ }}`

注释的内容不会呈现在输出中，它就像代码注释一样，是为了让模板更易读。

## 参数

一个参数就是模板中的一个值。它的取值有多种：

* 布尔值、字符串、字符、整数、浮点数、虚数和复数等字面量；
* 结构中的一个字段或 map 中的一个键。结构的字段名必须是导出的，即大写字母开头，map 的键名则不必；
* 一个函数或方法。必须只返回一个值，或者只返回一个值和一个错误。如果返回了非空的错误，则Execute方法执行终止，返回该错误给调用者；
* 等等..

`{{ .Field1.Key1.Method1.Field2.Key2.Method2 }`

```
type User struct {
	FirstName 	string
	LastName	string
}

func (u User) FullName() string {
	return u.FirstName + " " + u.LastName
}

func main() {
	t, err := template.ParseFiles("test")
	if err != nil {
		log.Fatal("Parse error:", err)
	}

	err = t.Execute(os.Stdout, User{FirstName: "lee", LastName: "darjun"})
	if err != nil {
		log.Fatal("Execute error:", err)
	}
}
```

```
My full name is {{ .FullName }}
```

模板执行会使用FullName方法的返回值替换{{ .FullName }}

关于参数的几个要点：

* 参数可以是任何类型；
* 如果参数为指针，实现会根据需要取其基础类型；
* 如果参数计算得到一个函数类型，它不会自动调用。例如{{ .Method1 }}，如果Method1方法返回一个函数，那么返回值函数不会调用。如果要调用它，使用内置的call函数。

## 管道

管道的语法与 Linux 中的管道类似，即命令的链式序列：

`{{ p1 | p2 | p3 }}`

每个单独的命令（即p1/p2/p3...）可以是下面三种类型：

* 参数，见上面；
* 可能带有参数的方法调用；
* 可能带有参数的函数调用

**在一个链式管道中，每个命令的结果会作为下一个命令的最后一个参数。最后一个命令的结果作为整个管道的值**

管道必须只返回一个值，或者只返回一个值和一个错误。如果返回了非空的错误，那么Execute方法执行终止，并将该错误返回给调用者。

```go

type Item struct {
	Name  string
	Price float64
	Num   int
}

func (item Item) Total() float64 {
	return item.Price * float64(item.Num)
}

func main() {
	t, err := template.ParseFiles("test")
	if err != nil {
		log.Fatal("Parse error:", err)
	}

	item := Item {"iPhone", 5499.99, 2 }

	err = t.Execute(os.Stdout, item)
	if err != nil {
		log.Fatal("Execute error:", err)
	}
}
```

模板文件:

```
Product: {{ .Name }}
Price: ￥{{ .Price }}
Num: {{ .Num }}
Total: ￥{{ .Total | printf "%.2f" }}
```

先调用Item.Total方法计算商品总价，然后使用printf格式化，保留两位小数。最终输出：
printf是 Go 模板内置的函数，这样的函数还有很多。

## 变量

在动作中，可以用管道的值定义一个变量。

`$variable := pipeline`

`$variable`为变量名，声明变量的动作不生成输出。

类似地，变量也可以重新赋值：

`$variable = pipeline`

在range动作中可以定义两个变量：

`range $index, $element := range pipeline`

这样就可以在循环中通过$index和$element访问索引和元素了。

变量的作用域持续到定义它的控制结构的{{ end }}动作。如果没有这样的控制结构，则持续到模板结束。模板调用不继承变量

执行开始时，$被设置为传入的数据参数，即.的值。

## 函数

Go 模板提供了大量的预定义函数，如果有特殊需求也可以实现自定义函数。模板执行时，遇到函数调用，先从模板自定义函数表中查找，而后查找全局函数表。

### 预定义函数

预定义函数分为以下几类：

* 逻辑运算，and/or/not；
* 调用操作，call；
* 格式化操作，print/printf/println，与用参数直接调用fmt.Sprint/Sprintf/Sprintln得到的内容相同；
* 比较运算，eq/ne/lt/le/gt/ge。

在上面条件动作的示例代码中，我们在代码中计算出大小关系再传入模板，这样比较繁琐，可以直接使用比较运算简化。

有两点需要注意：

* 由于是函数调用，所有的参数都会被求值，没有短路求值；
* 比较运算只作用于基本类型，且没有 Go 语法那么严格，例如可以比较有符号和无符号整数。

### 自定义函数

默认情况下，模板中无自定义函数，可以使用模板的Funcs方法添加。下面我们实现一个格式化日期的自定义函数：

```go
package main

import (
	"log"
	"os"
	"text/template"
	"time"
)

func formatDate(t time.Time) string {
	return t.Format("2016-01-02")
}

func main() {
	funcMap := template.FuncMap {
		"fdate": formatDate,
	}
	t := template.New("test").Funcs(funcMap)
	t, err := t.ParseFiles("test")
	if err != nil {
		log.Fatal("Parse errr:", err)
	}

	err = t.Execute(os.Stdout, time.Now())
	if err != nil {
		log.Fatal("Exeute error:", err)
	}
}
```

模板文件：

`Today is {{ . | fdate }}.`

模板的Func方法接受一个template.FuncMap类型变量，键为函数名，值为实际定义的函数。 可以一次设置多个自定义函数。自定义函数要求只返回一个值，或者返回一个值和一个错误。 设置之后就可以在模板中使用fdate了，输出：

`Today is 7016-01-07.`

这里不能使用template.ParseFiles，因为在解析模板文件的时候fdate未定义会导致解析失败。必须先创建模板，调用Funcs设置自定义函数，然后再解析模板。

注意：确保传递给template.New的参数是传递给ParseFiles的列表中的一个文件的基本名称


# 模板的几种创建方式

我们前面学习了两种模板的创建方式：

* 先调用template.New创建模板，然后使用Parse/ParseFiles解析模板内容；
* 直接使用template.ParseFiles创建并解析模板文件。

第一种方式，调用template.New创建模板时需要传入一个模板名字，后续调用ParseFiles可以传入一个或多个文件，这些文件中必须有一个基础名（即去掉路径部分）与模板名相同。如果没有文件名与模板名相同，则Execute调用失败，返回错误

`Execute error:template: test: "test" is an incomplete or empty template`

* 还有一种创建方式，使用ParseGlob函数。ParseGlob会对匹配给定模式的所有文件进行语法分析。

ParseGlob返回的模板以匹配的第一个文件基础名作为名称。ParseGlob解析时会对同一个目录下的文件进行排序，所以第一个文件总是固定的。

```go
func main() {
	t, err := template.ParseGlob("/Users/duyong/workspace/gotest/test-template/*.tmpl")
	if err != nil {
		log.Fatal("in globTemplate parse error:", err)
	}
	err = t.Execute(os.Stdout, nil)
	if err != nil {
		log.Fatal(err)
	}

	err = t.ExecuteTemplate(os.Stdout, "test.tmpl", nil)
	if err != nil {
		log.Fatal(err)
	}

	err = t.ExecuteTemplate(os.Stdout, "test1.tmpl", nil)
	if err != nil {
		log.Fatal(err)
	}
}
```

注意，如果多个不同路径下的文件名相同，那么后解析的会覆盖之前的。

# 嵌套模板

在一个模板文件中还可以通过{{ define }}动作定义其它的模板，这些模板就是嵌套模板。模板定义必须在模板内容的最顶层，像 Go 程序中的全局变量一样。

嵌套模板一般用于布局（layout）。很多文本的结构其实非常固定，例如邮件有标题和正文，网页有首部、正文和尾部等。 我们可以为这些固定结构的每部分定义一个模板。

定义模板文件layout.tmpl：

```
{{ define "layout" }}
This is body.
{{ template "content" . }}
{{ end }}

{{ define "content" }}
This is {{ . }} content.
{{ end }}
```

上面定义了两个模板layout和content，layout中使用了content。执行这种方式定义的模板必须使用ExecuteTemplate方法：

```go
func main() {
	t, err := template.ParseFiles("layout.tmpl")
	if err != nil {
		log.Fatal("Parse error:", err)
	}

	err = t.ExecuteTemplate(os.Stdout, "layout", "amazing")
	if err != nil {
		log.Fatal("Execute error:", err)
	}
}
```

## 块动作

块动作其实就是定义一个默认模板，语法如下：

```
{{ block "name" arg }}
T1
{{ end }}
```

其实它就等价于定义一个模板，然后立即使用它：

```
{{ define "name" }}
T1
{{ end }}

{{ template "name" arg }}
```