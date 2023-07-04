# 基于`Reactor`高并发服务器 `C++`

> 基于`Reactor`的高并发服务器，分为`反应堆模型`，`多线程`，`I/O模型`，`服务器`，`Http请求`和`响应`五部分

![全局](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayVyHeW1u9lLN9erbib5Gl9iaFzzueNicVmZLTYkp7MbpD5c6BZRjC6fOMg/640?wx_fmt=jpeg "全局反应堆模型")

# 反应堆模型

## `Channel`

> 描述了文件描述符以及`读写事件`，以及对应的读写销毁回调函数，对应存储`arg`读写回调对应的参数

![Channel](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayyXibpORz5m3TBibibqWYoznveXiasCfgkcHPu5vkr9W7WLoSxTsZJBLlWg/640?wx_fmt=jpeg "channel")



## Channel添加写和判断

> * 异或 `|`：`相同为0`，`异为1`
> * 按位与`&`：只有11为1，其它组合全部为0，即只有`真真为真`，其它`一假则假`
> * 去反 `~`：二进制`全部取反`
>
> * `添加写属性`：若对应为10 想要写添加写属性，与100`异或`，的110读写属性
> * `删除写属性`: 第三位`清零`，若为110，第三位清零，将写取`反011`，在按位与& 010只`留下读事件`

```cpp
// C++11 强类型枚举
enum class FDEvent
{
	TimeOut = 0x01,       //十进制1，超时了 1
	ReadEvent = 0x02,    //十进制2       10
	WriteEvent = 0x04   //十进制4  二进制 100
};
void Channel::writeEventEnable(bool flag)
{
	if (flag) //如果为真，添加写属性
	{
		// 异或 相同为0 异为1
		// WriteEvent 从右往左数第三个标志位1，通过异或 让channel->events的第三位为1
		m_events |= static_cast<int>(FDEvent::WriteEvent); // 按位异或 int events整型32位，0/1,
	}
	else // 如果不写，让channel->events 对应的第三位清零
	{
		// ~WriteEvent 按位与， ~WriteEvent取反 011 然后与 channel->events按位与&运算 只有11 为 1，其它皆为0只有同为真时则真，一假则假，1为真，0为假
		m_events = m_events & ~static_cast<int>(FDEvent::WriteEvent);  //channel->events 第三位清零之后，写事件就不再检测
	}
}
//判断文件描述符是否有写事件
bool Channel::isWriteEventEnable()
{
	return m_events & static_cast<int>(FDEvent::WriteEvent);  //按位与 ，第三位都是1，则是写，如果成立，最后大于0，如果不成立，最后为0
}
```

## `Dispatcher`

> `Dispatcher`作为`父类`函数，对应`Epoll`,`Poll`,`Select模型`。

![反应堆模型](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayFoicEL0lDZhBQib2so4J0dGkjFmdkKzG1KeQKRQoqibRIu3UNmdp6z6sw/640?wx_fmt=jpeg "反应堆模型")



## 选择反应堆模型

> 在`EventLoop`初始化时，针对`全局EventLoop`,将`m_dispatcher`初始化为`EpollDispatcher`.
>
> 使用`多态性`，`父类`建立`虚`函数，`子类`继承复函数，使用`override`取代`父类虚函数`。达到选择反应堆模型。

```cpp
m_dispatcher = new EpollDispatcher(this); //选择模型
//Dispatcher类为父类
virtual ~Dispatcher();  //也虚函数，在多态时
virtual int add();   //等于 = 0纯虚函数，就不用定义
//删除 将某一个节点从epoll树上删除
virtual int remove();
//修改
virtual int modify();
//事件检测， 用于检测待检测三者之一模型epoll_wait等的一系列事件上是否有事件被激活，读/写事件
virtual int dispatch(int timeout = 2);//单位 S 超时时长

//Epoll子类继承父类，override多态性覆盖父类函数，同时public继承，继承Dispatcher的私有变量
class EpollDispatcher : public Dispatcher  //继承父类Dispatcher
{

public:
EpollDispatcher(struct EventLoop* evLoop);
~EpollDispatcher();  //也虚函数，在多态时
// override修饰前面的函数，表示此函数是从父类继承过来的函数，子类将重写父类虚函数
// override会自动对前面的名字进行检查,
int add() override;   //等于 =纯虚函数，就不用定义 
//删除 将某一个节点从epoll树上删除
int remove() override;
//修改
int modify() override;
//事件检测， 用于检测待检测三者之一模型epoll_wait等的一系列事件上是否有事件被激活，读/写事件
int dispatch(int timeout = 2) override;//单位 S 超时时长
// 不改变的不写，直接继承父类
```

