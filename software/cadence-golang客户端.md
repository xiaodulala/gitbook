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


# 执行活动

 工作流实现的主要职责是为执行安排活动。最直接的方法是通过库方法workflow.ExecuteActivity。下面的示例代码演示了如何进行这个调用:
 
 ```yaml
 ao := cadence.ActivityOptions{
    TaskList:               "sampleTaskList",
    ScheduleToCloseTimeout: time.Second * 60,
    ScheduleToStartTimeout: time.Second * 60,
    StartToCloseTimeout:    time.Second * 60,
    HeartbeatTimeout:       time.Second * 10,
    WaitForCancellation:    false,
}
ctx = cadence.WithActivityOptions(ctx, ao)

future := workflow.ExecuteActivity(ctx, SimpleActivity, value)
var result string
if err := future.Get(ctx, &result); err != nil {
    return err
}
 ```
 
 让我们看看这个调用的每个组件。
 
## Activity options
 
  在调用workflow.ExecuteActivity()之前，您必须为调用配置ActivityOptions。这些选项自定义各种执行超时，并通过从初始上下文创建子上下文并覆盖所需的值来传入。然后子上下文被传递到workflow.ExecuteActivity()调用。如果多个活动共享相同的选项值，那么在调用workflow.ExecuteActivity()时可以使用相同的上下文实例。 
  
## 活动超时

可以有各种与活动相关联的超时。Cadence保证活动最多执行一次，所以一个活动成功或失败的超时时间如下:

* StartToCloseTimeout   工作者接收任务后处理任务的最大时间
* ScheduleToStartTimeout  在工作流调度任务之后，任务等待被活动工作者拾取的时间。如果在指定的时间内没有可用的worker来处理此任务，则该任务将超时。
* ScheduleToCloseTimeout  在工作流安排任务之后，任务完成所需的时间。这通常大于StartToClose和ScheduleToStart超时的总和。
* HeartbeatTimeout  如果任务在此期间没有心跳到Cadence服务，则将认为该任务失败。这对于长时间运行的任务很有用

## ExecuteActivity call 

调用中的第一个参数是所需的 cadence上下文对象。这种类型是上下文的副本。使用Done()方法返回cadence.Channel。频道而不是原生的Go chan。

第二个参数是我们注册为活动函数的函数。此参数也可以是表示活动函数的完全限定名称的字符串。传入实际函数对象的好处是，框架可以验证活动参数。

其余参数作为调用的一部分传递给活动。在我们的示例中，只有一个参数:value。此参数列表必须与活动函数声明的参数列表相匹配。Cadence客户端库将对此进行验证。

该方法调用立即返回并返回一个cadence.Future。这允许您执行更多的代码，而不必等待计划的活动完成。

当您准备处理活动的结果时，对返回的未来对象调用Get()方法。这个方法的参数是我们传递给workflow.ExecuteActivity()调用的ctx对象和一个将接收活动输出的输出参数。输出参数的类型必须与活动函数声明的返回值的类型匹配。Get()方法将阻塞，直到活动完成和结果可用。

您可以从未来检索workflow.ExecuteActivity()返回的结果值，并像使用同步函数调用的任何正常结果一样使用它。下面的示例代码演示了如果结果是字符串值，如何使用该结果:


```go
var result string
if err := future.Get(ctx1, &result); err != nil {
    return err
}

switch result {
case "apple":
    // Do something.
case "banana":
    // Do something.
default:
    return err
}
```

在这个例子中，我们在workflow.ExecuteActivity()之后立即对返回的future调用Get()方法。然而，这是不必要的。如果您希望并行执行多个活动，您可以重复调用workflow.ExecuteActivity()，存储返回的future，然后在稍后的时间通过调用future的Get()方法等待所有活动完成。

要在返回的future对象上实现更复杂的等待条件，可以使用cadence.Selector类。

# 子工作流

允许从工作流的实现中调度其他工作流。父工作流能够监视和影响子工作流的生命周期，就像它对调用的活动所做的那样。

