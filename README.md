# SDIC course lab resources for MapReduce.

This is a Java-implementation for MIT 6.824 Lab 1: MapReduce. For SDIC course lab, SJTU.

Some resources directly or indirectly refer to the [original materials](https://pdos.csail.mit.edu/6.824/labs/lab-1.html).

**Notice:** Only for educational or personal usage.

## Part1
完成doMap和doReduce函数

### doMap
- 以字符串形式读取inFile文件所有内容
- 调用mapFunc类下的map函数，得到一个KeyValue对的List
- 利用hashCode函数将各个KeyValue对分配到nReduce个reduce
- 根据jobName、mapTask和reduce编号生成中间文件，并将对应的KeyValue对写入文件(以JSON的格式)

### doReduce
- 读取doMap函数分配给该reduce的文件，并将所有内容组织成一个List
- 按照Key排序
- 将Key相同的value组织成一个List
- 对每一个Key都调用一次reduceFunc类下的reduce函数，得到一个字符串结果
- 将所有key和对应的结果以JSON的形式写入outFile文件

## Part2
完成WordCount下的mapFunc和reduceFunc函数，这两个函数将会作为参数传递到part1完成的doMap和doReduce函数中，所以两个part要结合起来看才能实现。

### mapFunc
- 利用matcher和pattern工具从文件内容中提炼出单词
- 将单词组织成KeyValue对，并返回一个包含所有单词的List(存在大量重复单词)，key为单词，value可以设定为任意字符串，为了便于理解，此处将value值设置为"1"，即单词数计为1的意思
- 返回的List<KeyVaue>将会被part1中的doMap函数分配给各个reduce去计数
  
### reduceFunc
- 该函数在part1中doReduce中调用
- 此时doReduce已将重复单词的value整合成一个数组
- 该**单词的数量即为该数组的长度**，所以直接返回传入字符串数组的长度即可

## Part3
该部分主要是对mapReduce进行多线程并发执行。需要完成的函数只有schedule函数，Master会在两个阶段map和reduce分别调用该函数，schedule的工作主要是讲当前所有的task分配给不同的worker，一般来说，task的数量多于worker数量，所以一个worker可能会处理多个task。
注：schedule应该**等待所有task完成后**再返回。

- 用CopyOnWriteArrayList<Boolean>数据结构标记所有task完成情况
- 进入一个大循环，当nTask个task全部执行完后退出循环
- 用CountDownLatch保证当所有task完成后才退出，每个task线程执行后都会把latch-1，latch等于0时，表示所有线程完成

