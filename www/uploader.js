/**
 * OmniBox 核心上传引擎
 * 负责大文件的物理切片与流式透传
 */


class OmniUploader
{
    constructor(fileInputId, uploadBtnId, statusTextId, progressBarId)
    {
        this.fileInput = document.getElementById(fileInputId);
        this.uploadBtn = document.getElementById(uploadBtnId);
        this.statusText = document.getElementById(statusTextId);
        this.progressBar = document.getElementById(progressBarId);

        // 1MB
        this.CHUNK_SIZE = 1024 * 1024;
    }

    async start() {
        if (this.fileInput.files.length === 0) {
            alert("请选择文件");
            return;
        }

        const file = this.fileInput.files[0];
        let offset = 0;

        // 锁定UI，防止重复点击
        this.uploadBtn.disabled = true;
        this.uploadBtn.style.background = "#555";
        this.uploadBtn.innerText = "🚀 传送中...";

        try {
            while (offset < file.size) {
                // 1. 物理切片
                const chunk = file.slice(offset, offset + this.CHUNK_SIZE);
                const isEof = (offset + chunk.size >= file.size);

                // 2. 更新进度条
                this.updateProgress(offset, chunk.size, file.size);

                // 3. 发包
                await this.shootChunk(file.name, offset, isEof, chunk);

                // 4. 游标推进
                offset += chunk.size;
            }

            this.onSuccess();
        }
        catch (error) {
            this.onError(error.message);
        }

        finally {
            // 解锁 UI
            this.uploadBtn.disabled = false;
            this.uploadBtn.style.background = "#00ffcc";
            this.uploadBtn.innerText = "🔥 启动文件传送";
            this.fileInput.value = "";                                                              // 清空选择
        }
    }

    // 发包
    async shootChunk(fileName, offset, isEof, chunkData)
    {
        const headers =
        {
            'Content-Type': 'application/octet-stream',
            // 使用 encodeURIComponent 防止中文文件名在 HTTP 头中变成乱码
            'X-File-Name': encodeURIComponent(fileName),
            'X-File-Offset': offset.toString(),
            'X-File-Eof': isEof ? '1' : '0'
        };

        const response = await fetch('/upload_chunk', {
            method: 'POST',
            headers: headers,
            body: chunkData
        });

        if (!response.ok)
        {
            throw new Error(`Gateway 拒绝接收，状态码: ${response.status}`);
        }
    }

    // UI 刷新辅助方法
    updateProgress(offset, chunkSize, totalSize)
    {
        const currentMB = (offset / 1024 / 1024).toFixed(2);
        const totalMB = (totalSize / 1024 / 1024).toFixed(2);
        const percent = Math.min(100, ((offset + chunkSize) / totalSize) * 100);

        this.statusText.innerText = `正在传输... ${currentMB} MB / ${totalMB} MB (${percent.toFixed(1)}%)`;
        this.statusText.style.color = "#ffaa00";
        this.progressBar.style.width = percent + '%';
    }

    onSuccess()
    {
        this.statusText.innerText = "传送完成";
        this.statusText.style.color = "#00ffcc";
    }

    onError(msg) {
        this.statusText.innerText = `❌ 链路断开: ${msg}`;
        this.statusText.style.color = "red";
    }
}
