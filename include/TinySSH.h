int TinySSHAppServer(int argc, char* argv[]);
/*
    远程控制台 客户端使用说明：
    1、登录指令：           login xxxxxxx          跟在login后面的xxxxxxx的是你指定的密钥
    2、连接指令：           connect xxxxxx         跟在connect后面的xxxxxx是你想连接的客户端的密钥，如果匹配成功，则能远程控制该客户端。进入cmd模式
    3、退出cmd模式指令：    :quit                  退出cmd指令模式
    4、进入cmd模式指令：    :cmd                   进入cmd指令模式
    5、取消连接指令：       disconnect             取消连接当前连接的客户端
*/
int TinySSHAppClient(int argc, char* argv[]);