![](../../img/golang/18.jpeg)

# 概述

在Go项目开发中,我们需要保证我们开发的模块功能稳定，且性能高效。所以我们要对我们自己的模块进行单元测试和性能测试。

在Go语言中，提供了Testing包来对我们的代码进行测试。

# 测试用例规范

* 测试用例文件必须以\_test.go结尾。go test命令执行时会遍历当前包下所有的以\_test.go文件作为测试用例源码文件。
* 测试用例函数必须以 Test、Benchmark、Example开头,后面直接跟函数名，函数名首字母需要大写。如：TestPrintHello。如果一个函数有多个测试用例,函数名称尽量表达出此函数的测试目的。
* 测试用例中变量命名规范: 测试用例中我们经常会定义输入和输出变量,最后比较输入和输出来判断测试用例是否通过。这两类变量通常定义为 expencted/actual或者是got/want。

# 单元测试

单元测试都是以Test开头。函数参数必须为 *test.T

如:

```go
func TestPrintHello(t *testing.T) {
	want := "dy"

	if got := PrintHello("dy"); got != want {
		t.Errorf("want %s,bug got %s", want, got)
	}
}
```
这样的写法需要每次使用`if`比较输入和输出。我们可以使用`github.com/stretchr/testify`包来直接对比输入和输出。

```go
func TestPrintHello(t *testing.T) {
	want := "dy"
	got := PrintHello("dy")
	assert.Equal(t, want, got, "values should be equal")
}
```

* 执行go test需要指定包路径,否则默认执行当前路径下的包的测试用例

```bash
# 以上测试用例执行
go test ./user
```

* go test -v参数,显示所有测试函数的运行细节。

```bash
go test -v ./user
```

* go test -run=<正则表达式> 指定要指定的测试函数

```bash
go test -run="TestPrint.*"
```

* go test -count=N 指定函数执行次数

```bash
go test -v -run="TestPrint.*" -count=2 ./user
```

## 多输入测试用例

如果测试用例中要枚举多个输入进行测试。最好的方式是定义一个输出输出结构,遍历执行并对比结果:

```go
func TestPrintHelloMutil(t *testing.T) {
	tests := []struct {
		arg  string
		want string
	}{
		{arg: "dy", want: "dy"},
		{arg: "abc", want: "abc"},
	}

	for _, tt := range tests {
		got := PrintHello(tt.arg)
		assert.Equal(t, tt.want, got, "values should be equal")
	}
}
```

## 自动生成单元测试代码

通过上面的示例我们大概了解,大部分的测试用例基本套路就是定义参数得到got。和之前的want做比较。这样就可以抽象出一个模型。

为了减少编写测试用例的时间,我们可以使用 [gotests](https://github.com/cweill/gotests)库来自动生成测试用例代码。这个库就是上面模型的实现。

* 安装工具

```bash
# go <=1.16
go get -u github.com/cweill/gotests/...

# go 1.17
go install github.com/cweill/gotests/...@latest
```

* 生成测试用例代码

```bash
cd /users

gotests -all -w .
```

* 补全代码

```go
func TestNewUser(t *testing.T) {
	type args struct {
		name string
		age  uint8
	}
	tests := []struct {
		name    string
		args    args
		want    *User
		wantErr bool
	}{
		// TODO: Add test cases.
		{name: "with all", args: args{name: "dy", age: 20}, want: &User{Name: "dy", Age: 20}, wantErr: false},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got, err := NewUser(tt.args.name, tt.args.age)
			if (err != nil) != tt.wantErr {
				t.Errorf("NewUser() error = %v, wantErr %v", err, tt.wantErr)
				return
			}
			if !reflect.DeepEqual(got, tt.want) {
				t.Errorf("NewUser() = %v, want %v", got, tt.want)
			}
		})
	}
}
```

补全: // TODO: Add test cases.
 
#  性能测试

*  函数名称必须以Benchmark开头
*  参数必须是 Testing.B  

```go
# b.N 为循环次数。N会在运行时自动调整。知道性能测试可以持续运行足够的时间。
func BenchmarkMax(b *testing.B) {
	for i := 0; i < b.N; i++ {
		Max(1, 2)
	}
}
```

* 运行性能测试函数

```bash
go test -bench=".*" `

# 输出
goos: darwin  
goarch: amd64
pkg: fuck_demo/user
cpu: Intel(R) Core(TM) i5-5287U CPU @ 2.90GHz
BenchmarkMax-4          315559854                3.681 ns/op
PASS
ok      fuck_demo/user  2.086s

# BenchmarkMax-4  4个cpu线程参与了此次测试。
# 315559854 循环了多少次。
# 3.681 ns/op 每次操作耗时 3.681纳秒。
```

需要注意,如果在性能测试函数中有一些耗时的初始化操作，这个时间不能计算在性能测试之内。所以需要重置性能计数。

```go
func BenchmarkMax(b *testing.B) {

	// 耗时操作
	fmt.Println("do something")
	time.Sleep(time.Second * 1)

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		Max(1, 2)
	}
}
```

```bash
# 显示使用内存信息
go test -bench=".*" -benchmem

