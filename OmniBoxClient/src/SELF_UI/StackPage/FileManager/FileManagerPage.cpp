#include "FileManagerPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QInputDialog>
#include <QDebug>
#include "ThreadPool.h"
#include "CinemaMessageBox.h"
#include "UserMgr.h"
#include "CinemaInputDialog.h"


FileManagerPage::FileManagerPage(QWidget* parent) : QWidget(parent)
, m_currentParentId(0), m_cutNodeId(-1)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("FileManagerPage");

    BuildUI();

    // ==========================================================
    // 👑 绑定网络响应槽函数
    // ==========================================================
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigListDirectory, this, &FileManagerPage::onListDirectoryRsp);
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigFolderCreated, this, &FileManagerPage::onCreateFolderRsp);
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigNodeDeleted, this, &FileManagerPage::onDeleteNodeRsp);
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigNodeRenamed, this, &FileManagerPage::onRenameNodeRsp);
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigNodeMoved, this, &FileManagerPage::onMoveNodeRsp);

    // 💡 启动时：默认拉取根目录 (parent_id = 0)
    RequestListDirectory(m_currentParentId);
}

void FileManagerPage::BuildUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // ================= 1. 顶部操作栏 =================
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->setSpacing(15);

    QPushButton* btnBack = new QPushButton(u8"⬅ 返回上级", this);
    btnBack->setObjectName("controlBtn");
    btnBack->setMinimumSize(100, 35);

    m_pathLabel = new QLabel(u8"当前位置: 根目录", this);
    m_pathLabel->setObjectName("pathLabel");

    m_btnPaste = new QPushButton(u8"📋 粘贴到此", this);
    m_btnPaste->setObjectName("controlBtn");
    m_btnPaste->setMinimumSize(100, 35);
    m_btnPaste->setVisible(false);

    QPushButton* btnNewFolder = new QPushButton(u8"📁 新建文件夹", this);
    btnNewFolder->setObjectName("controlBtn");
    btnNewFolder->setMinimumSize(120, 35);

    QPushButton* btnRefresh = new QPushButton(u8"🔄 刷新", this);
    btnRefresh->setObjectName("controlBtn");
    btnRefresh->setMinimumSize(90, 35);

    topLayout->addWidget(btnBack);
    topLayout->addWidget(m_pathLabel);
    topLayout->addStretch();
    topLayout->addWidget(m_btnPaste);
    topLayout->addWidget(btnNewFolder);
    topLayout->addWidget(btnRefresh);

    // ================= 2. 数据表格 =================
    m_fileTable = new QTableWidget(0, 4, this);
    m_fileTable->setObjectName("fileTable");
    m_fileTable->setHorizontalHeaderLabels({ u8"文件名", u8"类型", u8"大小", u8"修改时间" });

    // 列宽策略：文件名拉伸，其他固定适应内容
    m_fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_fileTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    m_fileTable->setAlternatingRowColors(true);
    m_fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止双击编辑
    m_fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fileTable->verticalHeader()->setVisible(false);
    m_fileTable->setFocusPolicy(Qt::NoFocus);

    // 👑 开启自定义右键菜单策略
    m_fileTable->setContextMenuPolicy(Qt::CustomContextMenu);

    layout->addLayout(topLayout);
    layout->addWidget(m_fileTable, 1);

    // ================= 3. 信号绑定 =================
    connect(btnBack, &QPushButton::clicked, this, &FileManagerPage::onBtnBackClicked);
    connect(btnNewFolder, &QPushButton::clicked, this, &FileManagerPage::onBtnNewFolderClicked);
    connect(m_btnPaste, &QPushButton::clicked, this, &FileManagerPage::onBtnPasteClicked);
    connect(btnRefresh, &QPushButton::clicked, this, &FileManagerPage::onBtnRefreshClicked);

    connect(m_fileTable, &QTableWidget::cellDoubleClicked, this, &FileManagerPage::onTableDoubleClicked);
    connect(m_fileTable, &QTableWidget::customContextMenuRequested, this, &FileManagerPage::onTableContextMenu);

    // ================= 4. 初始化右键菜单 =================
    m_contextMenu = new QMenu(this);
    m_contextMenu->setObjectName("fileContextMenu");
    m_contextMenu->addAction(u8"✂️ 剪切", this, &FileManagerPage::onActionCut);
    m_contextMenu->addAction(u8"✏️ 重命名", this, &FileManagerPage::onActionRename);
    m_contextMenu->addAction(u8"🗑️ 删除", this, &FileManagerPage::onActionDelete);
}

// -------------------------------------------------------------------------
// 🚀 [核心逻辑]：目录跳转与拉取
// -------------------------------------------------------------------------
void FileManagerPage::RequestListDirectory(int64_t parentId)
{
    m_fileTable->setRowCount(0); // 加载前清空

    omnibox::ListDirectoryRequest req;
    req.set_user_id(UserMgr::Instance()->GetId());
    req.set_parent_id(parentId);

    ThreadPool::Instance()->GetTCPMgr()->SendProtoMsg(omnibox::MsgId::ID_LIST_DIR_REQ, req);
}

