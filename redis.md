一、`redis`基础连接命令

```mysql
# 本地默认连接（127.0.0.1:6379）
redis-cli

# 指定IP、端口、密码连接
redis-cli -h IP地址 -p 端口 -a 密码

```

二、基础操作

```mysql
PING                # 心跳检测，正常返回 PONG
SET k1 v1           # 设置键值
GET k1              # 获取值
SELECT 数据库编号   # 切换数据库（Redis 默认 0~15 共16个库）

```

三、通用命令，针对`key`操作，不区分`value`类型

| 命令               | 作用                       | 示例 & 说明                                       |
| ------------------ | -------------------------- | ------------------------------------------------- |
| `KEYS pattern`     | 匹配查询所有符合规则的 key | `KEYS *` 查看全部 key⚠️ 生产环境禁用，会阻塞服务   |
| `DEL key [key...]` | 删除一个 / 多个 key        | `DEL k1 k2`                                       |
| `EXISTS key`       | 判断 key 是否存在          | 存在返回 1，不存在返回 0                          |
| `EXPIRE key 秒数`  | 给 key 设置过期时间        | `EXPIRE code 300` 5 分钟后自动删除                |
| `TTL key`          | 查看 key 剩余过期时间      | 正数：剩余秒数-1：永久有效-2：key 已失效 / 不存在 |

四、`redis`五大基本数据类型及命令

1.`string`字符串类型

最基础类型，数字类型支持自增、自减

```mysql
# 1. 增、改、查
SET key value                 # 设置键值（存在则覆盖）
GET key                       # 获取值

# 2. 批量操作
MSET k1 v1 k2 v2 k3 v3        # 批量设置
MGET k1 k2 k3                 # 批量获取

# 3. 过期 + 不存在才设置（常用做缓存、分布式锁）
SETEX key 秒数 value          # 设置值 + 同时指定过期时间
SETNX key value               # key 不存在才设置，存在则不操作

# 4. 数字自增/自减
INCR key                      # 整数 +1
INCRBY key 步长               # 整数 + 指定数值
INCRBYFLOAT key 小数步长      # 浮点数累加

```

`key`命名规范：

格式：项目名：业务：标识：`id`

`eg : heima:user:1、heima:goods:1001`

对象建议存`JSON`字符串

2.`Hash`哈希类型

`value`是字段-值结构，类似`map`

可以单独修改某一个字段，适合存储对象

```mysql
# 单个字段操作
HSET key field value                # 设置哈希字段
HGET key field                      # 获取单个字段值

# 批量字段操作
HMSET key f1 v1 f2 v2               # 批量设置字段
HMGET key f1 f2                     # 批量获取字段

# 全量获取
HGETALL key                         # 获取所有 field 和 value
HKEYS key                           # 只获取所有字段名
HVALS key                           # 只获取所有字段值

# 其他
HINCRBY key field 步长              # 哈希内数字字段自增
HSETNX key field value              # 字段不存在才设置

```

3.`list`列表类型

底层双向链表，元素有序、可重复

头尾增删快，中间查询慢