BenchmarkMax-4          323694256                3.678 ns/op           0 B/op          0 allocs/op

# 0 B/op 每次执行分配了多少内存
#  0 allocs/op 每次执行分配了多少**次**内存
都是越少越好。
```

```bash
# 指定参与的cpu个数
go test -bench=".*"  -GOMAXPROCS=2
```

```bash
# 指定测试时间(N)和循环次数(Nx)
go test -bench=".*" -benchtime=10s 执行10秒
go test -bench=".*" -benchtime=100x 执行100次
```

```bash
# 指定测试超时时间
go test -bench=".*" -timeout=20s
```



# 示例测试

如果你写的模块需要被其他人调用。你可以在代码中写示例测试，用来演示你模块的使用方式。

* 示例测试一般保存在example_test.go文件中。
* 函数必须以Example开头，没有输入参数，没有返回值。
* 示例测试通过输出注释来判断测试是否通过.输出注释格式为 Output: 结果值 或者Unordered output:开头的注释

```go
func ExampleFunc() {
	fmt.Println(strings.HasPrefix("_abc", "_"))
	fmt.Println(math.Abs(-100))
	// Output:
	// true
	// 100
}
```

* 示例函数命名规则

```go

func Example() { ... } // 代表了整个包的示例
func ExampleF() { ... } // 函数F的示例
func ExampleT() { ... } // 类型T的示例
func ExampleT_M() { ... } // 方法T_M的示例

# 当一个函数 类型 或者方法有多个示例测试时
func ExampleReverse()
func ExampleReverse_second()
func ExampleReverse_third()
```

## 大型示例测试

通常在一个文件中。只用一个Example函数。


# TestMain函数

主要用来做测试之前的准备工作和测试之后的清理工作。如连接数据库,清理临时文件等。

* 函数名必须是TestMain
* 参数必须是 *testing.M

```go
func TestMain(m *testing.M) {
	fmt.Println("do some stepup")
	m.Run()
	fmt.Println("do some cleanup")
}
```

# Mock测试

在单元测试中,我们经常会碰到如下情况：  
1. 函数内部调用了数据库操作等外部依赖。
2. 函数内部包含了一些未实现的调用。

此时,我们可以通过mock来处理。gomock是go官方提供的mock解决方案。 主要分为两部分:
gomock库和mockgen

* gomock包用来完成对象生命周期的管理。
* mockgen工具用来生成interface对应的mock类源文件。

## 安装

```bash
# gomock包下载
go get github.com/golang/mock/gomock
# mockgen工具下载
go install github.com/golang/mock/mockgen@latest
```


##  示例

假设我们现在user中有一个函数是获取用户的微信UUID，但是此方法还没有实现。所以我们没有办法实现这个函数的测试用例。在这种情况下，我们就需要使用mock测试了。

```go
# user.go
func GetWechatUUID(wechater wechat.Wechater, name string) string {
	uuid := wechater.GetUUID(name)
	return uuid
}
# interface
package wechat

type Wechater interface {
	GetUUID(name string) string
}
```

* 首先，使用mockgen工具,生成要mock的接口的实现。

```bash
mockgen -destination wechat/mock/mock_wechat.go -package mock_wechat  fuck_demo/wechat Wechater

# -destination: 存放mock类代码的文件。如果你没有设置这个选项，代码将被打印到标准输出
# -package: 用于指定mock类源文件的包名。如果你没有设置这个选项，则包名由mock_和输入文件的包名级联而成
# fuck_demo/wechat 是你接口所在的包
# Wechater 接口名称。可以是多个。用,分隔

```

* 使用mock文件,完成单元测试。

可以看到在指定路径下生成了mock_wechat.go文件。其中定义了一些函数和方法。这些方法用来支持我们编写单元测试。

```go
// GetUUID indicates an expected call of GetUUID.
func (mr *MockWechaterMockRecorder) GetUUID(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetUUID", reflect.TypeOf((*MockWechater)(nil).GetUUID), arg0)
}
```

```go
# 单元测试中使用

