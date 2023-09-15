
# 引子
ChatGPT回答问题时，是一个字一个字弹出的，给人一种在认真思考的感觉。比如下面这段视频:

大语言模型，Large Language Model，简称LLM。

从模型的视角来看，LLM每进行一次推理生成一个token，直到达到文本长度限制或生成终止符。token是语言模型里的概念，依赖于分词逻辑，有时候它是一个中文字，有时候一个中文词，有时候是一个英文单词。

从服务端的视角来看，生成的token需要通过HTTPS协议逐个返回到浏览器端。


# 概念

Client-Server 模式下，常规的交互方式是client端发送一次请求，接收一次响应。显然，这无法满足ChatGPT回复问题的场景。

其次，我们可能想到websocket，它依赖HTTP实现握手，升级成WebSocket。不过WebSocket需要client和server都持续占用一个socket，server侧成本比较高。

ChatGPT使用的是一种折中方案: server-sent event(简称SSE). 我们从OpenAI的 API 文档可以发现这一点:

![](/img/小文章/640.png)


SSE 模式下，client只需要向server发送一次请求，server就能持续输出，直到需要结束。整个交互过程如下图所示：

![](/img/小文章/650.png)



SSE仍然使用HTTP作为应用层传输协议，充分利用HTTP的长连接能力，实现服务端推送能力。

从代码层面来看，SSE模式与单次HTTP请求不同的点有:

* client端需要开启 keep-alive，保证连接不会超时

* HTTP响应的Header包含 Content-Type=text/event-stream，Cache-Control=no-cache 等

* HTTP响应的body一般是 "data: ..." 这样的结构

* HTTP响应里可能有一些空数据，以避免连接超时


# 代码示例

## 客户端

为了实现stream读取，我们可以分段读取 http.Response.Body。下面是这种方式可行的原因：

* http.Response.Body 的类型是 io.ReaderCloser，底层依赖一个HTTP连接，支持stream读。

* SSE 返回的数据通过换行符\n进行分割

```go
package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"net/http"
	"strings"
)

type ChatCompletionRspChoiceItem struct {
	Delta        map[string]string `json:"delta,omitempty"` // 只有 content 字段
	Index        int               `json:"index,omitempty"`
	Logprobs     *int              `json:"logprobs,omitempty"`
	FinishReason string            `json:"finish_reason,omitempty"`
}

type ChatCompletionRsp struct {
	ID      string                        `json:"id"`
	Object  string                        `json:"object"`
	Created int                           `json:"created"` // unix second
	Model   string                        `json:"model"`
	Choices []ChatCompletionRspChoiceItem `json:"choices"`
}

func main() {
	payload := strings.NewReader("")

	client := &http.Client{}
	req, _ := http.NewRequest("POST", "http://localhost:8088/stream", payload)
	req.Header.Add("Content-Type", "application/json")
	req.Header.Set("Accept", "text/event-stream")
	req.Header.Set("Cache-Control", "no-cache")
	req.Header.Set("Connection", "keep-alive")

	resp, err := client.Do(req)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer resp.Body.Close()

	reader := bufio.NewReader(resp.Body)
	for {
		line, err := reader.ReadBytes('\n')
		if err != nil {
			if err == io.EOF {
				// 忽略 EOF 错误
				break
			} else {
				if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
					fmt.Printf("[PostStream] fails to read response body, timeout\n")
				} else {
					fmt.Printf("[PostStream] fails to read response body, err=%s\n", err)
				}
			}
			break
		}
		line = bytes.TrimSuffix(line, []byte{'\n'})
		line = bytes.TrimPrefix(line, []byte("data: "))
		if bytes.Equal(line, []byte("[DONE]")) {
			break
		} else if len(line) > 0 {
			var chatCompletionRsp ChatCompletionRsp
			if err := json.Unmarshal(line, &chatCompletionRsp); err == nil {
				fmt.Printf(chatCompletionRsp.Choices[0].Delta["content"])
			} else {
				fmt.Printf("\ninvalid line=%s\n", line)
			}
		}
	}

	fmt.Println("the end")
}
```

