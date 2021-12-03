# Go中的error
在底层的实现中，`error`类型是一个普通的接口。

```go
// The error built-in interface type is the conventional interface for
// representing an error condition, with the nil value representing no error.
type error interface {
	Error() string
}
```
所以，实现我们自己的错误类型非常容易。只要我们实现`Error() string`方法，这个方法的结构体都会被视为一个合法的错误值并可以被返回

```go
type MyError struct {
	code int
	msg string
}

func (e *MyError)Error()string{
	return fmt.Sprintf("code:%d,msg:%s", e.code, e.msg)
}

func main() {
	err:= func1()
	fmt.Println(err)
}

func func1()error{
	return &MyError{code: 1,msg: "resource not found"}
}
```

# 内置的错误字符串(errorString)结构体

我们经常会使用`errors.New("test err")`或者`fmt.Errorf()`来生成一个错误值。这个错误值的结构其实就是errorString。errorString实现了`error`接口的方法,所以可以当做一个合法的错误值。它做的事情就是保存一个string,并且由`error`接口返回。

```go
package errors

// New returns an error that formats as the given text.
// Each call to New returns a distinct error value even if the text is identical.
func New(text string) error {
	return &errorString{text}
}

// errorString is a trivial implementation of error.
type errorString struct {
	s string
}

func (e *errorString) Error() string {
	return e.s
}
```

# errors不是异常(exceptions)

在其他语言中,exceptions机制各不相同。  


c++中，你无法确定被调用者会抛出什么异常。而在java中，引入了checkd exception,方法的所有者必须申明可能会抛出哪些exception,而调用者必须处理。这样的话，java中的异常不再是异常，他们从良性到灾难性的都有，需要由调用者来判断。

go中明确了异常和错误的概念。异常就是程序出现**不可预期**的致命问题,无法再继续运行，引入panic机制。错误是**可以预料**的错误，是业务处理的中的一部分。这里使用的是error。 

**对于真正意外的情况，那些表示不可恢复的程序错误，例如索引越界、不可恢复的环境问题、栈溢出，我们才使用 panic。对于其他的错误情况，我们应该是期望使用 error 来进行判定。**


# 定义 错误的方式

##  sintinel error

预定义的特定错误，叫做 sintinel error。 这是我们经常使用的定义错误的方式。如标准库中的io.EOF。

* 使用预定义判断是最不灵活的错误处理策略。因为调用方必须使用 "\==" 来比较预定义错误的值。当你想要提供更多的上下文时，就出现了一个问题，新增了上下文的错误会影响"=="检查

	```go
	func main() {
		err:=bar()
	
		// 影响==判断
		if err == simple.ErrorA{
			fmt.Println("handler errorA")
		}
		fmt.Println(err)
	}
	
	func bar()error{
		err:=simple.SimpleA()
		if err!=nil{
			// 增加了上下文信息
			return fmt.Errorf("bar:%w",err)
		}
		return nil
	}	
	```
* 不应该依赖检测error.Error()的输出。Error方法只是用来方便程序员输出字符串用来记录日志。而不应该被程序依赖(测试用例可能会用)。
* 预定义特定错误模式在两个包之间创建了源代码的依赖关系。例如检查错误是否等于io.EOF。你的代码必须导入io包。如果有很多包都需要导出各自的错误值。可能存在耦合。

所以我们应该尽量避免使用预定义特定错误的方式。

## error types
 
 实现 `error interface `接口的自定义错误类型。使用这种方式我们可以为错误信息增加更丰富的上下文信息。如标准库:
 
 ```go
 type PathError struct {
	Op   string
	Path string
	Err  error
}

func (e *PathError) Error() string { return e.Op + " " + e.Path + ": " + e.Err.Error() }
 ```
 
 但是在使用时,我们必须进行断言来判断是否是此错误:
 
 ```go
func main() {
	err:=pathErr()
	if err!=nil{
		if _,ok:=err.(*fs.PathError);ok{
			fmt.Println("handle path error")
		}
	}

	// normal
}

func pathErr()error{
	return &fs.PathError{
		Op:   "create",
		Path: "/home/test",
		Err:  errors.New("filepath not found"),
	}
}
 ```
 
 这种方式比sentinel方式好一些，因为它能可以捕获更多的上下文信息。但sentinel存在的一些问题依然没有得到解决。
 
 ## 为自定义错误增加特定的行为
 