## `EventLoop`

> 处理`所有的事件`，启动反应堆模型，处理机会`文件描述符后的事件,添加任务，处理`任务队列
> 调用`dispatcher`中的`添加移除，修改`操作
> 存储着任务队列`m_taskQ`  存储`fd和对应channel对应关系`:`m_channelmap`

###  私有函数变量

```cpp
// CHannelElement结构体
//定义任务队列的节点 类型，文件描述符信息
struct ChannelElement
{
	ElemType type;       //如何处理该节点中Channel
	Channel* channel;   //文件描述符信息
};

//私有函数变量
//加入开关 EventLoop是否工作
bool m_isQuit;
//该指针指向之类的实例epoll,poll,select
Dispatcher* m_dispatcher; 
//任务队列，存储任务，遍历任务队列就可以修改dispatcher检测的文件描述符
//任务队列
queue<ChannelElement*>m_taskQ;
//map 文件描述符和Channel之间的对应关系  通过数组实现
map<int,Channel*> m_channelmap;
// 线程相关，线程ID，name
thread::id m_threadID;
string m_threadName;  //主线程只有一个，固定名称，初始化要分为两个
//互斥锁，保护任务队列
mutex m_mutex;
// 整型数组
int m_socketPair[2]; //存储本地通信fd通过socketpair初始化
```



![EventLoop事件处理](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayzpdqwL3ECRhgnsNDctWflow9wjaZXVH5JrbFVhHic6mHQtZzwHCYusA/640?wx_fmt=jpeg "EventLoop事件处理")

![m_channelmap](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayECvsG4dWSPJUQJoBuD7CBKibb6N4PG2MWQspZc0SEv7bun0oQibu6HJA/640?wx_fmt=jpeg)



![任务队列ChannelElement](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWay1oou6Elyuzqa0qsuibI4y1HBzOybIiavaCiaEcvjKXI97BVDzZTySbXIw/640?wx_fmt=jpeg "任务队列ChannelElement")



![任务队列](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayUuRJVjicERYwdgibPnfXprfwWDX2SDIhcYUFYfI6oCd2sekDL2RvMnNw/640?wx_fmt=jpeg "任务队列list")

### 反应堆运行

> 反应堆模型启动之后将会在`while循环`中一直执行下去。首先调用`dispatcher`调用`Epoll的wait函数`，等待内核回应，根据其读写请求调用`evLoop`的`enactive`函数进行相关的读写操作。

```cpp
int EventLoop::Run()
{
    m_isQuit = false; //不退出
    //比较线程ID，当前线程ID与我们保存的线程ID是否相等
    if (m_threadID != this_thread::get_id())
    {
        //不相等时 直接返回-1
        return -1;
    }
    // 循环进行时间处理
    while (!m_isQuit) //只要没有停止 死循环
    {
        //调用初始化时选中的模型Epoll,Poll，Select
        m_dispatcher->dispatch(); //
        ProcessTaskQ();    //处理任务队列
    }
    return 0;
}
```

### enactive

> 根据传入的`event`调用对应`Channel`对应的`读写回调函数`

