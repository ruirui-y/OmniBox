// /www/js/AuthManager.js

class AuthManager {
    constructor(onAuthSuccess) {
        // 外部传入的成功回调函数，登录成功后执行
        this.onAuthSuccess = onAuthSuccess;

        // 绑定事件
        this.bindEvents();
        this.interceptPageLoad();
    }

    bindEvents() {
        document.getElementById('btnLogin').onclick = () => this.handleLogin();
    }

    interceptPageLoad() {
        // 页面一加载，拦截显示并强制呈现登录框
        document.getElementById('loginOverlay').style.display = 'flex';
        document.getElementById('mainApp').style.display = 'none';
    }

    startHeartbeatLoop() {
        const uid = localStorage.getItem('omni_uid');
        const token = localStorage.getItem('omni_token');

        // 每隔 10 秒发一次心跳
        this.heartbeatTimer = setInterval(async () => {
            try {
                const response = await fetch('/api/heartbeat', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ user_id: parseInt(uid), token: token })
                });
                const result = await response.json();
                if (!result.success) {
                    console.error("心跳上报失败，可能登录已过期");
                    // 可以视情况跳转回登录页
                }
            } catch (e) {
                console.error("网络异常，心跳中断");
            }
        }, 10000); // 10s 频率
    }

    async handleLogin() {
        const usernameInput = document.getElementById('loginUsername').value;
        const passwordInput = document.getElementById('loginPassword').value;
        const statusEl = document.getElementById('loginStatus');

        if (!usernameInput || !passwordInput) {
            statusEl.innerText = "[!] 缺乏认证凭据";
            statusEl.className = "login-status error";
            return;
        }

        statusEl.innerText = "VERIFYING CREDENTIALS...";
        statusEl.className = "login-status";

        try {
            const response = await fetch('/api/login', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    username: usernameInput,
                    password: passwordInput
                })
            });

            const result = await response.json();

            if (result.errcode === 0) {
                statusEl.innerText = "ACCESS GRANTED";
                statusEl.style.color = "#33ff33";

                // 保存令牌
                localStorage.setItem('omni_token', result.token);
                localStorage.setItem('omni_uid', result.user_id);

                // 极客延迟感，切换 UI 并唤醒主系统
                setTimeout(() => {
                    document.getElementById('loginOverlay').style.display = 'none';
                    document.getElementById('mainApp').style.display = 'block';
                    this.startHeartbeatLoop();                                                           // 登录成功后，启动心跳

                    // 👑 核心：触发外部传进来的成功回调
                    if (this.onAuthSuccess)
                    {
                        this.onAuthSuccess();
                    }
                }, 500);
            } else {
                statusEl.innerText = "[!] " + result.errmsg;
                statusEl.className = "login-status error";
            }
        } catch (e) {
            statusEl.innerText = "[!] 网关通信故障";
            statusEl.className = "login-status error";
        }
    }
}