func TestGetUUID(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	mockWechater := mock_wechat.NewMockWechater(ctrl)
	mockWechater.EXPECT().GetUUID("dy").Return("dy")
	got := GetWechatUUID(mockWechater, "dy")
	if got != "dy" {
		t.Errorf("get uuid fail")
	}
}
```

通过mock,不用我们自己去实现一个接口。降低了用例编写的复杂度。

## mockgen的使用


1. 源码模式

如果有接口文件,则通过源码模式来生成mock代码:

```bash
mockgen -destination wechat/mock/mock_wechat.go  -source wechat/wechat-interface.go 
# -source 要模拟的接口文件
```

2. 反射模式

```bash
mockgen -destination wechat/mock/mock_wechat.go -package mock_wechat  fuck_demo/wechat Wechater
# 我们上面的示例用的是这种方式。
```

3. 注释模式

如果要模拟的接口文件有多个，且分布在不同的文件中。我们需要对每个文件执行多次mockgen命令。mockgen 提供了一种通过注释生成mock文件的方式,需要借助go generate工具

在接口文件代码中,添加以下注释:

```bash
//go:generate mockgen -destination mock/mock_wechat.go -package wechat fuck_demo/wechat Wechater
type Wechater interface {
	GetUUID(name string) string
}

# 在命令行中执行
go generate ./...
```

## 使用mock代码编写单元测试

```go
# 单元测试中使用

func TestGetUUID(t *testing.T) {
	ctrl := gomock.NewController(t)
	defer ctrl.Finish()

	mockWechater := mock_wechat.NewMockWechater(ctrl)
	mockWechater.EXPECT().GetUUID("dy").Return("dy")
	got := GetWechatUUID(mockWechater, "dy")
	if got != "dy" {
		t.Errorf("get uuid fail")
	}
}
```

1. 创建mock控制器
2. defer 操作完之后回收。
3. 使用控制器返回一个mock对象。
4. mock对象调用

gomock 支持以下参数匹配：

* gomock.Any()，可以用来表示任意的入参。
* gomock.Eq(value)，用来表示与 value 等价的值。
* gomock.Not(value)，用来表示非 value 以外的值。
* gomock.Nil()，用来表示 None 值。

EXPECT()得到 Mock 的实例，然后调用 Mock 实例的方法，该方法返回第一个Call对象，然后可以对其进行条件约束，比如使用 Mock 实例的 Return 方法约束其返回值。Call对象还提供了以下方法来约束 Mock 实例：

```go

func (c *Call) After(preReq *Call) *Call // After声明调用在preReq完成后执行
func (c *Call) AnyTimes() *Call // 允许调用次数为 0 次或更多次
func (c *Call) Do(f interface{}) *Call // 声明在匹配时要运行的操作
func (c *Call) MaxTimes(n int) *Call // 设置最大的调用次数为 n 次
func (c *Call) MinTimes(n int) *Call // 设置最小的调用次数为 n 次
func (c *Call) Return(rets ...interface{}) *Call //  // 声明模拟函数调用返回的值
func (c *Call) SetArg(n int, value interface{}) *Call // 声明使用指针设置第 n 个参数的值
func (c *Call) Times(n int) *Call // 设置调用次数为 n 次
```

# fake测试

根据接口伪造一个实现接口的实例。


# 测试覆盖率


* 生成测试覆盖率数据

```bash
# 当前目录下所有文件全部提取,查看是否有对应的测试用例。生成测试覆盖率数据
go test -coverprofile=coverage.out ./...
```

* 分析覆盖率文件

```bash
go tool cover -func=coverage.out

# 输出
fuck_demo/user/user.go:13:      NewUser         100.0%
fuck_demo/user/user.go:20:      GetName         0.0%
fuck_demo/user/user.go:24:      SetAge          0.0%
fuck_demo/user/user.go:29:      PrintHello      0.0%
fuck_demo/user/user.go:33:      Max             0.0%
fuck_demo/user/user.go:37:      GetWechatUUID   100.0%
total:                          (statements)    37.5%
```

生成html文件在浏览器查看

```bash
go tool cover -html=coverage.out -o coverage.html
```

> [!NOTE]
> 有时候代码测试覆盖率会作为准许合入分支的一项检查。如果覆盖率不足，会导致合入分支失败。我们可以使用go-junit-report 将覆盖率结构文件转换为xml,供其他CI系统使用。

---

> [!WARNING]
> 使用mock生成的代码是不需要测试用例的，我们需要将生成的覆盖率文件中mock_*.go的文件去掉，否则会影响整体的测试覆盖率计算。


# 其他mock

* [sqlmock](https://github.com/DATA-DOG/go-sqlmock) 模拟数据库连接
* [httpmock](https://github.com/jarcoal/httpmock) 模拟http请求
* [bouk/monkey](https://github.com/bouk/monkey) 猴子补丁,替换函数指针来修改任意函数的实现。mock最终解决方案。