```cpp
int EventLoop::eventActive(int fd, int event)
{
    // 判断函数传入的参数是否为有效
    if (fd < 0)
    {
        return -1;
    }
    //基于fd从EventLoop取出对应的Channel
    Channel* channel = m_channelmap[fd]; //channelmap根据对应的fd取出对应的channel
    // 判断取出channel的fd与当前的fd是否相同
    assert(channel->getSocket() == fd); //如果为假，打印出报错信息
    if (event & (int)FDEvent::ReadEvent && channel->readCallback) //channel->readCallback不等于空
    {
        //调用channel的读回调函数
        channel->readCallback(const_cast<void*>(channel->getArg()));
    }
    if (event & (int)FDEvent::WriteEvent && channel->writeCallback)
    {
        channel->writeCallback(const_cast<void*>(channel->getArg()));
    }
    return 0;
}
```

### 添加任务

```cpp
int EventLoop::AddTask(Channel* channel, ElemType type)
{
    //加锁，有可能是当前线程，也有可能是主线程
    m_mutex.lock();
    // 创建新节点
    ChannelElement* node = new ChannelElement;
    node->channel = channel;
    node->type = type;
    m_taskQ.push(node);
    m_mutex.unlock();
    // 处理节点
    /*
    * 如当前EventLoop反应堆属于子线程
    *   1，对于链表节点的添加：可能是当前线程也可能是其它线程(主线程)
    *       1),修改fd的事件，可能是当前线程发起的，还是当前子线程进行处理
    *       2),添加新的fd，和新的客户端发起连接，添加任务节点的操作由主线程发起
    *   2，主线程只负责和客户端建立连接，判断当前线程，不让主线程进行处理，分给子线程
    *       不能让主线程处理任务队列，需要由当前的子线程处理
    */
    if (m_threadID == this_thread::get_id())
    {
        //当前子线程
        // 直接处理任务队列中的任务
        ProcessTaskQ();
    }
    else
    {
        //主线程 -- 告诉子线程处理任务队列中的任务
        // 1,子线程在工作 2，子线程被阻塞了：1，select,poll,epoll,如何解除其阻塞，在本代码阻塞时长是2s
        // 在检测集合中添加属于自己(额外)的文件描述，不负责套接字通信，目的控制文件描述符什么时候有数据,辅助解除阻塞
        // 满足条件，两个文件描述符，可以相互通信，//1，使用pipe进程间通信，进程更可，//2，socketpair 文件描述符进行通信
        taskWakeup(); //主线程调用，相当于向socket添加了数据
    }
    return 0;
}
```

### 处理任务

> 从任务队列中取出一个`任务`，根据`其任务类型`，调用`反应堆模型对应`，将`channel`在内核中的检测进行`删除`，`修改`，或`添加`

```cpp
int EventLoop::ProcessTaskQ()
{
    //遍历链表
    while (!m_taskQ.empty())
    {
        //将处理后的task从当前链表中删除，(需要加锁)
        // 取出头结点
        m_mutex.lock();
        ChannelElement* node = m_taskQ.front(); //从头部
        m_taskQ.pop();  //把头结点弹出，相当于删除 
        
        m_mutex.unlock();
        //读链表中的Channel,根据Channel进行处理
        Channel* channel = node->channel;
        // 判断任务类型
        if (node->type == ElemType::ADD)
        {
            // 需要channel里面的文件描述符evLoop里面的数据
            //添加  -- 每个功能对应一个任务函数，更利于维护
            Add(channel);
        }
        else if (node->type == ElemType::DELETE)
        {
            //Debug("断开了连接");
            //删除
            Remove(channel);
            // 需要资源释放channel 关掉文件描述符，地址堆内存释放，channel和dispatcher的关系需要删除

        }
        else if (node->type == ElemType::MODIFY)
        {
            //修改  的文件描述符事件
            Modify(channel);
        }
        delete node;
    }
    return 0;
}
int EventLoop::Add(Channel* channel)
{
    //把任务节点中的任务添加到dispatcher对应的检测集合里面，
    int fd = channel->getSocket();
    //找到fd对应数组元素的位置，并存储
    if (m_channelmap.find(fd) == m_channelmap.end())
    {
        m_channelmap.insert(make_pair(fd, channel)); //将当前fd和channel添加到map
        m_dispatcher->setChannel(channel); //设置当前channel
        int ret = m_dispatcher->add();  //加入
        return ret;
    }
    return -1;
}

int EventLoop::Remove(Channel* channel)
{
    //调用dispatcher的remove函数进行删除
    // 将要删除的文件描述符
    int fd = channel->getSocket();
    // 判断文件描述符是否已经在检测的集合了
    if (m_channelmap.find(fd) == m_channelmap.end())
    {
        return -1;
    }
    //从检测集合中删除 封装了poll,epoll select
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->remove();
    return ret;
}

int EventLoop::Modify(Channel* channel)
{
    // 将要修改的文件描述符
    int fd = channel->getSocket();
    // TODO判断
    if (m_channelmap.find(fd) == m_channelmap.end()) 
    {
        return -1; 
    }
    //从检测集合中删除
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->modify();
    return ret;
}
```