## 服务端

现在我们尝试mock chatgpt server逐字返回一段文字。这里涉及到两个点:

* Response Header 需要设置 Connection 为 keep-alive 和 Content-Type 为 text/event-stream

* 写入 respnose 以后，需要flush到client端

```go
package main

import (
	"encoding/json"
	"fmt"
	"net/http"
	"time"
)

type ChatCompletionRspChoiceItem struct {
	Delta        map[string]string `json:"delta,omitempty"` // 只有 content 字段
	Index        int               `json:"index,omitempty"`
	Logprobs     *int              `json:"logprobs,omitempty"`
	FinishReason string            `json:"finish_reason,omitempty"`
}

type ChatCompletionRsp struct {
	ID      string                        `json:"id"`
	Object  string                        `json:"object"`
	Created int                           `json:"created"` // unix second
	Model   string                        `json:"model"`
	Choices []ChatCompletionRspChoiceItem `json:"choices"`
}

func streamHandler(w http.ResponseWriter, req *http.Request) {
	w.Header().Set("Connection", "keep-alive")
	w.Header().Set("Content-Type", "text/event-stream")
	w.Header().Set("Cache-Control", "no-cache")

	var chatCompletionRsp ChatCompletionRsp
	runes := []rune(`大语言生成式模型通常使用深度学习技术，例如循环神经网络（RNN）或变压器（Transformer）来建模语言的概率分布。这些模型接收前面的词汇序列，并利用其内部神经网络结构预测下一个词汇的概率分布。然后，模型将概率最高的词汇作为生成的下一个词汇，并递归地生成一个词汇序列，直到到达最大长度或遇到一个终止符号。

在训练过程中，模型通过最大化生成的文本样本的概率分布来学习有效的参数。为了避免模型产生过于平凡的、重复的、无意义的语言，我们通常会引入一些技巧，如dropout、序列扰动等。
  
大语言生成模型的重要应用包括文本生成、问答系统、机器翻译、对话建模、摘要生成、文本分类等。`)
	for _, r := range runes {
		chatCompletionRsp.Choices = []ChatCompletionRspChoiceItem{
			{Delta: map[string]string{"content": string(r)}},
		}

		bs, _ := json.Marshal(chatCompletionRsp)
		line := fmt.Sprintf("data: %s\n", bs)
		fmt.Fprintf(w, line)
		if f, ok := w.(http.Flusher); ok {
			f.Flush()
		}

		time.Sleep(time.Millisecond * 100)
	}

	fmt.Fprintf(w, "data: [DONE]\n")
}

func main() {
	http.HandleFunc("/stream", streamHandler)
	http.ListenAndServe(":8088", nil)
}

```

在真实场景中，要返回的数据来源于另一个服务或函数调用，如果这个服务或函数调用返回时间不稳定，可能导致client端长时间收不到消息，所以一般的处理方式是：

* 对第三方的调用放到一个 goroutine 中

* 通过 time.Tick 创建一个定时器，向client端发送空消息

* 创建一个timeout channel，避免响应时间太久

```go
// 声明一个 event channel
// 声明一个 time.Tick channel
// 声明一个 timeout channel

select {
case ev := <-events:
  // send data event
case <- timeTick:
  // send empty event
case <-timeout:
    fmt.Fprintf(w, "[Done]\n\n")
}
```

# 小结

大语言模型生成响应整个结果的过程是比较漫长的，但token生成的速度比较快，ChatGPT将这一特性与SSE技术充分结合，一个字一个字地弹出到聊天窗，在用户体验上实现了质的提升。

纵观生成式模型，不管是LLAMA/小羊驼 (不能商用)，还是Stable Diffusion/Midjourney。在提供线上服务时，均可利用SSE技术提升用户体验，节省服务器资源。

# 参考

()[https://mp.weixin.qq.com/s/uHrWrTE555fbUyth4kd-_g]