```go
cwo := workflow.ChildWorkflowOptions{
    // Do not specify WorkflowID if you want Cadence to generate a unique ID for the child execution.
    WorkflowID:                   "BID-SIMPLE-CHILD-WORKFLOW",
    ExecutionStartToCloseTimeout: time.Minute * 30,
}
ctx = workflow.WithChildWorkflowOptions(ctx, cwo)

var result string
future := workflow.ExecuteChildWorkflow(ctx, SimpleChildWorkflow, value)
if err := future.Get(ctx, &result); err != nil {
    workflow.GetLogger(ctx).Error("SimpleChildWorkflow failed.", zap.Error(err))
    return err
}
```
在调用workflow.ExecuteChildworkflow()之前，必须为调用配置ChildWorkflowOptions。这些选项自定义各种执行超时，并通过从初始上下文创建子上下文并覆盖所需的值来传入。然后子上下文被传递到workflow.ExecuteChildWorkflow()调用中。如果多个子工作流共享相同的选项值，那么调用workflow.ExecuteChildworkflow()时可以使用相同的上下文实例。


调用中的第一个参数是所需的cadence.Context对象。这种类型是上下文的副本。使用Done()方法返回节奏。频道而不是原生的Go chan。


第二个参数是我们注册为工作流函数的函数。该参数也可以是表示工作流函数的完全限定名称的字符串。这样做的好处是，当您传入实际的函数对象时，框架可以验证工作流参数。  其余参数作为调用的一部分传递给工作流。在我们的示例中，只有一个参数:value。此参数列表必须与工作流函数声明的参数列表相匹配。

该方法调用立即返回并返回一个cadence.Future。这允许您执行更多的代码，而不必等待计划的工作流完成。


当您准备处理工作流的结果时，对返回的future对象调用Get()方法。这个方法的参数是我们传递给workflow. executechildworkflow()调用的ctx对象和一个将接收工作流输出的输出参数。输出参数的类型必须与工作流函数声明的返回值的类型匹配。Get()方法将阻塞，直到工作流完成和结果可用。

workflow.ExecuteChildWorkflow()函数类似于workflow.ExecuteActivity()。使用workflow.ExecuteActivity()所描述的所有模式也适用于workflow.ExecuteChildWorkflow()函数

当用户取消父工作流时，子工作流可以根据可配置的子策略取消或放弃。

# Activity and workflow retries

活动和工作流可能会由于各种中间条件而失败。在这种情况下，我们希望重试失败的活动或子工作流，甚至父工作流。这可以通过提供一个可选的重试策略来实现。重试策略如下所示

```go
// RetryPolicy defines the retry policy.
RetryPolicy struct {
    // Backoff interval for the first retry. If coefficient is 1.0 then it is used for all retries.
    // Required, no default value.
    InitialInterval time.Duration

    // Coefficient used to calculate the next retry backoff interval.
    // The next retry interval is previous interval multiplied by this coefficient.
    // Must be 1 or larger. Default is 2.0.
    BackoffCoefficient float64

    // Maximum backoff interval between retries. Exponential backoff leads to interval increase.
    // This value is the cap of the interval. Default is 100x of initial interval.
    MaximumInterval time.Duration

    // Maximum time to retry. Either ExpirationInterval or MaximumAttempts is required.
    // When exceeded the retries stop even if maximum retries is not reached yet.
    // First (non-retry) attempt is unaffected by this field and is guaranteed to run 
    // for the entirety of the workflow timeout duration (ExecutionStartToCloseTimeoutSeconds).
    ExpirationInterval time.Duration

    // Maximum number of attempts. When exceeded the retries stop even if not expired yet.
    // If not set or set to 0, it means unlimited, and relies on ExpirationInterval to stop.
    // Either MaximumAttempts or ExpirationInterval is required.
    MaximumAttempts int32

    // Non-Retriable errors. This is optional. Cadence server will stop retry if error reason matches this list.
    // Error reason for custom error is specified when your activity/workflow returns cadence.NewCustomError(reason).
    // Error reason for panic error is "cadenceInternal:Panic".
    // Error reason for any other error is "cadenceInternal:Generic".
    // Error reason for timeouts is: "cadenceInternal:Timeout TIMEOUT_TYPE". TIMEOUT_TYPE could be START_TO_CLOSE or HEARTBEAT.
    // Note that cancellation is not a failure, so it won't be retried.
    NonRetriableErrorReasons []string
}
```