net标准库

```go
// An Error represents a network error.
type Error interface { //扩展error接口。增加行为
	error
	Timeout() bool   // Is the error a timeout?
	Temporary() bool // Is the error temporary?
}

type UnknownNetworkError string
func (e UnknownNetworkError) Error() string   { return "unknown network " + string(e) }
func (e UnknownNetworkError) Timeout() bool   { return false }
func (e UnknownNetworkError) Temporary() bool { return false }
```

在这种情况下,我们可以直接判断错误的行为。而不是去判断错误的类型或者值。我们也无需再再关注底层的err类型。


# 常见的error处理和存在的问题

在error中最常见的处理方式就是通过多值进行返回，明了的暴露出来，要么处理，要么略过。最常见的处理方式如下：

```go
func bar()error{
	err:= foo()
	if err!=nil{
		return fmt.Errorf("bar:%s",err)
	}
	return nil
}

func foo()error{
	err:= simple.SimpleA()
	if err!=nil{
		return fmt.Errorf("foo:%s",err)
	}
	return nil
}
```
尽管我们通过`fmt.Errorf`的方式为错误增加了一些上下文信息，但还是不利于我们去排查错误。我们需要知道更多的错误信息: 在什么文件，在哪一行用来更好的定位。


# 错误处理最佳实践(github.com/pkg/errors)

github.com/pkg/errors为我们封装了更加丰富的错误处理方式：保存错误堆栈信息、错误包装、格式化输出等功能。

## 几个重要结构和函数

* fundamental
基本错误包含错误消息和堆栈信息，但是没有调用者。

```go
// fundamental is an error that has a message and a stack, but no caller.
type fundamental struct {
	msg string
	*stack
}

func main() {
	err1:=errors.New("new error")
	err2:=errors.Errorf("new errorf")
	fmt.Printf("%+v\n\n",err1)  //可以格式化输出
	fmt.Printf("%+v\n\n",err2)

}
```

* withMessage

包装错误信息，但是不包含堆栈信息。
通过 ` WithMessage(err error, message string) error` 和 `WithMessagef(err error, format string, args ...interface{}) error`函数来包装好的错误信息。

```go
type withMessage struct {
	cause error
	msg   string
}

func (w *withMessage) Error() string { return w.msg + ": " + w.cause.Error() }
func (w *withMessage) Cause() error  { return w.cause }

// Unwrap provides compatibility for Go 1.13 error chains.
func (w *withMessage) Unwrap() error { return w.cause }

func (w *withMessage) Format(s fmt.State, verb rune) {
	switch verb {
	case 'v':
		if s.Flag('+') {
			fmt.Fprintf(s, "%+v\n", w.Cause())
			io.WriteString(s, w.msg)
			return
		}
		fallthrough
	case 's', 'q':
		io.WriteString(s, w.Error())
	}
}
```

```go
func main() {
	err := foo()
	fmt.Printf("main err:%s\n\n",err)
	fmt.Printf("main err:%v\n\n",err)
	fmt.Printf("main err:%+v\n\n",err)

}


func foo()error{
	_,err:=ioutil.ReadFile("./test.yaml")
	return errors.WithMessage(err,"read file err")
}
```

* withStack

含有堆栈信息的错误包装。使用`Wrap(err error, message string) error`或者`Wrapf(err error, format string, args ...interface{}) error`

