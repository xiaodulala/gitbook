# 介绍

## 概览

Cadence要求工作流代码的决定论。它支持多线程代码的确定性执行，以及像select这样的由Go设计而非确定性的结构。Cadence解决方案以接口的形式提供相应的结构，这些接口具有类似的功能，但支持确定性执行。

例如，工作流代码必须使用工作流，而不是原生的Go通道。通道接口。而不是选择工作流。必须使用选择器接口。

有关更多信息，请参见创建工作流。

## 链接

* GitHub project:  [https://github.com/uber-go/cadence-client](https://github.com/uber-go/cadence-client)
* Samples:  [https://github.com/uber-common/cadence-samples](https://github.com/uber-common/cadence-samples)
* GoDoc documentation: [https://godoc.org/go.uber.org/cadence](https://godoc.org/go.uber.org/cadence) 

# Worker service

工作者或工作者服务是托管工作流和活动实现的服务。worker向Cadence服务轮询任务，执行任务，并将任务执行结果反馈给Cadence服务。工人服务由Cadence客户开发、部署和运营。

您可以在新的或现有的服务中运行一个Cadence worker。使用框架api来启动Cadence worker，并链接所有需要服务执行的活动和工作流实现。

```go
package main

import (
	"github.com/uber-go/tally"
	"go.uber.org/cadence/.gen/go/cadence/workflowserviceclient"
	"go.uber.org/cadence/worker"
	"go.uber.org/yarpc"
	"go.uber.org/yarpc/transport/tchannel"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
	"strings"
)

var HostPort = "10.0.0.35:7933"
var Domain = "SimpleDomain"
var TaskListName = "SimpleWorker"
var ClientName = "SimpleWorker"
var CadenceService = "cadence-frontend"

func main(){
	startWorker(buildLogger(),buildCadenceClient())
	select {

	}
}


func buildLogger()*zap.Logger{
	config:=zap.NewDevelopmentConfig()
	config.Level.SetLevel(zapcore.InfoLevel)

	var err error

	logger,err:= config.Build()
	if err!=nil{
		panic("Failed to setup logger")
	}

	return logger
}

func buildCadenceClient()workflowserviceclient.Interface{
	ch,err:=tchannel.NewChannelTransport(tchannel.ServiceName(ClientName))
	if err != nil {
		panic("Failed to setup tchannel")
	}
	// Dispatcher封装了一个YARPC应用程序。它充当入口点，以传输和编码无关的方式发送和接收YARPC请求。
	// Outbounds 为远程服务提供访问。定义请求如何从此服务发送到远程服务
	dispatcher :=yarpc.NewDispatcher(yarpc.Config{
		Name:                               strings.ToLower(ClientName),
		Outbounds:                          yarpc.Outbounds{
			CadenceService: {Unary: ch.NewSingleOutbound(HostPort)},
		},
	})

	if err := dispatcher.Start(); err != nil {
		panic("Failed to start dispatcher")
	}

	return workflowserviceclient.New(dispatcher.ClientConfig(CadenceService))
}

func startWorker(logger *zap.Logger,service workflowserviceclient.Interface){
	// TaskListName标识一组客户端工作流、活动和工作者。
	// 它可以是您的组名、客户机名或应用程序名。

	workerOptions :=worker.Options{
		Logger: logger,
		MetricsScope: tally.NewTestScope(TaskListName, map[string]string{}),
	}

	worker1 := worker.New(
		service,
		Domain,
		TaskListName,
		workerOptions,
		)
	err :=worker1.Start()
	if err !=nil{
		panic("Failed to start worker")
	}

	logger.Info("Started Worker.", zap.String("worker", TaskListName))
}
```

# Creating workflows

工作流是协调逻辑的实现。Cadence编程框架(又名客户端库)允许您将工作流协调逻辑编写为使用标准Go数据建模的简单过程代码。客户端库负责worker服务和Cadence服务之间的通信，并确保事件之间的状态持久性，即使在worker失败的情况下。此外，任何特定的执行都不绑定到特定的工作机器。协调逻辑的不同步骤最终可能在不同的工作人员实例上执行，框架确保在执行该步骤的工作人员上重新创建必要的状态。

然而，为了方便这种操作模型，Cadence编程框架和托管服务都对协调逻辑的实现施加了一些要求和限制。下面的实现部分将详细描述这些需求和限制。

##  概述

下面的示例代码显示了执行一个活动的工作流的简单实现。工作流还将它在初始化过程中接收到的唯一参数作为活动的参数传递。



## 声名

在Cadence编程模型中，用一个函数实现了工作流。函数声明指定了工作流接受的参数以及它可能返回的任何值。

```go
func SimpleWorkflow(ctx workflow.Context, value string) error
```

* 函数的第一个参数是ctx workflow.Context。这是所有工作流函数所需的参数，Cadence客户端库使用它来传递执行上下文。实际上，所有可从工作流函数调用的客户端库函数都需要这个ctx参数。这个context参数与标准context是同一个概念。Go提供的上下文。工作流之间的唯一区别。上下文和语境。上下文是工作流中的Done()函数。上下文返回工作流。频道改为标准的go chan。
* 第二个参数string是一个自定义工作流参数，可用于在开始时将数据传递到工作流中。工作流可以有一个或多个这样的参数。工作流函数的所有参数必须是可序列化的，这本质上意味着参数不能是通道、函数、可变参数或不安全的指针。
* 因为它只声明error作为返回值，这意味着工作流不返回值。错误返回值用于指示在执行过程中遇到了错误，应该终止工作流。


## 执行

为了支持工作流实现的同步和顺序编程模型，对于工作流实现必须如何行为以保证正确性有一定的限制和要求。要求是

* 执行必须是确定性的
* 执行必须是幂等的

考虑这些需求的一个简单方法是工作流代码如下:

* 工作流代码只能读取和操作本地状态或从Cadence客户端库函数返回值接收的状态
* 工作流代码不应该影响外部系统中的更改，除非通过调用活动。
* 工作流代码应该只通过Cadence客户端库提供的函数与时间交互(即Workflow . now ()， Workflow . sleep())。
* 工作流代码不应该直接创建和与goroutines交互，它应该使用Cadence客户端库提供的函数 (workflow.Go() instead of go, workflow.Channel instead of chan, workflow.Selector instead of select).
* 工作流代码应该通过Cadence客户端库(即Workflow . getlogger())提供的记录器进行所有日志记录。
* 工作流代码不应该在使用range的地图上迭代，因为地图迭代的顺序是随机的。


 现在我们已经奠定了基本规则，我们可以看一下用于编写Cadence工作流的一些特殊函数和类型，以及如何实现一些常见模式。
 
 特殊Cadence客户端库函数和类型：
 
  Cadence客户端库提供了许多函数和类型作为一些原生Go函数和类型的替代。使用这些替换函数/类型是必要的，以确保工作流代码执行在执行上下文中是确定的和可重复的。
  
  协程相关的构造:
  
  * workflow.Go :  这是go语句的替换。
  * workflow.Channel :  这是对原生chan类型的替换。Cadence同时支持缓冲和非缓冲通道
  * workflow.Selector:   这是对select语句的替换。

  时间相关函数:
  
  * workflow.Now()： time.now的替代品。
  * workflow.Sleep() time.sleep()的替代品。

## 失败
  
  要将工作流标记为失败，只需要工作流函数通过err返回值返回一个错误即可
  
## 注册

对于一些能够调用工作流类型的客户机代码，工作进程需要知道它能够访问的所有实现。工作流通过以下调用注册:

```go
workflow.Register(SimpleWorkflow)
```
`这个调用实际上在工作进程内部创建了一个完全限定函数名和实现之间的内存映射。从init()函数调用这个注册方法是安全的。如果工作者接收到它不知道的工作流类型的任务，它将使该任务失败。但是，任务失败不会导致整个工作流失败。


# 活动概览

活动是业务逻辑中特定任务的实现。

活动被实现为函数。数据可以通过函数参数直接传递给活动。参数可以是基本类型或结构，唯一的要求是参数必须是可序列化的。虽然不是必需的，但是我们建议活动函数的第一个参数是context类型的。上下文，以允许活动与其他框架方法交互。函数必须返回一个错误值，也可以有选择地返回结果值。结果值可以是基本类型，也可以是结构体，唯一的要求是它是可序列化的。

通过调用参数传递给活动或通过结果值返回的值记录在执行历史中。在工作流逻辑需要处理的每个事件中，整个执行历史都从Cadence服务转移到工作流工作者。因此，较大的执行历史可能会对工作流的性能产生负面影响。因此，要注意通过活动调用参数或返回值传输的数据量。否则，活动实现上不存在额外的限制。


## 概览

下面的示例演示了一个简单的活动，该活动接受一个字符串参数，向其添加一个单词，然后返回结果。

```go
package simple

import (
    "context"

    "go.uber.org/cadence/activity"
    "go.uber.org/zap"
)

func init() {
    activity.Register(SimpleActivity)
}

// SimpleActivity is a sample Cadence activity function that takes one parameter and
// returns a string containing the parameter value.
func SimpleActivity(ctx context.Context, value string) (string, error) {
    activity.GetLogger(ctx).Info("SimpleActivity called.", zap.String("Value", value))
    return "Processed: " + value, nil
}
```

让我们看看这个活动的每个组件。


在Cadence编程模型中，活动是用函数实现的。函数声明指定了活动接受的参数以及它可能返回的值。一个活动函数可以采取零个或多个活动特定参数，并可以返回一个或两个值。它必须至少返回一个错误值。活动函数可以接受任何可序列化类型作为参数并返回结果。

```go
func SimpleActivity(ctx context.Context, value string) (string, error)
```

 函数的第一个参数是context.Context。这是一个可选参数，可以省略。该参数是标准的Go上下文。第二个字符串参数是特定于活动的自定义参数，可用于在开始时将数据传递到活动中。一个活动可以有一个或多个这样的参数。活动函数的所有参数必须是可序列化的，这本质上意味着参数不能是通道、函数、可变参数或不安全的指针。该活动声明了两个返回值:string和error。的返回值用于返回结果.
 
 
  您可以按照编写任何其他Go服务代码的相同方式编写活动实现代码。此外，您可以使用常用的记录器和度量控制器，以及标准的Go并发结构
  
## 心跳

对于长时间运行的活动，Cadence为活动代码提供了一个API，向Cadence管理的服务报告活动和进度。

```go
progress := 0
for hasWork {
    // Send heartbeat message to the server.
    cadence.RecordActivityHeartbeat(ctx, progress)
    // Do some work.
    ...
    progress++
}
```

When an activity times out due to a missed heartbeat, the last value of the details (progress in the above sample) is returned from the cadence.ExecuteActivity function as the details field of TimeoutError with TimeoutType_HEARTBEAT.

Cadence Go Client 0.17.0版本中新的自动心跳选项(打开新窗口):如果你不需要报告进度，但仍然想通过长时间运行的活动的心跳报告工作人员的活跃度，有一个新的自动心跳选项，你可以在注册活动时启用。当启用此选项时，Cadence库将在后台为你做心跳。

```go
RegisterActivityOptions struct {
		...
		// Automatically send heartbeats for this activity at an interval that is less than the HeartbeatTimeout.
		// This option has no effect if the activity is executed with a HeartbeatTimeout of 0.
		// Default: false
		EnableAutoHeartbeat bool
	}
```

你也可以从外部源检测一个活动:

```go
// Instantiate a Cadence service client.
cadence.Client client = cadence.NewClient(...)

// Record heartbeat.
err := client.RecordActivityHeartbeat(taskToken, details)
```

* taskToken:在活动内部检索到的ActivityInfo结构体的二进制taskToken字段的值。
* details:包含进度信息的可序列化负载

 当一个活动被取消，或者它的工作流执行完成或失败时，传递给它的函数的上下文被取消，这将其通道的关闭状态设置为Done。活动可以使用它来执行任何必要的清理和中止执行。取消只交付给调用RecordActivityHeartbeat的活动。
 
## 注册
 
  为了使该活动对托管它的工作进程可见，该活动必须通过调用activity. register进行注册。
  
 ```go
 func init() {
    activity.Register(SimpleActivity)
}
 ```
 
 这个调用在工作进程内部的完全限定函数名和实现之间创建了一个内存映射。如果一个worker接收到一个请求来启动一个它不知道的活动类型的活动执行，它将使该请求失败。 
 
## 错误活动

要将一个活动标记为失败，活动函数必须通过错误返回值返回一个错误。