要启用重试，在执行ActivityOptions或ChildWorkflowOptions时提供一个自定义的重试策略。

```go
expiration := time.Minute * 10
retryPolicy := &cadence.RetryPolicy{
    InitialInterval:    time.Second,
    BackoffCoefficient: 2,
    MaximumInterval:    expiration,
    ExpirationInterval: time.Minute * 10,
    MaximumAttempts:    5,
}
ao := workflow.ActivityOptions{
    ScheduleToStartTimeout: expiration,
    StartToCloseTimeout:    expiration,
    HeartbeatTimeout:       time.Second * 30,
    RetryPolicy:            retryPolicy, // Enable retry.
}
ctx = workflow.WithActivityOptions(ctx, ao)
activityFuture := workflow.ExecuteActivity(ctx, SampleActivity, params)
```

如果活动在失败前检测它的进度，重试尝试将包含该进度，因此活动实现可以从失败的进度中恢复，如下所示:

```go
func SampleActivity(ctx context.Context, inputArg InputParams) error {
    startIdx := inputArg.StartIndex
    if activity.HasHeartbeatDetails(ctx) {
        // Recover from finished progress.
        var finishedIndex int
        if err := activity.GetHeartbeatDetails(ctx, &finishedIndex); err == nil {
            startIdx = finishedIndex + 1 // Start from next one.
        }
    }

    // Normal activity logic...
    for i:=startIdx; i<inputArg.EndIdx; i++ {
        // Code for processing item i goes here...
        activity.RecordHeartbeat(ctx, i) // Report progress.
    }
}
```
与活动的重试一样，您需要为ChildWorkflowOptions提供一个重试策略，以启用子工作流的重试。要为父工作流启用重试，请在通过StartWorkflowOptions启动工作流时提供重试策略。

 当使用RetryPolicy时，工作流的历史事件会有一些微妙的变化。对于带有RetryPolicy的活动:
 
 * ActivityTaskScheduledEvent将扩展ScheduleToStartTimeout和ScheduleToCloseTimeout。这两个超时将被服务器覆盖，其长度与重试策略的ExpirationInterval相同。如果没有指定ExpirationInterval，它将被覆盖到工作流的超时时间。
* ActivityTaskStartedEvent将不会显示在历史中，直到活动完成或失败，没有更多的重试。这是为了避免记录ActivityTaskStarted事件，但后来它失败并重试。使用DescribeWorkflowExecution API将返回PendingActivityInfo，如果它正在重试，它将包含attemptCount。

对于带有RetryPolicy的工作流:

* 如果工作流失败并需要重试，则工作流执行将使用ContinueAsNew事件关闭。该事件将ContinueAsNewInitiator设置为RetryPolicy，并为下一次重试尝试设置新的RunID。
* 新的尝试将立即创建。但是第一个决策任务将不会被调度，直到后退持续时间也被记录在新运行的WorkflowExecutionStartedEventAttributes事件中，作为firstDecisionTaskBackoffSeconds。

# 错误处理

活动或子工作流可能会失败，您可以根据不同的错误情况以不同的方式处理错误。如果活动返回error . new()或fmt.Errorf()，这些错误将被转换为workflow.GenericError。如果活动返回一个错误为cadence。NewCustomError(" err-reason "， details)，该错误将被转换为*cadence.CustomError。还有其他类型的错误，比如工作流错误。TimeoutError,工作流。CanceledError workflow.PanicError。下面是一个错误代码的示例:

