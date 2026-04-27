// /www/js/app.js

// 1. 实例化文件系统管理器
const fsManager = new FileSystemManager(
    'fileListContainer',
    'currentPathLabel',
    'uploadTargetLabel'
);

// 2. 绑定“新建目录”
document.getElementById('btnNewFolder').onclick = () => {
    fsManager.createNewFolder();
};

// 3. 绑定“启动文件传送”
document.getElementById('btnStartUpload').onclick = () => {
    const currentParentId = fsManager.currentFolderId;
    const uploader = new OmniUploader(
        'fileInput',
        'btnStartUpload',
        'statusText',
        'progressBar',
        currentParentId
    );
    uploader.start();
};

// 4. 👑 启动鉴权管理器
window.onload = () => {
    // 实例化 AuthManager，并传入一个箭头函数
    // 当鉴权通过时，AuthManager 会自动调用这个函数，点亮整个文件系统
    const authManager = new AuthManager(() => {
        console.log("鉴权通过，正在加载文件系统树...");
        fsManager.init();
    });
};