# 多线程

## `ThreadPool`

> 定义线程池，`运行线程池`，`public函数`取出线程池中某个子线程的`反应堆实例EventLoop`，线程池的`EventLoop反应堆模型`事件由主线程传入，属于`主线程`，其`内部`，`任务队列`，`fd和Channel`对应关系，`ChannelElement`都是所有线程需要使用的数据

![线程池工作](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayibztUVcIf3VKUk8pVcWurWN4S4JkhFwBb9tnVicNlxibuBqxLOicb6DpKw/640?wx_fmt=jpeg "线程池工作")

### 线程池运行创建子工作线程

> 线程池运行语句在主线层运行，根据`当前线程数量`，申请响应的`工作线程池`，并将工作线程运行起来，将工作线程加入到线程池的`vector数组`当中。

```cpp
void ThreadPool::Run()
{
	assert(!m_isStart); //运行期间此条件不能错
	//判断是不是主线程
	if(m_mainLoop->getTHreadID() != this_thread::get_id())
	{
		exit(0);
	}
	// 将线程池设置状态标志为启动
	m_isStart = true;
	// 子线程数量大于0
	if (m_threadNum > 0)
	{
		for (int i = 0; i < m_threadNum; ++i)
		{
			WorkerThread* subThread = new WorkerThread(i); // 调用子线程
			subThread->Run();
			m_workerThreads.push_back(subThread);
		}
	}
}
```

### 取出工作线程池中的`EventLoop`

```cpp
EventLoop* ThreadPool::takeWorkerEventLoop()
{
	//由主线程来调用线程池取出反应堆模型
	assert(m_isStart); //当前程序必须是运行的
	//判断是不是主线程
	if (m_mainLoop->getTHreadID() != this_thread::get_id())
	{
		exit(0);
	}
	//从线程池中找到一个子线层，然后取出里面的反应堆实例
	EventLoop* evLoop = m_mainLoop; //将主线程实例初始化
	if (m_threadNum > 0)
	{
		evLoop = m_workerThreads[m_index]->getEventLoop();
		//雨露均沾，不能一直是一个pool->index线程
		m_index = ++m_index % m_threadNum;
	}
	return evLoop;
}
```

### 工作线程运行

> 在子线程中申请`反应堆模型`，供子线程在客户端连接时取出 ,供类`Connection`使用

```cpp
void WorkerThread::Run()
{
	//创建子线程,3,4子线程的回调函数以及传入的参数
	//调用的函数，以及此函数的所有者this
	m_thread = new thread(&WorkerThread::Running,this);
	// 阻塞主线程，让当前函数不会直接结束，不知道当前子线程是否运行结束
	// 如果为空，子线程还没有初始化完毕,让主线程等一会，等到初始化完毕
	unique_lock<mutex> locker(m_mutex);
	while (m_evLoop == nullptr)
	{
		m_cond.wait(locker);
	}
}

void* WorkerThread::Running()
{
	m_mutex.lock();
	//对evLoop做初始化
	m_evLoop = new EventLoop(m_name);
	m_mutex.unlock();
	m_cond.notify_one(); //唤醒一个主线程的条件变量等待解除阻塞
	// 启动反应堆模型
	m_evLoop->Run();
}
```

# `IO` 模型

## `Buffer`

> `读写`内存结构体，添加字符串，`接受套接字数据`，将写缓存区数据`发送`

