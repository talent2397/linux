/**
 * @file base_utils.c
 * @brief 基础工具函数库 - 提供系统命令执行、进程状态检查等通用功能
 *
 * @details 该文件包含了一系列用于执行系统命令、检查进程状态等通用功能的函数。
 * 这些函数主要用于WiFi模块的系统操作，但也可以在其他模块中使用。
 *
 * @author 系统开发组
 * @version 1.0
 * @date 2025-03-18
 */

#include <stdlib.h>	   // 标准库函数：malloc, free, system, exit等
#include <stddef.h>	   // 标准定义：NULL, size_t, ptrdiff_t等
#include <stdbool.h>   // 布尔类型：true, false
#include <stdio.h>	   // 标准输入输出：printf, FILE, fopen, fread等
#include <stdarg.h>	   // 可变参数处理：va_list, va_start, va_end等
#include <string.h>	   // 字符串操作：strcpy, strcat, strlen, memset等
#include <sys/types.h> // 系统类型定义：pid_t, size_t等
#include <sys/time.h>  // 时间相关：gettimeofday, timeval结构体等

/**
 * @brief 安全执行系统命令并检查执行结果
 *
 * @param cmd 要执行的系统命令字符串
 * @return int 执行结果：0=成功，1=系统调用失败，2=子进程异常退出，其他=子进程退出码
 *
 * @note 该函数是对标准system()函数的增强版本，提供了详细的错误检查和状态返回
 * @example
 *   int result = base_utils_system("ifconfig wlan0 up");
 *   if(result == 0) {
 *       printf("命令执行成功\n");
 *   }
 */
int base_utils_system(char *cmd)
{
	int ret = 0; // 返回值变量，用于存储子进程的退出状态码

	// 执行系统命令，system()函数会创建子进程来执行命令
	pid_t status = system(cmd);

	// 检查system()函数本身是否执行失败（比如无法创建子进程）
	if (status == -1)
	{
		// system()函数调用失败，可能是系统资源不足等原因
		printf("system(\"%s\") error", cmd);
		return 1; // 返回错误码1表示system调用失败
	}
	else
	{
		// system()调用成功，现在检查子进程的退出状态

		// WIFEXITED宏检查子进程是否正常退出（通过exit()或return退出）
		if (WIFEXITED(status))
		{
			// 子进程正常退出，获取其退出状态码
			ret = WEXITSTATUS(status);

			// 检查退出状态码，0表示成功，非0表示失败
			if (ret == 0)
			{
				// 命令执行成功
				return 0; // 返回0表示完全成功
			}
			else
			{
				// 命令执行失败，返回具体的错误码
				printf("%s return error: %d", cmd, ret);
				return ret; // 返回子进程的退出码，便于调试
			}
		}
		else
		{
			// 子进程异常退出（比如被信号终止，如Ctrl+C）
			printf("%s exec error", cmd);
			return 2; // 返回错误码2表示子进程异常退出
		}
	}
}

/**
 * @brief 执行系统命令并获取命令的输出结果
 *
 * @param exe 要执行的命令字符串
 * @param buf 存储命令输出的缓冲区指针，如果为NULL则不读取输出
 * @param len 缓冲区的大小（字节数）
 * @return int 实际读取的字节数，如果buf为NULL则返回0
 *
 * @note 该函数使用popen()创建管道来执行命令，可以读取命令的标准输出
 * @warning 缓冲区buf必须足够大以容纳命令输出，否则可能造成数据截断
 * @example
 *   char output[100];
 *   int bytes = base_utils_shell_exec("date", output, sizeof(output)-1);
 *   if(bytes > 0) {
 *       output[bytes] = '\0';  // 添加字符串结束符
 *       printf("当前时间: %s\n", output);
 *   }
 */
int base_utils_shell_exec(const char *exe, char *buf, int len)
{
	int count = 0; // 读取的字节数计数器，初始化为0

	// 使用popen()创建管道执行命令，"r"表示以读取模式打开
	// popen()比system()更高级，可以获取命令的输出结果
	FILE *stream = popen(exe, "r");

	// 检查管道是否创建成功
	if (stream == NULL)
	{
		// popen()调用失败，可能是命令不存在或权限不足
		return count; // 返回0表示没有读取到数据
	}

	// 如果提供了有效的缓冲区指针，则读取命令输出
	if (buf != NULL)
	{
		// 从管道中读取命令输出到缓冲区
		// fread()参数：目标缓冲区，元素大小，元素数量，文件流
		count = fread(buf, sizeof(char), len, stream);
	}

	// 关闭管道，释放系统资源
	// 必须调用pclose()，否则会造成文件描述符泄漏
	pclose(stream);

	// 返回实际读取的字节数
	return count;
}

/**
 * @brief 检查指定名称的进程是否正在运行
 *
 * @param name 要检查的进程名称
 * @param length 进程名称的长度
 * @return int 进程状态：1=进程存在，-1=进程不存在或检查失败
 *
 * @note 该函数通过执行"ps | grep"命令来查找进程，会排除grep命令本身
 * @warning 进程名长度不能超过20个字符，否则可能造成缓冲区溢出
 * @example
 *   int state = base_utils_get_process_state("wpa_supplicant", 14);
 *   if(state == 1) {
 *       printf("WiFi服务正在运行\n");
 *   } else {
 *       printf("WiFi服务未运行\n");
 *   }
 */
int base_utils_get_process_state(const char *name, int length)
{
	int bytes;	  // 读取的字节数
	char buf[10]; // 小缓冲区，只需要知道是否有输出，不需要具体内容
	char cmd[60]; // 命令缓冲区，存储要执行的shell命令
	FILE *strea;  // 文件流指针（应该是stream，这里拼写有误）

	// 安全检查：进程名长度不能超过20个字符
	// 防止命令缓冲区溢出（cmd[60]最多容纳60个字符）
	if (length > 20)
	{
		printf("process name is too long!"); // 注意：拼写错误，应该是"name"
		return -1;							 // 返回错误码-1
	}

	// 构造查找进程的命令：
	// ps: 列出所有进程
	// grep %s: 过滤出包含指定名称的进程行
	// grep -v grep: 排除掉grep命令本身（因为grep也会出现在进程列表中）
	sprintf(cmd, "ps | grep %s | grep -v grep", name);

	// 执行命令并创建管道
	strea = popen(cmd, "r");

	// 检查管道是否创建成功
	if (!strea)
		return -1; // popen失败，返回-1

	// 读取命令输出到缓冲区
	// 我们不需要具体内容，只需要知道是否有输出（即进程是否存在）
	bytes = fread(buf, sizeof(char), sizeof(buf), strea);

	// 关闭管道
	pclose(strea);

	// 判断进程是否存在
	if (bytes > 0)
	{
		// 读取到了数据，说明找到了匹配的进程
		printf("%s :process exist", name);
		return 1; // 返回1表示进程存在
	}
	else
	{
		// 没有读取到数据，说明进程不存在
		printf("%s :process not exist", name);
		return -1; // 返回-1表示进程不存在
	}
}