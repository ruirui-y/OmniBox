// /www/js/FileSystemManager.js

class FileSystemManager {
    constructor(listContainerId, pathLabelId, targetLabelId) {
        this.listContainer = document.getElementById(listContainerId);
        this.pathLabel = document.getElementById(pathLabelId);
        this.targetLabel = document.getElementById(targetLabelId);

        this.currentFolderId = 0;
        this.currentPath = "/root/";
        this.folderHistory = [];
    }

    async init() {
        await this.loadDirectory(0, "/root/");
    }

    // ==========================================
    // 1. 拉取目录列表 (List)
    // ==========================================
    async loadDirectory(folderId, pathName) {
        this.currentFolderId = folderId;
        this.currentPath = pathName;

        this.pathLabel.innerText = this.currentPath;
        this.targetLabel.innerText = this.currentPath;

        this.listContainer.innerHTML = `<li style="text-align: center; color: #555;">[ 扫描目录中... ]</li>`;

        try {
            // 真正发起 HTTP GET 请求到你的 C++ 网关
            const response = await fetch(`/api/list_dir?parent_id=${folderId}`);
            if (!response.ok) throw new Error("网络层异常");

            const data = await response.json();

            if (data.success) {
                this.renderList(data.nodes); // 传入从数据库拿到的真实节点列表
            } else {
                throw new Error(data.message);
            }
        } catch (error) {
            console.warn("后端未就绪，使用模拟数据演示 UI");
            this.renderList(this.getMockData(folderId)); // 容错：如果你后端还没写好，降级到假数据让你看效果
        }
    }

    // 渲染 DOM 列表 (动态生成 UI，相当于 Qt 里的 new QWidget)
    renderList(files) {
        this.listContainer.innerHTML = '';

        if (this.currentFolderId !== 0) {
            const backLi = document.createElement('li');
            backLi.innerHTML = `<span><span class="icon-dir">📁</span>.. (返回上级)</span>`;
            backLi.onclick = () => this.goBack();
            this.listContainer.appendChild(backLi);
        }

        if (!files || files.length === 0) {
            this.listContainer.innerHTML += `<li style="text-align: center; color: #555;">[ 空目录 ]</li>`;
            return;
        }

        files.forEach(item => {
            const li = document.createElement('li');

            // 构造左侧的名称和图标
            let leftContent = '';
            if (item.is_dir) {
                leftContent = `<span style="cursor:pointer;" onclick="fsManager.enterFolder(${item.node_id}, '${item.node_name}')"><span class="icon-dir">📁</span>${item.node_name}</span>`;
            } else {
                leftContent = `<span><span class="icon-file">📄</span>${item.node_name}</span>`;
            }

            // 构造右侧的元数据和操作按钮！(极其硬核)
            const rightContent = `
                <div style="display: flex; align-items: center; gap: 10px;">
                    <span class="file-meta">${item.is_dir ? '目录' : this.formatSize(item.file_size)} | ${item.update_time}</span>
                    <button style="padding: 2px 5px; font-size: 10px; background: #333; color: #fff;" onclick="fsManager.renameNode(${item.node_id}, '${item.node_name}')">[改名]</button>
                    <button style="padding: 2px 5px; font-size: 10px; background: #550000; color: #ff4444;" onclick="fsManager.deleteNode(${item.node_id}, '${item.node_name}')">[删除]</button>
                </div>
            `;

            li.innerHTML = leftContent + rightContent;
            this.listContainer.appendChild(li);
        });
    }

    // 进入子目录 (抽离出来的辅助函数)
    enterFolder(folderId, folderName) {
        this.folderHistory.push({ id: this.currentFolderId, path: this.currentPath });
        this.loadDirectory(folderId, this.currentPath + folderName + "/");
    }

    goBack() {
        if (this.folderHistory.length > 0) {
            const prevFolder = this.folderHistory.pop();
            this.loadDirectory(prevFolder.id, prevFolder.path);
        }
    }

    // ==========================================
    // 2. 新建目录 (Create)
    // ==========================================
    async createNewFolder() {
        const folderName = prompt("请输入新文件夹名称:");
        if (!folderName) return;

        try {
            const response = await fetch('/api/create_folder', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ parent_id: this.currentFolderId, folder_name: folderName })
            });
            const result = await response.json();
            if (result.success) {
                this.loadDirectory(this.currentFolderId, this.currentPath); // 成功后刷新当前目录
            } else {
                alert("创建失败: " + result.message);
            }
        } catch (e) {
            console.log("模拟新建目录:", folderName);
        }
    }

    // ==========================================
    // 3. 重命名节点 (Rename)
    // ==========================================
    async renameNode(nodeId, oldName) {
        const newName = prompt(`将 [${oldName}] 重命名为:`, oldName);
        if (!newName || newName === oldName) return;

        try {
            const response = await fetch('/api/rename_node', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ node_id: nodeId, new_name: newName })
            });
            const result = await response.json();
            if (result.success) {
                this.loadDirectory(this.currentFolderId, this.currentPath); // 刷新
            } else {
                alert("重命名失败: " + result.message);
            }
        } catch (e) {
            console.log(`模拟重命名 ID:${nodeId} 为 ${newName}`);
        }
    }

    // ==========================================
    // 4. 删除节点 (Delete)
    // ==========================================
    async deleteNode(nodeId, nodeName) {
        // 危险操作，弹窗二次确认 (相当于 QMessageBox::warning)
        if (!confirm(`⚠️ 警告：确定要删除 [${nodeName}] 吗？`)) return;

        try {
            const response = await fetch('/api/delete_node', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ node_id: nodeId })
            });
            const result = await response.json();
            if (result.success) {
                this.loadDirectory(this.currentFolderId, this.currentPath); // 刷新
            } else {
                alert("删除失败: " + result.message);
            }
        } catch (e) {
            console.log(`模拟删除 ID:${nodeId}`);
        }
    }

    // 辅助工具：字节转换
    formatSize(bytes) {
        if (bytes === 0) return '0 B';
        const k = 1024, sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    // 模拟数据 (确保数据结构与你的 protobuf NodeInfo 完全一致)
    getMockData(folderId) {
        if (folderId === 0) {
            return [
                { node_id: 101, node_name: "project_omega", is_dir: true, file_size: 0, update_time: "2026-04-16" },
                { node_id: 102, node_name: "system_core.bin", is_dir: false, file_size: 1288490188, update_time: "2026-04-15" }
            ];
        }
        return [];
    }
}