![读写位置移动](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayOWMYdF95GsoRt4michrF54jaMpzh7mXAjoKQ0JHXIF6yOqJrUPZARPA/640?wx_fmt=jpeg "读写位置移动")

### 发送目录

```cpp
int Buffer::sendData(int socket)
{
	// 判断buffer里面是否有需要发送的数据 得到未读数据即待发送
	int readable = readableSize();
	if (readable > 0)
	{
		//发送出去buffer->data + buffer->readPos 缓存区的位置+已经读到的位置
		// 管道破裂 -- 连接已经断开，服务器继续发数据，出现管道破裂 -- TCP协议
		// 当内核产生信号时，MSG_NOSIGNAL忽略，继续保持连接
		// Linux的信号级别高，Linux大多数信号都会终止信号
		int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
		if (count > 0)
		{
			// 往后移动未读缓存区位置
			m_readPos += count;
			// 稍微休眠一下
			usleep(1); // 1微妙
		}
		return count;
	}
	return 0;
}
```



### 发送文件

> 发送文件是不需要将读取到的文件`放入缓存`的，直接内核发送提高`文件IO`效率。

```cpp
int Buffer::sendData(int cfd, int fd, off_t offset, int size)
{
	int count = 0;
	while (offset < size)
	{
		//系统函数，发送文件，linux内核提供的sendfile 也能减少拷贝次数
		// sendfile发送文件效率高，而文件目录使用send
		//通信文件描述符，打开文件描述符，fd对应的文件偏移量一般为空，
		//单独单文件出现发送不全，offset会自动修改当前读取位置
		int ret = (int)sendfile(cfd, fd, &offset, (size_t)(size - offset));
		if (ret == -1 && errno == EAGAIN)
		{
			printf("not data ....");
			perror("sendfile");
		}
		count += (int)offset;
	}
	return count;
}

```

## `TcpConnection`

> 负责`子线程与客户端`进行通信，分别存储这`读写销毁回调函数`->调用相关`buffer函数`完成相关的`通信功能`

![TcpConnection](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayFJIuzTia0ibOdic4EeGFqGao0wWwwiaTBKLx254ialoFBlXPVE7xVMAc5vA/640?wx_fmt=jpeg "TcpConnection工作")

![主线程](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWay7rlB4x3e64g8HNnLNJRicqJibNicgYZabIicEcC8A9uV6Xz4UNbTGVhrmw/640?wx_fmt=jpeg "主线程")

### 初始化

> 申请`读写缓存区`，并初始化`Channel`，初始化`子线程与客户端`与`服务器进行通信时回调函数`

```cpp
TcpConnection::TcpConnection(int fd, EventLoop* evloop)
{
	//并没有创建evloop，当前的TcpConnect都是在子线程中完成的
	m_evLoop = evloop;
	m_readBuf = new Buffer(10240); //10K
	m_writeBuf = new Buffer(10240);
	// 初始化
	m_request = new HttpRequest;
	m_response = new HttpResponse;

	m_name = "Connection-" + to_string(fd);

	// 服务器最迫切想知道的时候，客户端有没有数据到达
	m_channel =new Channel(fd,FDEvent::ReadEvent, processRead, processWrite, destory, this);
	// 把channel放到任务循环的任务队列里面
	evloop->AddTask(m_channel, ElemType::ADD);
}
```

### 读写回调

> 读事件将调用`HttpRequest`解析，客户端发送的`读取请求`。写事件，将针对读事件将对应的数据`写入缓存区`，由写事件进行发送。但由于`效率的考虑`，在读事件时，已经设置成边`读变发送提高效率`，发送文件也将采用Linux内核提供的`sendfile方法`，不读取内核直接发送，比`send`的效率`快`了，很多，在很大程度上，写事件的写功能基本被`架空`。