void FileManagerPage::onListDirectoryRsp(const omnibox::ListDirectoryResponse& rsp)
{
    m_fileTable->setRowCount(0);
    for (int i = 0; i < rsp.nodes_size(); ++i)
    {
        const auto& node = rsp.nodes(i);
        InsertRowToUI(node.node_id(),
            QString::fromStdString(node.node_name()),
            node.is_dir(),
            node.file_size(),
            QString::fromStdString(node.update_time()));
    }
}

void FileManagerPage::onBtnBackClicked()
{
    if (m_historyStack.isEmpty()) {
        CinemaMessageBox::ShowInfo(this, u8"提示", u8"已经是根目录了！");
        return;
    }
    // 出栈，拿到上一级的 ID
    m_currentParentId = m_historyStack.pop();
    m_pathLabel->setText(m_currentParentId == 0 ? u8"当前位置: 根目录" : u8"当前位置: 某个文件夹"); // 可根据实际完善路径名记录
    RequestListDirectory(m_currentParentId);
}

void FileManagerPage::onTableDoubleClicked(int row, int column)
{
    // 提取隐藏在第一列的属性
    QTableWidgetItem* item = m_fileTable->item(row, 0);
    int64_t nodeId = item->data(Qt::UserRole).toLongLong();
    bool isDir = item->data(Qt::UserRole + 1).toBool();

    if (isDir) 
    {
        // 👑 进入文件夹：当前 ID 入栈备份，更新当前 ID 为目标文件夹
        m_historyStack.push(m_currentParentId);
        m_currentParentId = nodeId;
        m_pathLabel->setText(QString(u8"当前位置: 进入目录 [%1]").arg(item->text().remove(u8"📁 ")));
        RequestListDirectory(m_currentParentId);
    }
    else 
    {
        // TODO: 双击文件，弹出预览或开始下载逻辑
        qDebug() << "[FileManager] 双击了文件，准备处理下载...";
    }
}

// -------------------------------------------------------------------------
// 🚀 [增删改] 业务逻辑
// -------------------------------------------------------------------------
void FileManagerPage::onBtnNewFolderClicked()
{
    bool ok;
    // 👑 使用原生的深度定制框替换 QInputDialog
    QString folderName = CinemaInputDialog::GetInput(this, u8"新建文件夹", u8"请输入新文件夹的名称:", u8"新建文件夹", &ok);

    if (ok && !folderName.isEmpty()) {
        omnibox::CreateFolderRequest req;
        req.set_user_id(UserMgr::Instance()->GetId());
        req.set_parent_id(m_currentParentId);
        req.set_folder_name(folderName.toStdString());
        ThreadPool::Instance()->GetTCPMgr()->SendProtoMsg(omnibox::MsgId::ID_CREATE_FOLDER_REQ, req);
    }
}

void FileManagerPage::onCreateFolderRsp(const omnibox::CreateFolderResponse& rsp)
{
    RequestListDirectory(m_currentParentId); // 刷新当前列表
}

void FileManagerPage::onTableContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = m_fileTable->itemAt(pos);
    if (item) {
        m_fileTable->selectRow(item->row()); // 确保点击右键时该行被选中
        m_contextMenu->exec(QCursor::pos()); // 在鼠标位置弹出菜单
    }
}

void FileManagerPage::onActionRename()
{
    int row = m_fileTable->currentRow();
    if (row < 0) return;

    QTableWidgetItem* item = m_fileTable->item(row, 0);
    int64_t nodeId = item->data(Qt::UserRole).toLongLong();
    QString oldName = item->text().remove(u8"📁 ").remove(u8"📄 "); // 去掉图标前缀

    bool ok;
    // 👑 同样替换为自定义输入框
    QString newName = CinemaInputDialog::GetInput(this, u8"重命名", u8"请输入新的名称:", oldName, &ok);

    if (ok && !newName.isEmpty() && newName != oldName) {
        omnibox::RenameNodeRequest req;
        req.set_user_id(UserMgr::Instance()->GetId());
        req.set_node_id(nodeId);
        req.set_new_name(newName.toStdString());
        ThreadPool::Instance()->GetTCPMgr()->SendProtoMsg(omnibox::MsgId::ID_RENAME_NODE_REQ, req);
    }
}

void FileManagerPage::onRenameNodeRsp(const omnibox::RenameNodeResponse& rsp)
{
    RequestListDirectory(m_currentParentId);
}

