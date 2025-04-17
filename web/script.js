document.addEventListener('DOMContentLoaded', function() {
  // 获取DOM元素
  const serverIpInput = document.getElementById('server-ip');
  const serverPortInput = document.getElementById('server-port');
  const connectBtn = document.getElementById('connect-btn');
  const disconnectBtn = document.getElementById('disconnect-btn');
  const messageArea = document.getElementById('message-area');
  const messageInput = document.getElementById('message-input');
  const sendBtn = document.getElementById('send-btn');
  const statusDisplay = document.getElementById('status-display');

  // WebSocket对象
  let socket = null;

  // 连接按钮点击事件
  connectBtn.addEventListener('click', () => {
    const ip = serverIpInput.value.trim();
    const port = serverPortInput.value.trim();

    if (!ip || !port) {
        statusDisplay.textContent = "请输入服务器IP和端口";
        return;
    }

    const serverAddress = `ws://${ip}:${port}`;
    statusDisplay.textContent = `正在连接到 ${serverAddress}...`;

    // 创建WebSocket连接
    try {
      socket = new WebSocket(serverAddress);

      // 连接打开事件
      socket.addEventListener('open', (event) => {
        console.log('WebSocket 连接已打开');
        statusDisplay.textContent = `已连接到 ${ip}:${port}`;

        // 更新按钮状态
        connectBtn.disabled = true;
        disconnectBtn.disabled = false;
        messageInput.disabled = false;
        sendBtn.disabled = false;
        serverIpInput.disabled = true; // 连接后禁止修改IP和端口
        serverPortInput.disabled = true;
      });

      // 接收消息事件
      socket.addEventListener('message', (event) => {
        console.log('收到消息:', event.data);
        appendMessage('server-message', `服务器: ${event.data}`);
      });

      // 连接关闭事件
      socket.addEventListener('close', (event) => {
        console.log('WebSocket 连接已关闭:', event);
        handleDisconnect(`连接已关闭 (代码: ${event.code})`);
      });

      // 连接错误事件
      socket.addEventListener('error', (event) => {
        console.error('WebSocket 错误:', event);
        handleDisconnect('连接错误!');
      });

    } catch (error) {
      console.error('创建WebSocket连接失败:', error);
      statusDisplay.textContent = `连接失败: ${error.message}`;
    }
  });

  // 断开连接按钮点击事件
  disconnectBtn.addEventListener('click', () => {
    if (socket) {
      console.log('手动断开连接...');
      socket.close();
    }
  });

  // 发送消息按钮点击事件
  sendBtn.addEventListener('click', sendMessage);
  messageInput.addEventListener('keypress', (event) => {
    // 按下 Enter 键发送消息
    if (event.key === 'Enter' && !event.shiftKey) { // Shift+Enter 换行
        event.preventDefault(); // 阻止默认的 Enter 换行行为
        sendMessage();
    }
  });

  function sendMessage() {
    // 检查连接状态和输入内容
    if (!socket || socket.readyState !== WebSocket.OPEN || !messageInput.value.trim()) {
        console.log("无法发送消息：连接未打开或消息为空");
        return;
    }

    const message = messageInput.value.trim();

    // 发送消息
    console.log('发送消息:', message);
    socket.send(message);

    // 在消息区显示自己的消息
    appendMessage('my-message', `我: ${message}`);

    // 清空输入框
    messageInput.value = '';
  }

  // 处理断开连接的逻辑
  function handleDisconnect(reason = '已断开连接') {
    statusDisplay.textContent = reason;

    // 更新按钮状态
    connectBtn.disabled = false;
    disconnectBtn.disabled = true;
    messageInput.disabled = true;
    sendBtn.disabled = true;
    serverIpInput.disabled = false; // 允许重新输入IP和端口
    serverPortInput.disabled = false;

    // 清除socket对象
    socket = null;
  }

  // 辅助函数：向消息区域添加消息
  function appendMessage(className, text) {
      const messageDiv = document.createElement('div');
      messageDiv.className = className;
      messageDiv.textContent = text;
      messageArea.appendChild(messageDiv);
      // 滚动到底部
      messageArea.scrollTop = messageArea.scrollHeight;
  }

});