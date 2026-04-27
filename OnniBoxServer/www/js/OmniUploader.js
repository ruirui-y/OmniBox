/**
 * OmniBox 核心上传引擎
 */
class OmniUploader {
    // 👈 1. 构造函数新增 parentId 参数，默认挂载到根目录(0)
    constructor(fileInputId, uploadBtnId, statusTextId, progressBarId, parentId = 0) {
        this.fileInput = document.getElementById(fileInputId);
        this.uploadBtn = document.getElementById(uploadBtnId);
        this.statusText = document.getElementById(statusTextId);
        this.progressBar = document.getElementById(progressBarId);

        // 绑定新的性能面板 UI
        this.timeText = document.getElementById('timeText');
        this.speedText = document.getElementById('speedText');

        // 切片大小：暂定 1MB，求稳
        this.CHUNK_SIZE = 1024 * 1024;

        // 性能侦测与寻址变量
        this.startTime = 0;
        this.uploadedBytes = 0;

        // 👈 2. 把当前所处的文件夹 ID 存入引擎内部
        this.parentId = parentId;
    }

    async start() {
        if (this.fileInput.files.length === 0) {
            alert("⚠️ 请先在弹药库中选择文件！"); return;
        }
        const file = this.fileInput.files[0];
        this.uploadBtn.disabled = true;
        this.uploadBtn.style.background = "#555";
        this.uploadedBytes = 0;

        this.startTime = Date.now();
        this.timeText.innerText = "⏱️ 已耗时: 0.0s";
        this.speedText.innerText = "🚀 速率: 计算中...";

        try {
            // ==========================================
            // 🎬 阶段一：前端暗流涌动 (提取秒传特征)
            // ==========================================
            this.speedText.innerText = "🧠 状态: 疯狂计算量子哈希中...";
            const fileHash = await this.calculateRapidHash(file);
            console.log(`[极客嗅探] 寻址目录: ${this.parentId}, 哈希: ${fileHash}`);

            // ==========================================
            // 🎬 阶段二：网关查岗 (秒传判定)
            // ==========================================
            this.speedText.innerText = "📡 状态: 向网关查岗中...";
            const isExist = await this.checkFastUpload(fileHash, file.name);

            if (isExist) {
                // 🏆 分支 A：秒传大满贯！
                this.uploadedBytes = file.size;
                this.updateProgress(file.size);
                const costTime = ((Date.now() - this.startTime) / 1000).toFixed(2);
                this.timeText.innerText = `⏱️ 秒传耗时: ${costTime}s`;
                this.speedText.innerText = "⚡ 速率: 光速 (命中秒传)";
                this.onSuccess();
                return;
            }

            // ==========================================
            // 🎬 阶段三：硬核苦力 (切片并发火力覆盖)
            // ==========================================
            this.speedText.innerText = "🚀 状态: 准备切片火力覆盖...";
            await this.uploadConcurrently(file, fileHash);
            this.onSuccess();
        } catch (error) {
            this.onError(error.message);
        }
    }

    async calculateRapidHash(file) {
        const SAMPLE_SIZE = 256 * 1024;
        let chunks = [];

        chunks.push(file.slice(0, SAMPLE_SIZE));

        if (file.size > SAMPLE_SIZE * 2) {
            const mid = Math.floor(file.size / 2);
            chunks.push(file.slice(mid, mid + SAMPLE_SIZE));
        }

        if (file.size > SAMPLE_SIZE) {
            chunks.push(file.slice(file.size - SAMPLE_SIZE, file.size));
        }

        const buffers = await Promise.all(chunks.map(c => c.arrayBuffer()));

        const totalLength = buffers.reduce((acc, buf) => acc + buf.byteLength, 0);
        const combinedBuffer = new Uint8Array(totalLength);
        let offset = 0;
        for (const buf of buffers) {
            combinedBuffer.set(new Uint8Array(buf), offset);
            offset += buf.byteLength;
        }

        const hashBuffer = await crypto.subtle.digest('SHA-256', combinedBuffer);
        const hashArray = Array.from(new Uint8Array(hashBuffer));
        const hashHex = hashArray.map(b => b.toString(16).padStart(2, '0')).join('');

        return `${file.size}_${hashHex}`;
    }

