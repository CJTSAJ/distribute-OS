# Reliable Data Transport Protocol
## 设计策略
- **策略描述**
  - 主要使用的算法是Go-Back-N协议
  - 客户端设计了两个缓存，**一个缓存存放最近发出去的packet**，用于packet丢失时重发；**另一个缓存存放服务端发来的ack**，并且这些ack都大于当前期望的ack值，即后发于期望packet的packet的ack。当期望packet没有收到ack时，重发该packet，之后的包根据ack缓存确定是否需要重发。
  - 服务端设计了一个缓存，**存放后发于期望packet的packet**，当期望packet包宋送达时，直接从缓存中读取之后的包，并向客户端发送响应的ack。
  - 错误检测运用了常见的**checksum算法**，对于错误的packet直接丢弃重发，没做校验。
  - 经过测试滑动窗口的大小在10左右，time out间隔为0.1时速度和包的数量都趋向最优。
 
- **数据包结构描述**
  - 数据包头长度为7 bytes
  - 0~1 byte为short类型的**checksum**
  - 2~5 byte为int类型的**seq_number**，即包的序列
  - 第6位byte为char类型**length**，表示包中有效数据的长度
  
- **数据包结构示意**

|checksum|packet seq|payload size|payload|
|-|-|-|-|
|2 byte|4 byte|1 byte|The rest|

- **ack包结构描述**
  - ack包只有两部分，一部分为checksum，另一部分为ack_number
  - 0~1 byte为short类型的checksum
  - 2~5 byte为int类型的ack_number，即确认位
 
- **ack包结构示意**

|checksum|ack_number|nothing|
|-|-|-|
|2 byte|4 byte|The rest|
