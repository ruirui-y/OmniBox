// /www/js/FileSystemManager.js

class FileSystemManager {
    constructor(listContainerId, pathLabelId, targetLabelId) {
        // DOM 节点绑定
        this.listContainer = document.getElementById(listContainerId);
        this.pathLabel = document.getElementById(pathLabelId);
        this.targetLabel = document.getElementById(targetLabelId); // 右侧上传面板的路径指示器

        // 内部状态
        this.currentFolderId = 0; // 0 表示根目录
        this.currentPath = "/root/";
        this.folderHistory = [];  // 用于实现“返回上级”功能
    }

    // 初始化：加载根目录
    async init() {
        await this.loadDirectory(0, "/root/");
    }

    // 核心网络请求：加载指定目录
    async loadDirectory(folderId, pathName) {
        this.currentFolderId = folderId;
        this.currentPath = pathName;

        // 更新 UI 上的路径显示
        this.pathLabel.innerText = this.currentPath;
        this.targetLabel.innerText = this.currentPath;

        this.listContainer.innerHTML = `<li style="text-align: center; color: #555;">[ 扫描目录中... ]</li>`;

        try {
            // TODO: 这里将来对接你的 C++ 接口获取文件列表
            // const response = await fetch(`/api/list_files?parent_id=${folderId}`);
            // const data = await response.json();

            // 【模拟数据】为了前端能跑起来，我们假装收到了后端的数据
            const mockData = this.getMockData(folderId);
            this.renderList(mockData);

        } catch (error) {
            this.listContainer.innerHTML = `<li style="color: red;">[ 目录扫描失败: ${error.message} ]</li>`;
        }
    }

    // 渲染 DOM 列表
    renderList(files) {
        this.listContainer.innerHTML = ''; // 清空列表

        // 1. 如果不是根目录，永远在顶部加上“返回上级”
        if (this.currentFolderId !== 0) {
            const backLi = document.createElement('li');
            backLi.innerHTML = `<span><span class="icon-dir">📁</span>.. (返回上级)</span>`;
            backLi.onclick = () => this.goBack();
            this.listContainer.appendChild(backLi);
        }

        // 2. 渲染空目录
        if (files.length === 0) {
            this.listContainer.innerHTML += `<li style="text-align: center; color: #555;">[ 空目录 ]</li>`;
            return;
        }

        // 3. 渲染真实文件和文件夹
        files.forEach(item => {
            const li = document.createElement('li');

            if (item.is_dir) {
                li.innerHTML = `
                    <span><span class="icon-dir">📁</span>${item.name}</span>
                    <span class="file-meta">目录 | ${item.date}</span>
                `;
                // 点击文件夹：进入下一级
                li.onclick = () => {
                    this.folderHistory.push({ id: this.currentFolderId, path: this.currentPath });
                    this.loadDirectory(item.id, this.currentPath + item.name + "/");
                };
            } else {
                li.innerHTML = `
                    <span><span class="icon-file">📄</span>${item.name}</span>
                    <span class="file-meta">${this.formatSize(item.size)} | ${item.date}</span>
                `;
                // 点击文件：可以触发下载或其他操作
                li.onclick = () => alert(`准备下载文件: ${item.name}`);
            }

            this.listContainer.appendChild(li);
        });
    }

    // 返回上级逻辑
    goBack() {
        if (this.folderHistory.length > 0) {
            const prevFolder = this.folderHistory.pop();
            this.loadDirectory(prevFolder.id, prevFolder.path);
        }
    }

    // 新建文件夹功能 (留出接口)
    createNewFolder() {
        const folderName = prompt("请输入新文件夹名称:");
        if (folderName) {
            console.log(`向后端发送创建目录指令: 所在父级[${this.currentFolderId}], 名称[${folderName}]`);
            // TODO: 调用 C++ 后端 API 创建文件夹，然后重新 loadDirectory()
        }
    }

    // 工具函数：字节转换
    formatSize(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024, sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    // 【模拟后端数据】
    getMockData(folderId) {
        if (folderId === 0) {
            return [
                { id: 101, name: "project_omega", is_dir: true, size: 0, date: "2026-04-16" },
                { id: 102, name: "system_core.bin", is_dir: false, size: 1288490188, date: "2026-04-15" }
            ];
        } else if (folderId === 101) {
            return [
                { id: 201, name: "secret_keys.txt", is_dir: false, size: 1024, date: "2026-04-16" }
            ];
        }
        return [];
    }
}