void FileManagerPage::onActionDelete()
{
    int row = m_fileTable->currentRow();
    if (row < 0) return;

    if (!CinemaMessageBox::ShowQuestion(this, u8"删除确认", u8"确定要删除该项吗？(移入回收站)")) return;

    int64_t nodeId = m_fileTable->item(row, 0)->data(Qt::UserRole).toLongLong();
    omnibox::DeleteNodeRequest req;
    req.set_user_id(UserMgr::Instance()->GetId());
    req.set_node_id(nodeId);
    ThreadPool::Instance()->GetTCPMgr()->SendProtoMsg(omnibox::MsgId::ID_DELETE_NODE_REQ, req);
}

void FileManagerPage::onDeleteNodeRsp(const omnibox::DeleteNodeResponse& rsp)
{
    RequestListDirectory(m_currentParentId);
}

void FileManagerPage::onActionCut()
{
    int row = m_fileTable->currentRow();
    if (row < 0) return;

    // 1. 记录要被移动的节点信息到“虚拟剪切板”
    QTableWidgetItem* item = m_fileTable->item(row, 0);
    m_cutNodeId = item->data(Qt::UserRole).toLongLong();
    m_cutNodeName = item->text().remove(u8"📁 ").remove(u8"📄 ");

    // 2. 激活 UI 粘贴模式
    m_btnPaste->setText(QString(u8"📋 粘贴 [%1] 到此").arg(m_cutNodeName));
    m_btnPaste->setVisible(true);

    // 给用户一个轻量提示
    qDebug() << u8"[FileManager] 剪切成功，请导航到目标文件夹后点击右上角粘贴";
}

void FileManagerPage::onBtnPasteClicked()
{
    // 1. 基本安全校验
    if (m_cutNodeId == -1) return;

    // 前端防痴呆拦截：不能把文件夹移动到自己当前所在的目录 (原地踏步)
    // 注意：更严谨的拦截（比如不能把父目录移动到子目录里）需要后端 MetaService 去校验，前端这里只做基础拦截
    if (m_cutNodeId == m_currentParentId) {
        CinemaMessageBox::ShowWarning(this, u8"警告", u8"不能将文件夹移动到其自身内部！");
        return;
    }

    // 2. 向服务器发送移动请求
    omnibox::MoveNodeRequest req;
    req.set_user_id(UserMgr::Instance()->GetId());
    req.set_node_id(m_cutNodeId);
    req.set_target_parent_id(m_currentParentId); // 👑 目标地就是我当前所在的目录！

    ThreadPool::Instance()->GetTCPMgr()->SendProtoMsg(omnibox::MsgId::ID_MOVE_NODE_REQ, req);

    // 3. 恢复 UI 状态 (无论成功失败，发完请求就清空剪切板)
    m_cutNodeId = -1;
    m_cutNodeName.clear();
    m_btnPaste->setVisible(false);
}

void FileManagerPage::onMoveNodeRsp(const omnibox::MoveNodeResponse& rsp)
{
    // 收到后端的移动成功响应后，刷新当前目录，就能看到刚刚粘贴过来的文件了！
    RequestListDirectory(m_currentParentId);
}

void FileManagerPage::onBtnRefreshClicked()
{
    RequestListDirectory(m_currentParentId);
}

// -------------------------------------------------------------------------
// 🎨 UI 辅助渲染
// -------------------------------------------------------------------------
void FileManagerPage::InsertRowToUI(int64_t nodeId, const QString& name, bool isDir, int64_t size, const QString& updateTime)
{
    int row = m_fileTable->rowCount();
    m_fileTable->insertRow(row);

    // 第一列：文件名 (带上 Emoji 图标作为区分)
    QString displayName = (isDir ? u8"📁 " : u8"📄 ") + name;
    QTableWidgetItem* nameItem = new QTableWidgetItem(displayName);

    // 👑 绝杀技巧：将 ID 和 是否为目录 的属性，藏入 UserRole
    nameItem->setData(Qt::UserRole, QVariant::fromValue(nodeId));
    nameItem->setData(Qt::UserRole + 1, QVariant::fromValue(isDir));

    m_fileTable->setItem(row, 0, nameItem);

    // 第二列：类型
    QTableWidgetItem* typeItem = new QTableWidgetItem(isDir ? u8"文件夹" : u8"文件");
    typeItem->setTextAlignment(Qt::AlignCenter);
    if (isDir) typeItem->setForeground(QColor("#00C3FF")); // 文件夹字体稍微高亮
    m_fileTable->setItem(row, 1, typeItem);

    // 第三列：大小
    QString sizeStr = isDir ? "-" : FormatSize(size); // 文件夹不显示大小
    QTableWidgetItem* sizeItem = new QTableWidgetItem(sizeStr);
    sizeItem->setTextAlignment(Qt::AlignCenter);
    m_fileTable->setItem(row, 2, sizeItem);

    // 第四列：更新时间
    QTableWidgetItem* timeItem = new QTableWidgetItem(updateTime);
    timeItem->setTextAlignment(Qt::AlignCenter);
    m_fileTable->setItem(row, 3, timeItem);
}

QString FileManagerPage::FormatSize(int64_t bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 2) + " KB";
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}