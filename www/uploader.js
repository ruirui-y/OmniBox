/**
 * OmniBox 核心上传引擎 (ES6 Class) - 带性能侦测雷达版
 */
class OmniUploader {
    constructor(fileInputId, uploadBtnId, statusTextId, progressBarId) {
        this.fileInput = document.getElementById(fileInputId);
        this.uploadBtn = document.getElementById(uploadBtnId);
        this.statusText = document.getElementById(statusTextId);
        this.progressBar = document.getElementById(progressBarId);

        // 绑定新的性能面板 UI
        this.timeText = document.getElementById('timeText');
        this.speedText = document.getElementById('speedText');

        // 切片大小：暂定 1MB，求稳
        this.CHUNK_SIZE = 1024 * 1024;

        // 性能侦测变量
        this.startTime = 0;
    }

    async start() {
        if (this.fileInput.files.length === 0) {
            alert("⚠️ 请先在弹药库中选择文件！"); return;
        }
        const file = this.fileInput.files[0];
        this.uploadBtn.disabled = true;
        this.uploadBtn.style.background = "#555";

        this.startTime = Date.now();
        this.timeText.innerText = "⏱️ 已耗时: 0.0s";
        this.speedText.innerText = "🚀 速率: 计算中...";

        // 🚀 启动滑动窗口并发引擎！
        try {
            await this.uploadConcurrently(file);
            this.onSuccess();
        } catch (error) {
            this.onError(error.message);
        }
    }

    async uploadConcurrently(file) {
        let offset = 0;
        const MAX_CONCURRENT = 4; // 🚀 工业级并发度：保持 4 个切片同时在天上飞！
        let activePromises = [];

        while (offset < file.size) {
            const currentOffset = offset;
            const chunk = file.slice(currentOffset, currentOffset + this.CHUNK_SIZE);
            const isEof = (currentOffset + chunk.size >= file.size);

            offset += this.CHUNK_SIZE; // 游标继续无情推进

            // 将发包动作封装成 Promise
            const p = this.shootChunk(file.name, currentOffset, isEof, chunk).then(() => {
                // 收到 200 OK 后，更新进度条，并从活跃池中移除自己
                this.updateProgress(currentOffset, chunk.size, file.size);
                activePromises.splice(activePromises.indexOf(p), 1);
            });

            activePromises.push(p);

            // 如果当前在天上飞的切片达到了上限 (4个)，就等！
            // Promise.race 会等待最快落地的那个切片，只要腾出 1 个空位，循环立刻继续，补充弹药！
            if (activePromises.length >= MAX_CONCURRENT) {
                await Promise.race(activePromises);
            }
        }

        // 等待最后几块肉丁彻底落地
        await Promise.all(activePromises);
    }

    // 独立的发包函数 (剥离了进度刷新和重试逻辑，变得极其纯粹)
    async shootChunk(fileName, offset, isEof, chunkData) {
        const headers = {
            'Content-Type': 'application/octet-stream',
            'X-File-Name': encodeURIComponent(fileName),
            'X-File-Offset': offset.toString(),
            'X-File-Eof': isEof ? '1' : '0'
        };

        const response = await fetch('/upload_chunk', {
            method: 'POST',
            headers: headers,
            body: chunkData
        });

        if (!response.ok) {
            throw new Error(`Gateway HTTP 异常: ${response.status}`);
        }
    }

    // 核心 UI 刷新辅助方法 (包含速率计算)
    updateProgress(offset, chunkSize, totalSize) {
        // 1. 进度条计算
        const currentMB = (offset / 1024 / 1024).toFixed(2);
        const totalMB = (totalSize / 1024 / 1024).toFixed(2);
        const percent = Math.min(100, ((offset + chunkSize) / totalSize) * 100);

        this.statusText.innerText = `正在穿透... ${currentMB} MB / ${totalMB} MB (${percent.toFixed(1)}%)`;
        this.statusText.style.color = "#ffaa00";
        this.progressBar.style.width = percent + '%';

        // 2. 性能侦测计算 (耗时与速度)
        const now = Date.now();
        const elapsedSeconds = (now - this.startTime) / 1000; // 换算成秒

        if (elapsedSeconds > 0.1) {
            this.timeText.innerText = `⏱️ 已耗时: ${elapsedSeconds.toFixed(1)}s`;

            // 速度 = 已发送的 MB 数 / 已耗费的秒数
            const uploadedMB = (offset + chunkSize) / 1024 / 1024;
            const speedMBps = uploadedMB / elapsedSeconds;
            this.speedText.innerText = `🚀 速率: ${speedMBps.toFixed(2)} MB/s`;
        }
    }

    onSuccess() {
        this.statusText.innerText = "✅ 传送大满贯！碎片已在服务器完成拼装。";
        this.statusText.style.color = "#00ffcc";

        // 最终耗时定格
        const finalSeconds = (Date.now() - this.startTime) / 1000;
        this.timeText.innerText = `⏱️ 总耗时: ${finalSeconds.toFixed(1)}s`;

        // 恢复按钮状态
        this.uploadBtn.disabled = false;
        this.uploadBtn.style.background = "#00ffcc";
        this.uploadBtn.innerText = "🔥 启动量子传送";
        this.fileInput.value = "";
    }

    onError(msg) {
        this.statusText.innerText = `❌ 链路断开: ${msg}`;
        this.statusText.style.color = "red";
    }
}