
# 概述

Go语言虽然简单易学，但是想要变成真正的编程个高手，写出易阅读，效率高，共享性和扩展性都比较优秀的高质量代码却是一件不简单的事情。  

如何写出优雅且高质量的Go代码呢？

1. 遵循Go编码规范,这是最容易实现的途径。可以从Go社区中获取。比较受欢迎的是[Uber Go 语言编码规范](https://github.com/xxjwxc/uber_go_guide_cn)
2. 学习并遵循前辈们探索沉淀下来的一些最佳实践。如
	* [Effective Go](https://golang.org/doc/effective_go)：高效 Go 编程，由 Golang 官方编写，里面包含了编写 Go 代码的一些建议，也可以理解为最佳实践。

	* [Go Code Review Comments](https://github.com/golang/go/wiki/CodeReviewComments)：Golang 官方编写的 Go 最佳实践，作为 Effective Go 的补充。

本文是对这些规范和实践的一些总结。


# 编码规范

## 错误处理

* error作为函数的返回值,必须对error进行处理。或将返回值赋值给明确忽略。对于`defer   xx.Close()` 可以不用显示处理。  

	```go
	func load() error {
	  // normal code
	}
	
	// bad
	load()
	
	// good
	 _ = load()
	```
* 如果一个函数返回了 `value,error`。你不能对这个value做任何假设，必须先判断error。唯一可以忽略error的是你连value都不关心。
* error作为函数的值返回且有多个返回值的时候，error必须是最后一个参数。
*  尽早进行错误处理，并尽早返回，减少嵌套。
	
	```go
	//bad
	if err!=nil{
		//error code
	}else{
		//normal code
	}
	
	//good
	if err!=nil{
		return err
	}
	// normal code
	```
* 错误描述建议
	* 告诉用户他们可以做什么，而不是告诉他们不能做什么。
	* 当声明一个需求时，用 must 而不是 should。例如，must be greater than 0、must match regex ‘[a-z]+’。
	* 当声明一个格式不对时，用 must not。例如，must not contain。
	* 当声明一个动作时用 may not。例如，may not be specified when otherField is empty、only name may be specified。
	* 引用文字字符串值时，请在单引号中指示文字。例如，ust not contain ‘…’。
	* 当引用另一个字段名称时，请在反引号中指定该名称。例如，must be greater than request。
	* 指定不等时，请使用单词而不是符号。例如，must be less than 256、must be greater than or equal to 0 (不要用 larger than、bigger than、more than、higher than)。
	* 指定数字范围时，请尽可能使用包含范围。
	* 建议 Go 1.13 以上，error 生成方式为 fmt.Errorf("module xxx: %w", err)。
	* 错误描述用小写字母开头，结尾不要加标点符号，例如：
	
	```go
	  // bad
	  errors.New("Redis connection failed")
	  errors.New("redis connection failed.")
	
	  // good
	  errors.New("redis connection failed")
	```

# 最佳实践

##  错误处理

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




# 参考引用

1. [Rob Pike 所有关于 Go 的谚语](https://go-proverbs.github.io/)
2. [Uber Go 语言编码规范](https://github.com/xxjwxc/uber_go_guide_cn)
3. 孔令飞老师的极客时间的 [Go语言项目实战](https://time.geekbang.org/column/intro/100079601?tab=catalog)
4. 毛剑老师极客时间 [Go进阶训练营](https://u.geekbang.org/subject/go?utm_source=time_web&utm_medium=menu&utm_term=timewebmenu&utm_identify=geektime&utm_content=menu&utm_campaign=timewebmenu&gk_cus_user_wechat=university)