```cpp
int TcpConnection::processRead(void* arg)
{
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// 接受数据 最后要存储到readBuf里面
	int socket = conn->m_channel->getSocket();
	int count = conn->m_readBuf->socketRead(socket);
	// data起始地址 readPos该读的地址位置
	Debug("接收到的http请求数据: %s", conn->m_readBuf->data());

	if (count > 0)
	{
		// 接受了http请求，解析http请求
		
#ifdef MSG_SEND_AUTO
		//添加检测写事件
		conn->m_channel->writeEventEnable(true);
		//  MODIFY修改检测读写事件
		conn->m_evLoop->AddTask(conn->m_channel, ElemType::MODIFY);
#endif
		bool flag = conn->m_request->parseHttpRequest(
			conn->m_readBuf, conn->m_response,
			conn->m_writeBuf, socket);
		if (!flag)
		{
			//解析失败，回复一个简单的HTML
			string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			conn->m_writeBuf->appendString(errMsg);
		}
	}
	else
	{
		
#ifdef MSG_SEND_AUTO  //如果被定义，
		//断开连接
		conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
#endif
	}
	// 断开连接 完全写入缓存区再发送不能立即关闭，还没有发送
#ifndef MSG_SEND_AUTO  //如果没有被定义，
	conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
#endif
	return 0;
}

//写回调函数，处理写事件，将writeBuf中的数据发送给客户端
int TcpConnection::processWrite(void* arg)
{
	Debug("开始发送数据了(基于写事件发送)....");
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	// 发送数据
	int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
	if (count > 0)
	{
		// 判断数据是否全部被发送出去
		if (conn->m_writeBuf->readableSize() == 0)
		{
			// 数据发送完毕
			// 1，不再检测写事件 --修改channel中保存的事件
			conn->m_channel->writeEventEnable(false);
			// 2, 修改dispatcher中检测的集合，往enentLoop反映模型认为队列节点标记为modify
			conn->m_evLoop->AddTask(conn->m_channel, ElemType::MODIFY);
			//3，若不通信，删除这个节点
			conn->m_evLoop->AddTask(conn->m_channel, ElemType::DELETE);
		}
	}
	return 0;
}
```

## `HttpRequest`

> 定义`http 请求结构体`添加请求头结点，`解析请求行`，头，`解析/处理http`请求协议，获取文件类型
> 发送`文件/目录` 设置请求`url,Method，Version ,state`

### 处理客户端解析请求

> 在`while循环内部`，完成对`请求行`和`请求头`的解析。解析完成之后，根据请求行，读取`客户端需要`的数据，并对应进行操作

```cpp
bool HttpRequest::parseHttpRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket)
{
	bool flag = true;
	// 先解析请求行
	while (m_curState !=PressState::ParseReqDone)
	{
		// 根据请求头目前的请求状态进行选择
		switch (m_curState)
		{
		case PressState::ParseReqLine:
			flag = parseRequestLine(readBuf);
			break;
		case PressState::ParseReqHeaders:
			flag = parseRequestHeader(readBuf);
			break;
		case PressState::ParseReqBody: //post的请求，咱不做处理
			// 读取post数据
			break;
		default:
			break;
		}
		if (!flag)
		{
			return flag;
		}
		//判断是否解析完毕，如果完毕，需要准备回复的数据
		if (m_curState == PressState::ParseReqDone)
		{
			// 1，根据解析出的原始数据，对客户端请求做出处理
			processHttpRequest(response);
			// 2,组织响应数据并发送
			response->prepareMsg(sendBuf, socket);
		}
	}
	// 状态还原，保证还能继续处理第二条及以后的请求
	m_curState = PressState::ParseReqLine;
	//再解析请求头
	return flag;
}
```

### 处理客户端请求

> 根据请求行规则判断是`请求目录`，还是`请求文件`，调用`Buffer`相关`发送目录`，和`发送文件重载函数`，完成`通信任务`。