    // 📡 网关查岗请求
    async checkFastUpload(fileHash, fileName) {
        try {
            // 👈 3. 极其重要：在秒传查岗的 URL 里加上 parent_id。
            // 这样后端的 HandleCheckFile 在执行 INSERT "幻象建房" 时，就知道往哪个文件夹下挂载了！
            const url = `/check_file?hash=${fileHash}&name=${encodeURIComponent(fileName)}&parent_id=${this.parentId}`;

            const response = await fetch(url);
            if (!response.ok) return false;

            const result = await response.json();
            return result.status === "exists";
        } catch (e) {
            console.warn("网关查岗失败，退化为正常上传", e);
            return false;
        }
    }

    async uploadConcurrently(file, fileHash) {
        let offset = 0;
        let chunkIndex = 0;
        const MAX_CONCURRENT = 4;
        let activePromises = [];

        while (offset < file.size) {
            const currentOffset = offset;
            const currentChunkIndex = chunkIndex;
            const chunk = file.slice(currentOffset, currentOffset + this.CHUNK_SIZE);
            const isEof = (currentOffset + chunk.size >= file.size);

            offset += this.CHUNK_SIZE;
            chunkIndex++;

            const task = (async () => {
                const chunkBuffer = await chunk.arrayBuffer();
                const hashBuffer = await crypto.subtle.digest('SHA-256', chunkBuffer);
                const hashArray = Array.from(new Uint8Array(hashBuffer));
                const blockHash = hashArray.map(b => b.toString(16).padStart(2, '0')).join('');

                await this.shootChunk(
                    fileHash,
                    blockHash,
                    currentChunkIndex,
                    file.name,
                    currentOffset,
                    isEof,
                    chunk,
                    file.size
                );

                this.uploadedBytes += chunk.size;
                this.updateProgress(file.size);
            })();

            const p = task.then(() => {
                activePromises.splice(activePromises.indexOf(p), 1);
            });

            activePromises.push(p);

            if (activePromises.length >= MAX_CONCURRENT) {
                await Promise.race(activePromises);
            }
        }

        await Promise.all(activePromises);
    }

    // 独立的发包函数
    async shootChunk(fileHash, blockHash, chunkIndex, fileName, offset, isEof, chunkData, fileSize) {
        const headers = {
            'Content-Type': 'application/octet-stream',

            // 逻辑层信息 
            'X-File-Hash': fileHash,
            'X-File-Name': encodeURIComponent(fileName),
            'X-File-Size': fileSize.toString(),
            'X-Parent-Id': this.parentId.toString(),            // 👈 4. 新增：向服务器明示此文件所属的父目录
            'X-Chunk-Index': chunkIndex.toString(),
            'X-File-Eof': isEof ? '1' : '0',

            // 物理层信息 
            'X-Block-Hash': blockHash,
            'X-File-Offset': offset.toString()
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

    updateProgress(totalSize) {
        const currentMB = (this.uploadedBytes / 1024 / 1024).toFixed(2);
        const totalMB = (totalSize / 1024 / 1024).toFixed(2);
        const percent = Math.min(100, (this.uploadedBytes / totalSize) * 100);

        this.statusText.innerText = `正在穿透... ${currentMB} MB / ${totalMB} MB (${percent.toFixed(1)}%)`;
        this.statusText.style.color = "#ffaa00";
        this.progressBar.style.width = percent + '%';

        const now = Date.now();
        const elapsedSeconds = (now - this.startTime) / 1000;

        if (elapsedSeconds > 0.1) {
            this.timeText.innerText = `⏱️ 已耗时: ${elapsedSeconds.toFixed(1)}s`;
            const uploadedMB = this.uploadedBytes / 1024 / 1024;
            const speedMBps = uploadedMB / elapsedSeconds;
            this.speedText.innerText = `🚀 速率: ${speedMBps.toFixed(2)} MB/s`;
        }
    }

    onSuccess() {
        this.statusText.innerText = "✅ 传送大满贯！碎片已在服务器完成拼装。";
        this.statusText.style.color = "#00ffcc";

        this.progressBar.style.width = '100%';

        const finalSeconds = (Date.now() - this.startTime) / 1000;
        this.timeText.innerText = `⏱️ 总耗时: ${finalSeconds.toFixed(1)}s`;

        if (finalSeconds > 0) {
            const finalSpeedMBps = (this.uploadedBytes / 1024 / 1024) / finalSeconds;
            this.speedText.innerText = `🚀 平均速率: ${finalSpeedMBps.toFixed(2)} MB/s`;
        }

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