```go
err := workflow.ExecuteActivity(ctx, YourActivityFunc).Get(ctx, nil)
switch err := err.(type) {
    case *cadence.CustomError:
        switch err.Reason() {
            case "err-reason-a":
                // Handle error-reason-a.
                var details YourErrorDetailsType
                err.Details(&details)
                // Deal with details.
            case "err-reason-b":
                // Handle error-reason-b.
            default:
                // Handle all other error reasons.
        }
    case *workflow.GenericError:
        switch err.Error() {
            case "err-msg-1":
                // Handle error with message "err-msg-1".
            case "err-msg-2":
                // Handle error with message "err-msg-2".
            default:
                // Handle all other generic errors.
        }
    case *workflow.TimeoutError:
        switch err.TimeoutType() {
            case shared.TimeoutTypeScheduleToStart:
                // Handle ScheduleToStart timeout.
            case shared.TimeoutTypeStartToClose:
                // Handle StartToClose timeout.
            case shared.TimeoutTypeHeartbeat:
                // Handle heartbeat timeout.
            default:
        }
    case *workflow.PanicError:
        // Handle panic error.
    case *cadence.CanceledError:
        // Handle canceled error.
    default:
        // All other cases (ideally, this should not happen).
}
```

# 信号

信号提供了一种直接向运行中的工作流发送数据的机制。在此之前，您有两个向工作流实现传递数据的选项:

* 通过启动参数
* 作为活动的返回值

 使用开始参数，我们只能在工作流执行开始之前传入值。
 
 活动的返回值允许我们将信息传递给正在运行的工作流，但是这种方法有其自身的复杂性。一个主要缺点是对投票的依赖。这意味着数据需要存储在第三方位置，直到它可以被活动拾取为止。此外，该活动的生命周期需要管理，如果在获取数据之前失败，则需要手动重新启动该活动。
 
 另一方面，信号提供了一种完全异步和持久的机制，用于向运行中的工作流提供数据。当接收到运行中的工作流的信号时，Cadence将事件和负载保存在工作流历史记录中。工作流可以在之后的任何时间处理信号，而不会有丢失信息的风险。工作流还可以通过阻塞信号通道来停止执行。
 
 
```go
 var signalVal string
signalChan := workflow.GetSignalChannel(ctx, signalName)

s := workflow.NewSelector(ctx)
s.AddReceive(signalChan, func(c workflow.Channel, more bool) {
    c.Receive(ctx, &signalVal)
    workflow.GetLogger(ctx).Info("Received signal!", zap.String("signal", signalName), zap.String("value", signalVal))
})
s.Select(ctx)

if len(signalVal) > 0 && signalVal != "SOME_VALUE" {
    return errors.New("signalVal")
}

```
 

In the example above, the workflow code uses workflow.GetSignalChannel to open a workflow.Channel for the named signal. We then use a workflow.Selector to wait on this channel and process the payload received with the signal.


您可能不知道工作流是否正在运行并可以接受信号。客户端。SignalWithStartWorkflow(打开新窗口)API允许你向当前工作流实例发送信号，如果当前工作流实例存在，或者创建一个新的运行，然后发送信号。因此SignalWithStartWorkflow不接受运行ID作为参数。

# Continue as new

需要定期重新运行的工作流可以天真地实现为一个大的for循环，其中工作流的整个逻辑都在for循环的主体内。这种方法的问题是，该工作流的历史记录将不断增长，达到服务强制的最大大小。

ContinueAsNew是一个低层次的构造，它能够实现这样的工作流，而不会有失败的风险。该操作以原子的方式完成当前的执行，并以相同的工作流ID开始新的工作流执行。新的执行不会从旧的执行中继承任何历史记录。为了触发这种行为，工作流函数应该通过返回特殊的ContinueAsNewError错误来终止:


```go
func SimpleWorkflow(workflow.Context ctx, value string) error {
    ...
    return workflow.NewContinueAsNewError(ctx, SimpleWorkflow, value)
}
```

# side effect

工作流。SideEffect对于简短的、不确定的代码片段非常有用，例如获取随机值或生成UUID。它执行一次所提供的功能，并将其结果记录到工作流历史中。工作流。在重播时，SideEffect不会重新执行，而是返回记录的结果。它可以被视为“内联”活动。关于工作流需要注意的一点。副作用是，不像Cadence保证活动最多执行一次，workflow.SideEffect没有这样的保证。在一定的故障条件下，工作流程。SideEffect最终可能会执行

