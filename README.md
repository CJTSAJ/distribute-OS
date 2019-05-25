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
