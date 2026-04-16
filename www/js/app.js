// /www/js/app.js

// 1. 实例化文件系统管理器
const fsManager = new FileSystemManager(
    'fileListContainer',
    'currentPathLabel',
    'uploadTargetLabel'
);

// 2. 绑定“新建目录”按钮事件
document.getElementById('btnNewFolder').onclick = () => {
    fsManager.createNewFolder();
};

// 3. 绑定“启动文件传送”按钮事件，实现核心联通！
document.getElementById('btnStartUpload').onclick = () => {

    // 获取当前文件系统的目录 ID (这就是 C++ 数据库需要的 parent_id)
    const currentParentId = fsManager.currentFolderId;

    // 实例化极速上传器，把 parent_id 传进去！
    // (注意：你需要稍微改一下 OmniUploader 的构造函数，接收 parentId 并在发送 Header 时带上它)
    const uploader = new OmniUploader(
        'fileInput',
        'btnStartUpload',
        'statusText',
        'progressBar',
        currentParentId // 👈 架构联通点
    );

    uploader.start();
};

// 4. 页面加载完毕后，启动文件系统
window.onload = () => {
    fsManager.init();
};