使副作用失效的唯一方法是恐慌，这会导致决策任务失败。在超时后，Cadence重新安排并重新执行决策任务，给了SideEffect另一个成功的机会。不要从SideEffect返回任何数据，除了通过它记录的返回值。

```go
encodedRandom := SideEffect(func(ctx cadence.Context) interface{} {
    return rand.Intn(100)
})

var random int
encodedRandom.Get(&random)
if random < 50 {
    ...
} else {
    ...
}
```

# 查询

如果工作流执行停留在某个状态的时间超过预期时间，您可能需要查询当前调用堆栈。可以通过Cadence CLI执行查询。例如:

`cadence-cli --domain samples-domain workflow query -w my_workflow_id -r my_run_id -qt __stack_trace`

该命令使用__stack_trace，它是Cadence客户端库支持的内置查询类型。您可以添加自定义查询类型来处理查询，例如查询工作流的当前状态，或者查询工作流已经完成了多少活动。为此，您需要使用workflow.SetQueryHandler设置一个查询处理程序。

处理函数必须是一个返回两个值的函数:

* A serializable result
* An error

处理程序函数可以接收任意数量的输入参数，但所有输入参数必须是可序列化的。下面的示例代码设置了一个查询处理程序，用于处理查询类型current_state:

```go
func MyWorkflow(ctx workflow.Context, input string) error {
    currentState := "started" // This could be any serializable struct.
    err := workflow.SetQueryHandler(ctx, "current_state", func() (string, error) {
        return currentState, nil
    })
    if err != nil {
        currentState = "failed to register query handler"
        return err
    }
    // Your normal workflow code begins here, and you update the currentState as the code makes progress.
    currentState = "waiting timer"
    err = NewTimer(ctx, time.Hour).Get(ctx, nil)
    if err != nil {
        currentState = "timer failed"
        return err
    }

    currentState = "waiting activity"
    ctx = WithActivityOptions(ctx, myActivityOptions)
    err = ExecuteActivity(ctx, MyActivity, "my_input").Get(ctx, nil)
    if err != nil {
        currentState = "activity failed"
        return err
    }
    currentState = "done"
    return nil
}
```

You can now query current_state by using the CLI

`cadence-cli --domain samples-domain workflow query -w my_workflow_id -r my_run_id -qt current_state`

 您还可以使用在Cadence客户端对象上的QueryWorkflow() API从代码中发出查询。
 
 ## 一致性查询
 
 查询有两个一致性级别，最终一致性和强一致性。考虑一下，如果您要对工作流发出信号，然后立即查询该工作流
 
 `cadence-cli --domain samples-domain workflow signal -w my_workflow_id -r my_run_id -n signal_name -if ./input.json`
 
 `cadence-cli --domain samples-domain workflow query -w my_workflow_id -r my_run_id -qt current_state`
 
  在本例中，如果信号要更改工作流状态，查询可能会也可能不会看到查询结果中反映的状态更新,这就是查询最终保持一致的含义。
  
  查询有另一个一致性级别，称为强一致性。强一致性的查询保证基于工作流状态，其中包括发出查询之前的所有事件。如果创建外部事件的调用在发出查询之前成功返回，则认为事件出现在查询之前。查询未完成时创建的外部事件可能反映在查询结果所基于的工作流状态中，也可能不反映。

为了在CLI中运行一致性查询，请执行以下操作:

`cadence-cli --domain samples-domain workflow query -w my_workflow_id -r my_run_id -qt current_state --qcl strong`

为了使用go客户端运行查询，请执行以下操作:

```go
resp, err := cadenceClient.QueryWorkflowWithOptions(ctx, &client.QueryWorkflowWithOptionsRequest{
    WorkflowID:            workflowID,
    RunID:                 runID,
    QueryType:             queryType,
    QueryConsistencyLevel: shared.QueryConsistencyLevelStrong.Ptr(),
})
```