```cpp
bool HttpRequest::processHttpRequest(HttpResponse* response)
{
	if (strcasecmp(m_method.data(), "get") != 0)   //strcasecmp比较时不区分大小写
	{
		//非get请求不处理
		return -1;
	}
	m_url = decodeMsg(m_url); // 避免中文的编码问题 将请求的路径转码 linux会转成utf8
	//处理客户端请求的静态资源(目录或文件)
	const char* file = NULL;
	if (strcmp(m_url.data(), "/") == 0) //判断是不是根目录
	{
		file = "./";
	}
	else
	{
		file = m_url.data() + 1;  // 指针+1 把开始的 /去掉吧

	}
	//判断file属性，是文件还是目录
	struct stat st;
	int ret = stat(file, &st); // file文件属性，同时将信息传入st保存了文件的大小
	if (ret == -1)
	{
		//文件不存在  -- 回复404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd); //发送404对应的html文件
		response->setFileName("404.html");
		response->setStatusCode(StatusCode::NotFound);
		// 响应头
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;

	}
	response->setFileName(file);
	response->setStatusCode(StatusCode::OK);
	//判断文件类型
	if (S_ISDIR(st.st_mode)) //如果时目录返回1，不是返回0
	{
		//把这个目录中的内容发送给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), (int)st.st_size);
		//sendDir(file, cfd);
		// 响应头
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
	}
	else
	{
		//把这个文件的内容发给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), (int)st.st_size);
		//sendFile(file, cfd);
		// 响应头
		response->addHeader("Content-type", getFileType(file));
		response->addHeader("Content-length", to_string(st.st_size));
		response->sendDataFunc = sendFile;
	}
	return false;
}
```

## `HttpResponse`

> 定义`http响应`，`添加响应头`，准备响应的数据

# 服务器

## `TcpServer`

> `服务器类`，复制服务器的初始化，`设置监听`，`启动服务器`，并接受`主线程的连接请求`

![TcpServer工作流程](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWaybIOnzsoQRjxTJYpKVfLRAIX5kbdF90FdlAITz0RZE9bvbkx4PAicLuA/640?wx_fmt=jpeg "TCpServer工作流程")





# 主函数

* 传入用户输入的`端口`和`文件夹`
	* 端口将作为服务器`端口`，文件夹将作为浏览器访问的文件夹
* 初始化`TcpServer`服务器实例 - 传入端口和`初始化线程个数`
* 运行服务器
```cpp
#include <stdlib.h>
#include <unistd.h>
#include "TcpServer.h"
//初始化监听的套接字
// argc 输入参数的个数
// argv[0]可执行程序的名称 
// argv[1]传入的第一个参数， port
// argv[2]传入的第二个参数   path
int main(int argc, char* argv[])
{
#if 0
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = (unsigned short)atoi(argv[1]);
    //切换服务器的根目录，将根目录当前目录切换到其它目录
    chdir(argv[2]);
    // 启动服务器
#else
    // VS code 调试
    unsigned short port = 8080;
    chdir("/home/foryouos/blog");
#endif
    // 创建服务器实例
    TcpServer* server = new TcpServer(port, 4);
    // 服务器运行 - 启动线程池-对监听的套接字进行封装，并放到主线程的任务队列，启动反应堆模型
    // 底层的epoll也运行起来，
    server->Run();
    return 0;
}
```
# 初始化`TcpServer`

![初始化TcpServer](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbtycdzkcLic0JLia7rAVa6cia5eS1xx3fWYlEUTcSw2Z0ethrjONW7NsSaSNNicZgpYCk8NAPzoiaIeqcw/640?wx_fmt=jpeg "初始化TcpServer")


# 启动`TcpServer`

![启动TcpServer](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWayuDicPlYutY6icNuYHb5WKQhPib0BkvU6WfFWzr3dUcicfFTiaq0sJjTpAicg/640?wx_fmt=jpeg "启动TCpServer")

# 检测到客户端请求

![客户端请求](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvYbuq71ib9iaeQVznZ5coWaybSQDFav1DdHFxBZibVBXqrYg9mNEEaZMdZVZibvwtDF0A8fRZSGoFEZA/640?wx_fmt=jpeg "客户端请求")


# 详细代码

> [Github](https://github.com/foryouos/cppserver-linux/tree/main/c_simple_server/cpp_server)[^1]

[^1]:https://github.com/foryouos/cppserver-linux/tree/main/c_simple_server/cpp_server