```go
type withStack struct {
	error  // 这里的error其实是withMessage 堆栈信息是对withMessage的进一步封装
	*stack
}

func (w *withStack) Cause() error { return w.error }

// Unwrap provides compatibility for Go 1.13 error chains.
func (w *withStack) Unwrap() error { return w.error }

func (w *withStack) Format(s fmt.State, verb rune) {
	switch verb {
	case 'v':
		if s.Flag('+') {
			fmt.Fprintf(s, "%+v", w.Cause())
			w.stack.Format(s, verb)
			return
		}
		fallthrough
	case 's':
		io.WriteString(s, w.Error())
	case 'q':
		fmt.Fprintf(s, "%q", w.Error())
	}
}
```

* 获取错误根本原因

```go
func foo()error{
	_,err:=ioutil.ReadFile("./test.yaml")
	return errors.Wrap(err,"read file err")
}
fmt.Println("main:",errors.Cause(err)) // open ./test.yaml: no such file or directory
```

* 错误判断和解析 Is As Unwarp

从go 1.13以后,标准库采用了此包的错误处理方式。并添加了 Is As Unwap几个函数来更方便的处理错误。 此包为了兼容1.13,同时也增加了三个函数，内部都是调用的标准库函数。

```go

func Is(err, target error) bool { return stderrors.Is(err, target) }


func As(err error, target interface{}) bool { return stderrors.As(err, target) }

func Unwrap(err error) error {
	return stderrors.Unwrap(err)
}
```

由于错误采用包装的方式增加上下文信息并向上层传递。上层如果想要判断错误是否为某个具体错误,就不能直接用比较值的方式。Is方法用来将包装好的error剥开，并判断是否包装了目标错误。

```go
func main() {
	err :=bar()
	if err!=nil{
		if errors.Is(err,errFileNotFound){
			fmt.Printf("%+v\n",err)
		}
	}

}


func bar()error{
	return errors.WithMessage(foo(),"bar")
}

func foo()error{
	return errFileNotFound
}
```


# warp error的最佳实践

错误包装虽然可以方便我们处理错误,但也不能随意的进行包装，造成错误信息太过繁琐。这里对于错误的包装也有一些注意的地方

* 错误只处理一次,打印日志也算一种错误处理。不要造成日志冗余。

	```go
	func Bar()error{
		_,err:=ioutil.ReadFile("./test.yaml")
		if err!=nil{
			log.Println(err) //bad
			return err
		}
		return nil
	}
	```
* 在你的应用代码中,使用errors.new()或者errors.errorf返回错误

	```
	func parseArgs(args []string)error{
		if len(args) < 3{
			return errors.Errorf("not enough arguement,expected at least 3 args")
		}
		return nil
	}
	```
* 如果调用的是其他的函数，通常直接返回

	```go
	err:=bar()
	if err!=nil{
		return err
	}
	```
* 如果调用的第三方库或者标准库的。考虑使用Warp或者Warpf保存堆栈信息。

	```go
	func bar()error{
	path:="./a.txt"
	_,err:=os.Open(path)
	if err!=nil{
		return errors.Wrapf(err,"faild to open %q",path)
	}
	return nil
	}
	```

* 在程序的顶部或者是工作的 goroutine 顶部(请求入口)，使用 %+v 把堆栈详情记录。

	```go
	func main() {
		err := app.Run()
		if err!=nil{
			fmt.Printf("err:%+v\n",err)
		}
	}
	```
* 使用error.Cause获取根错误再进行sentinel errror判断。或者直接使用Is函数。
* **选择 wrap error 是只有 applications 可以选择应用的策略。具有最高可重用性的包只能返回根错误值。此机制与 Go 标准库中使用的相同(kit 库的 sql.ErrNoRows)。**  
	Packages that are reusable across many projects only return root error values.  
	如果你开发的是一个基础组件或者工具库。请返回确定的根错误值，不要返回包装错误。
* **一旦确定函数/方法将处理错误，错误就不再是错误。如果函数/方法仍然需要发出返回，则它不能返回错误值。它应该只返回零(比如降级处理中，你返回了降级数据，然后需要 return nil)。**  
	Once an error is handled, it is not allowed to be passed up the call stack any longer.

# go2错误处理


[https://go.googlesource.com/proposal/+/master/design/29934-error-values.md](https://go.googlesource.com/proposal/+/master/design/29934-error-values.md)


