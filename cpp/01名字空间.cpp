/* 

1. 基本概念
名字空间也叫命名空间,标识一个作用域
定义在名字空间中的实体被称为名字空间成员

2. 作用
避免名字冲突
划分逻辑单元

3. 语法

3.1 定义
namespace 名字空间名{
    名字空间的成员1;
    ....
}

名字空间的成员可以是全局函数 全局变量 类型 名字空间

3.2 使用

通过作用域限定操作符 "::"



*/
#include <iostream>;

namespace ns1{
    void func(void){
        std::cout << "ns1的func"<< std::endl;
    }
}

namespace ns2 {
    void func(void){
        std::cout << "ns2的func" << std::endl;
    }
}

 