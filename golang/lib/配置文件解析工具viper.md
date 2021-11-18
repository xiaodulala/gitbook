# 概述

> Vipier 是一个完整的Go应用程序配置文件解析方案。它可以处理任何类型或者格式的配置文件。
> 

源码地址: [viper](https://github.com/spf13/viper)

Vipier支持:

1. 设置默认值。
2. 从不同格式的配置文件中读取配置包括json,toml,yaml,hcl,envfile.
3. 从环境变量读取配置
4. 从第三方配置存储服务读取配置 etcd consul并且监听配置改变。
5. 从命令行参数选项中读取配置。
6. 从本地缓冲中读取配置
7. 直接设置值。

Vipie读取配置文件方式的顺序从高到底为:

1. 直接使用 set的方式设置
2. 命令行参数
3. 环境变量
4. 配置文件
5. kv存储
6. 默认值

> [!TIP]
> viper配置键不区分大小写。

# 使用

*  导入包

```bash
go get github.com/spf13/viper
```

* Viper使用方式。

viper中初始化了一个全局的vipier实例。所以我们可以在导入包后直接使用viper.set等方法设置配置值。我们可以在我们的应用中直接使用这个实例。

```go
var v *Viper

func init() {
	v = New()
	......
}
```
* 设置默认值

```go
func main() {
	viper.SetDefault("ContentDir", "content")
	viper.SetDefault("LayoutDir", "layouts")
	
	fmt.Println(viper.GetString("contentdir")) //ok
	fmt.Println(viper.GetString("ContentDir")) // ok
}
```

* 从配置文件读取

```go
func main() {
	viper.SetConfigName("config")
	viper.SetConfigType("yaml")
	viper.AddConfigPath(".")
	viper.AddConfigPath("./config")
	if err := viper.ReadInConfig(); err != nil {
		// 如果配置文件未找到的错误想被特殊处理
		if _, ok := err.(viper.ConfigFileNotFoundError); ok {
			fmt.Println("config not found")
		} else {
			fmt.Println("cofnig file cound but other errors")
			panic(err)
		}
	}

	viper.SetDefault("app.name", "default")

	fmt.Println(viper.GetString("app.name")) //配置文件优先级高于默认值
	fmt.Println(viper.GetStringSlice("app.databases"))
}
```

* 回写配置文件

一共有以下几种操作

* WriteConfig  按照**初始化的配置文件路径** 回写配置。如果路径不存在，报错。文件存在则**直接覆盖**。
* SafeWriteConfig 同上 路径不存在，报错。文件存在，**不会覆盖**。

* WriteConfigAs  写入**给定的文件路径**下。如果文件存在，**会覆盖**
* SafeWriteConfigAs 写入**给定的文件路径**下。如果文件存在，**不会覆盖**

不加AS,回写到viper.AddConfigPath(".")下的路径下。加AS,指定路径。  
加Safe,如果文件存在不覆盖。不加Safe，文件存在会覆盖。

```go
	//重新设置值
	viper.Set("app.name", "gogogo")
	//回写配置文件
	viper.WriteConfig() // 回写到AddConfigPath路径中。
	viper.SafeWriteConfig()
	viper.WriteConfigAs("./tpl/config.yaml")
	viper.SafeWriteConfigAs("./tpl/config.yaml")
```


* 配置文件热加载

```go
func main() {
	viper.SetConfigName("config")
	viper.SetConfigType("yaml")
	viper.AddConfigPath(".")
	viper.AddConfigPath("./config")
	if err := viper.ReadInConfig(); err != nil {
		// 如果配置文件未找到的错误想被特殊处理
		if _, ok := err.(viper.ConfigFileNotFoundError); ok {
			fmt.Println("config not found")
		} else {
			fmt.Println("cofnig file cound but other errors")
			panic(err)
		}
	}
	viper.WatchConfig()

	go func() {
		viper.OnConfigChange(func(e fsnotify.Event) {
			fmt.Println(e.Name) // Users/duyong/WorkPlace/goDemo/pflagDemo/config.yaml
			fmt.Println(e.Op)   // WRITE
			fmt.Println(viper.GetString("app.name"))
		})
	}()

	select {}

}
```
* 缓存中读取

```go
viper.SetConfigType("yaml") // or viper.SetConfigType("YAML")

// any approach to require this configuration into your program.
var yamlExample = []byte(`
Hacker: true
name: steve
hobbies:
- skateboarding
- snowboarding
- go
clothing:
  jacket: leather
  trousers: denim
age: 35
eyes : brown
beard: true
`)

viper.ReadConfig(bytes.NewBuffer(yamlExample))

viper.Get("name") // this would be "steve"
```

*  读取环境变量

提供的方法：
1.  AutomaticEnv()
2. BindEnv(string...) : error
3. SetEnvPrefix(string)
4. SetEnvKeyReplacer(string...) *strings.Replacer
5. AllowEmptyEnv(bool)

在处理ENV变量时，重要的是要认识到Viper将ENV变量视为区分大小写的。

Viper提供了一种机制来尝试确保ENV变量是惟一的。通过使用SetEnvPrefix，可以告诉Viper在读取环境变量时使用前缀。BindEnv和AutomaticEnv都将使用这个前缀。

```go
func main() {
	viper.SetEnvPrefix("viper") // 自动转为大写

	// 第一个参数为键名。只有第一个参数时,环境变量默认使用 前缀_大写键名 VIPER_USERNAME
	viper.BindEnv("username")
	os.Setenv("VIPER_USERNAME", "jack")
	fmt.Println(viper.Get("username"))

	// 当有第二个参数时,第二个参数为显示指定的环境变量。就叫ID.
	viper.BindEnv("id", "ID")
	os.Setenv("ID", "123")
	fmt.Println(viper.Get("id"))

}
```

```go
func main() {
	os.Setenv("VIPER_USER_NAME", "jack")
	os.Setenv("VIPER_USER_AGE", "20")

	viper.AutomaticEnv()
	viper.SetEnvPrefix("VIPER")
	// viper.get时 key中的. _ - 被替换为_
	viper.SetEnvKeyReplacer(strings.NewReplacer(".", "_", "-", "_"))
	viper.BindEnv("user.name")
	viper.BindEnv("user.age", "USER_AGE")

	fmt.Println(viper.Get("user.name"))
	fmt.Println(viper.Get("user.age"))
}
```

* 与Pflag绑定使用

```go
func main() {
	// 绑定单个标志
	port := pflag.Int("port", 1138, "Port to run Application server on")
	viper.BindPFlag("port", pflag.Lookup("port"))
	pflag.Parse()
	fmt.Println(*port)
}
```

```go
func main() {
	// 绑定标志集
	port := pflag.Int("port", 1138, "Port to run Application server on")
	viper.BindPFlags(pflag.CommandLine)
	pflag.Parse()
	fmt.Println(*port)
}
```

* 从远程键值对数据库读取

```go
viper.AddSecureRemoteProvider("etcd","http://127.0.0.1:4001","/config/hugo.json","/etc/secrets/mykeyring.gpg")
viper.SetConfigType("json") // because there is no file extension in a stream of bytes,  supported extensions are "json", "toml", "yaml", "yml", "properties", "props", "prop", "env", "dotenv"
err := viper.ReadRemoteConfig()
```