当使用强一致的查询时，您应该期望比最终一致的查询更高的延迟。


# 异步活动完成

在某些情况下，在完成一个活动的功能后再完成它是不可能的或不可取的。例如，您可能有一个需要用户输入才能完成活动的应用程序。您可以使用轮询机制来实现活动，但更简单且更少资源密集型的实现是异步完成一个Cadence活动。

实现异步完成的活动有两个部分:

* 活动从外部系统提供完成所需的信息，并通知Cadence服务它正在等待外部回调。
* 外部服务调用Cadence服务来完成活动。

下面的例子演示了第一部分:


```go
// Retrieve the activity information needed to asynchronously complete the activity.
activityInfo := cadence.GetActivityInfo(ctx)
taskToken := activityInfo.TaskToken

// Send the taskToken to the external service that will complete the activity.
...

// Return from the activity a function indicating that Cadence should wait for an async completion
// message.
return "", activity.ErrResultPending
```

下面的代码演示了如何成功完成该活动:

```go
// Instantiate a Cadence service client.
// The same client can be used to complete or fail any number of activities.
cadence.Client client = cadence.NewClient(...)

// Complete the activity.
client.CompleteActivity(taskToken, result, nil)
```

要让活动失败，你需要做以下操作

```go
// Fail the activity.
client.CompleteActivity(taskToken, nil, err)
```

下面是CompleteActivity函数的参数:

* taskToken:在活动内部检索到的ActivityInfo结构体的二进制taskToken字段的值。
* result:活动记录的返回值。这个值的类型必须与活动函数声明的返回值的类型匹配。
* err:如果活动因错误而终止，返回的错误代码。

如果error不为空，则忽略result字段的值.

# 测试

Cadence Go客户端库提供了一个测试框架来促进测试工作流的实现。该框架适合于实现工作流逻辑的单元测试和功能测试。

下面的代码实现了SimpleWorkflow示例的单元测试:

```go
package sample

import (
    "errors"
    "testing"

    "github.com/stretchr/testify/mock"
    "github.com/stretchr/testify/suite"

    "go.uber.org/cadence"
    "go.uber.org/cadence/testsuite"
)

type UnitTestSuite struct {
    suite.Suite
    testsuite.WorkflowTestSuite

    env *testsuite.TestWorkflowEnvironment
}

func (s *UnitTestSuite) SetupTest() {
    s.env = s.NewTestWorkflowEnvironment()
}

func (s *UnitTestSuite) AfterTest(suiteName, testName string) {
    s.env.AssertExpectations(s.T())
}

func (s *UnitTestSuite) Test_SimpleWorkflow_Success() {
    s.env.ExecuteWorkflow(SimpleWorkflow, "test_success")

    s.True(s.env.IsWorkflowCompleted())
    s.NoError(s.env.GetWorkflowError())
}

func (s *UnitTestSuite) Test_SimpleWorkflow_ActivityParamCorrect() {
    s.env.OnActivity(SimpleActivity, mock.Anything, mock.Anything).Return(
        func(ctx context.Context, value string) (string, error) {
            s.Equal("test_success", value)
            return value, nil
        }
    )
    s.env.ExecuteWorkflow(SimpleWorkflow, "test_success")

    s.True(s.env.IsWorkflowCompleted())
    s.NoError(s.env.GetWorkflowError())
}

func (s *UnitTestSuite) Test_SimpleWorkflow_ActivityFails() {
    s.env.OnActivity(SimpleActivity, mock.Anything, mock.Anything).Return(
        "", errors.New("SimpleActivityFailure"))
    s.env.ExecuteWorkflow(SimpleWorkflow, "test_failure")

    s.True(s.env.IsWorkflowCompleted())

    s.NotNil(s.env.GetWorkflowError())
    s.True(cadence.IsGenericError(s.env.GetWorkflowError()))
    s.Equal("SimpleActivityFailure", s.env.GetWorkflowError().Error())
}

func TestUnitTestSuite(t *testing.T) {
    suite.Run(t, new(UnitTestSuite))
}
```