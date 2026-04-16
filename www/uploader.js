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

        this.uploadedBytes = 0; // 👈 核心：新增累加器
    }

    async start() {
        if (this.fileInput.files.length === 0) {
            alert("⚠️ 请先在弹药库中选择文件！"); return;
        }
        const file = this.fileInput.files[0];
        this.uploadBtn.disabled = true;
        this.uploadBtn.style.background = "#555";
        this.uploadedBytes = 0; // 👈 每次点击上传，账本清零！

        this.startTime = Date.now();
        this.timeText.innerText = "⏱️ 已耗时: 0.0s";
        this.speedText.innerText = "🚀 速率: 计算中...";

        // 🚀 启动滑动窗口并发引擎！
        try {
            // ==========================================
            // 🎬 阶段一：前端暗流涌动 (提取秒传特征)
            // ==========================================
            this.speedText.innerText = "🧠 状态: 疯狂计算量子哈希中...";
            const fileHash = await this.calculateRapidHash(file);
            console.log(`[极客嗅探] 文件大小: ${file.size}, 急速哈希: ${fileHash}`);

            // ==========================================
            // 🎬 阶段二：网关查岗 (秒传判定)
            // ==========================================
            this.speedText.innerText = "📡 状态: 向网关查岗中...";
            const isExist = await this.checkFastUpload(fileHash, file.name);

            if (isExist) {
                // 🏆 分支 A：秒传大满贯！
                this.uploadedBytes = file.size; // 账本直接拉满
                this.updateProgress(file.size);
                const costTime = ((Date.now() - this.startTime) / 1000).toFixed(2);
                this.timeText.innerText = `⏱️ 秒传耗时: ${costTime}s`;
                this.speedText.innerText = "⚡ 速率: 光速 (命中秒传)";
                this.onSuccess();
                return; // 瞬间下班，网络层一字节都不发！
            }

            // ==========================================
            // 🎬 阶段三：硬核苦力 (切片并发火力覆盖)
            // ==========================================
            this.speedText.innerText = "🚀 状态: 准备切片火力覆盖...";
            // 记得把灵魂 (fileHash) 传进去，让服务器知道切片属于谁
            await this.uploadConcurrently(file, fileHash);
            this.onSuccess();
        } catch (error) {
            this.onError(error.message);
        }
    }

    // 🧠 核心黑科技：急速哈希算法 (大小 + 头256K + 中256K + 尾256K)
    async calculateRapidHash(file) {
        const SAMPLE_SIZE = 256 * 1024;                                                 // 256KB
        let chunks = [];

        // 1. 抽样头部
        chunks.push(file.slice(0, SAMPLE_SIZE));

        // 2. 抽样中间
        if (file.size > SAMPLE_SIZE * 2) {
            const mid = Math.floor(file.size / 2);
            chunks.push(file.slice(mid, mid + SAMPLE_SIZE));
        }

        // 3. 抽样尾部
        if (file.size > SAMPLE_SIZE) {
            chunks.push(file.slice(file.size - SAMPLE_SIZE, file.size));
        }

        // 4. 将所有抽样的切片读取为二进制缓冲
        const buffers = await Promise.all(chunks.map(c => c.arrayBuffer()));

        // 5. 拼接缓冲区
        const totalLength = buffers.reduce((acc, buf) => acc + buf.byteLength, 0);
        const combinedBuffer = new Uint8Array(totalLength);
        let offset = 0;
        for (const buf of buffers) {
            combinedBuffer.set(new Uint8Array(buf), offset);
            offset += buf.byteLength;
        }

        // 6. 调用浏览器底层原生 Crypto API 算 SHA-256 (极快，不卡 UI)
        const hashBuffer = await crypto.subtle.digest('SHA-256', combinedBuffer);
        const hashArray = Array.from(new Uint8Array(hashBuffer));
        const hashHex = hashArray.map(b => b.toString(16).padStart(2, '0')).join('');

        // 7. 终极防撞金牌：文件总大小 + 抽样哈希
        return `${file.size}_${hashHex}`;
    }

    // 📡 网关查岗请求
    async checkFastUpload(fileHash, fileName) {
        try {
            // 这里假设你的网关提供了一个 /check_file 的 GET 接口
            const response = await fetch(`/check_file?hash=${fileHash}&name=${encodeURIComponent(fileName)}`);
            if (!response.ok) return false;

            const result = await response.json();
            // 假设服务器返回 { "status": "exists" } 表示库里有
            return result.status === "exists";
        } catch (e) {
            console.warn("网关查岗失败，退化为正常上传", e);
            return false;
        }
    }

    async uploadConcurrently(file, fileHash)
    {
        let offset = 0;
        let chunkIndex = 0;                                                                         // 👈 极其重要：这就是你数据库 mapping 表里的 block_order
        const MAX_CONCURRENT = 4;
        let activePromises = [];

        while (offset < file.size) {
            const currentOffset = offset;
            const currentChunkIndex = chunkIndex;
            const chunk = file.slice(currentOffset, currentOffset + this.CHUNK_SIZE);
            const isEof = (currentOffset + chunk.size >= file.size);

            offset += this.CHUNK_SIZE;
            chunkIndex++;                                                                           // 序号递增

            // 【神级封装】：将“算哈希”和“发请求”打包成一个原子的异步任务
            const task = (async () => {
                // 1. 脑力劳动：光速计算这个 1MB 碎块的独立哈希
                const chunkBuffer = await chunk.arrayBuffer();
                const hashBuffer = await crypto.subtle.digest('SHA-256', chunkBuffer);
                const hashArray = Array.from(new Uint8Array(hashBuffer));
                const blockHash = hashArray.map(b => b.toString(16).padStart(2, '0')).join('');

                // 2. 体力劳动：带上双哈希身份证，发射炮弹！
                await this.shootChunk(
                    fileHash,
                    blockHash,                                                                      // 这块肉丁的专属身份证
                    currentChunkIndex,                                                              // 在总文件里的绝对排位
                    file.name,
                    currentOffset,
                    isEof,
                    chunk,
                    file.size
                );

                // 3. 记账刷新
                this.uploadedBytes += chunk.size;
                this.updateProgress(file.size);
            })();

            // 将这个原子任务丢进天上飞的队列
            const p = task.then(() => {
                activePromises.splice(activePromises.indexOf(p), 1);
            });

            activePromises.push(p);

            // 滑动窗口控制并发度
            if (activePromises.length >= MAX_CONCURRENT) {
                await Promise.race(activePromises);
            }
        }

        // 等待所有切片全部完成
        await Promise.all(activePromises);
    }

    // 独立的发包函数
    async shootChunk(fileHash, blockHash, chunkIndex, fileName, offset, isEof, chunkData, fileSize) {
        const headers = {
            'Content-Type': 'application/octet-stream',

            // 逻辑层信息 (用于网关更新虚拟文件树和 Mapping 表)
            'X-File-Hash': fileHash,
            'X-File-Name': encodeURIComponent(fileName),
            'X-File-Size': fileSize.toString(),
            'X-Chunk-Index': chunkIndex.toString(),                         //  数据库的 block_order
            'X-File-Eof': isEof ? '1' : '0',                                //  通知服务器：这是最后一块，可以结算大满贯了

            // 物理层信息 (用于 TransferServer 底层去重和落盘)
            'X-Block-Hash': blockHash,                                      // TransferServer 根据这个决定存不存这块肉丁
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

    // 核心 UI 刷新辅助方法 (包含速率计算)
    updateProgress(totalSize) {
        // 1. 进度条计算（完全依赖累加器，单调递增，绝不倒流！）
        const currentMB = (this.uploadedBytes / 1024 / 1024).toFixed(2);
        const totalMB = (totalSize / 1024 / 1024).toFixed(2);
        const percent = Math.min(100, (this.uploadedBytes / totalSize) * 100);

        this.statusText.innerText = `正在穿透... ${currentMB} MB / ${totalMB} MB (${percent.toFixed(1)}%)`;
        this.statusText.style.color = "#ffaa00";
        this.progressBar.style.width = percent + '%';

        // 2. 性能侦测计算
        const now = Date.now();
        const elapsedSeconds = (now - this.startTime) / 1000;

        if (elapsedSeconds > 0.1) {
            this.timeText.innerText = `⏱️ 已耗时: ${elapsedSeconds.toFixed(1)}s`;

            // 速度 = 真实的累加 MB 数 / 已耗费的秒数
            const uploadedMB = this.uploadedBytes / 1024 / 1024;
            const speedMBps = uploadedMB / elapsedSeconds;
            this.speedText.innerText = `🚀 速率: ${speedMBps.toFixed(2)} MB/s`;
        }
    }

    onSuccess() {
        this.statusText.innerText = "✅ 传送大满贯！碎片已在服务器完成拼装。";
        this.statusText.style.color = "#00ffcc";

        // 👑 1. 强行把进度条焊死在 100%！消除任何并发浮点数带来的微小缝隙
        this.progressBar.style.width = '100%';

        // 👑 2. 最终耗时定格
        const finalSeconds = (Date.now() - this.startTime) / 1000;
        this.timeText.innerText = `⏱️ 总耗时: ${finalSeconds.toFixed(1)}s`;

        // 👑 3. 最终平均速率定格 (用真实落地的总字节数 / 真实总耗时)
        if (finalSeconds > 0) {
            const finalSpeedMBps = (this.uploadedBytes / 1024 / 1024) / finalSeconds;
            this.speedText.innerText = `🚀 平均速率: ${finalSpeedMBps.toFixed(2)} MB/s`;
        }

        // 4. 恢复按钮状态，准备下一次传送
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