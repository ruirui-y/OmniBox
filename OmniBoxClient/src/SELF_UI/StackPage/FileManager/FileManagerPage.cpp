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
, current_parent_id_(0), cut_node_id_(-1)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setObjectName("FileManagerPage");

    path_names_.append(u8"根目录");
    BuildUI();

    path_label_->setText(u8"当前位置: " + path_names_.join(u8" / "));

    // ==========================================================
    // 👑 绑定网络响应槽函数
    // ==========================================================
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigListDirectory, this, &FileManagerPage::onListDirectoryRsp);
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigFolderCreated, this, &FileManagerPage::onCreateFolderRsp);
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigNodeDeleted, this, &FileManagerPage::onDeleteNodeRsp);
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigNodeRenamed, this, &FileManagerPage::onRenameNodeRsp);
    connect(ThreadPool::Instance()->GetTCPMgr(), &TCPMgr::SigNodeMoved, this, &FileManagerPage::onMoveNodeRsp);

    // 💡 启动时：默认拉取根目录 (parent_id = 0)
    RequestListDirectory(current_parent_id_);
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

    path_label_ = new QLabel(u8"当前位置: 根目录", this);
    path_label_->setObjectName("pathLabel");

    btn_paste_ = new QPushButton(u8"📋 粘贴到此", this);
    btn_paste_->setObjectName("controlBtn");
    btn_paste_->setMinimumSize(100, 35);
    btn_paste_->setVisible(false);

    QPushButton* btnNewFolder = new QPushButton(u8"📁 新建文件夹", this);
    btnNewFolder->setObjectName("controlBtn");
    btnNewFolder->setMinimumSize(120, 35);

    QPushButton* btnRefresh = new QPushButton(u8"🔄 刷新", this);
    btnRefresh->setObjectName("controlBtn");
    btnRefresh->setMinimumSize(90, 35);

    topLayout->addWidget(btnBack);
    topLayout->addWidget(path_label_);
    topLayout->addStretch();
    topLayout->addWidget(btn_paste_);
    topLayout->addWidget(btnNewFolder);
    topLayout->addWidget(btnRefresh);

    // ================= 2. 数据表格 =================
    file_table_ = new QTableWidget(0, 4, this);
    file_table_->setObjectName("fileTable");
    file_table_->setHorizontalHeaderLabels({ u8"文件名", u8"类型", u8"大小", u8"修改时间" });

    // 列宽策略：文件名拉伸，其他固定适应内容
    file_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    file_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    file_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    file_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    file_table_->setAlternatingRowColors(true);
    file_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);    // 禁止双击编辑
    file_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    file_table_->verticalHeader()->setVisible(false);
    file_table_->setFocusPolicy(Qt::NoFocus);
    file_table_->setShowGrid(false);                                    // 彻底关闭原生的丑陋网格线
    file_table_->verticalHeader()->setDefaultSectionSize(45);           // 统一增加行高，告别拥挤感 (现代 UI 标配)

    // 👑 开启自定义右键菜单策略
    file_table_->setContextMenuPolicy(Qt::CustomContextMenu);

    layout->addLayout(topLayout);
    layout->addWidget(file_table_, 1);

    // ================= 3. 信号绑定 =================
    connect(btnBack, &QPushButton::clicked, this, &FileManagerPage::onBtnBackClicked);
    connect(btnNewFolder, &QPushButton::clicked, this, &FileManagerPage::onBtnNewFolderClicked);
    connect(btn_paste_, &QPushButton::clicked, this, &FileManagerPage::onBtnPasteClicked);
    connect(btnRefresh, &QPushButton::clicked, this, &FileManagerPage::onBtnRefreshClicked);

    connect(file_table_, &QTableWidget::cellDoubleClicked, this, &FileManagerPage::onTableDoubleClicked);
    connect(file_table_, &QTableWidget::customContextMenuRequested, this, &FileManagerPage::onTableContextMenu);

    // ================= 4. 初始化右键菜单 =================
    context_menu_ = new QMenu(this);
    context_menu_->setObjectName("fileContextMenu");
    context_menu_->addAction(u8"✂️ 剪切", this, &FileManagerPage::onActionCut);
    context_menu_->addAction(u8"✏️ 重命名", this, &FileManagerPage::onActionRename);
    context_menu_->addAction(u8"🗑️ 删除", this, &FileManagerPage::onActionDelete);
}

// -------------------------------------------------------------------------
// 🚀 [核心逻辑]：目录跳转与拉取
// -------------------------------------------------------------------------
void FileManagerPage::RequestListDirectory(int64_t parentId)
{
    file_table_->setRowCount(0); // 加载前清空

    omnibox::ListDirectoryRequest req;
    req.set_user_id(UserMgr::Instance()->GetId());
    req.set_parent_id(parentId);

    ThreadPool::Instance()->GetTCPMgr()->SendProtoMsg(omnibox::MsgId::ID_LIST_DIR_REQ, req);
}

void FileManagerPage::onListDirectoryRsp(const omnibox::ListDirectoryResponse& rsp)
{
    file_table_->setRowCount(0);
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

// -------------------------------------------------------------------------
// 双击进入文件夹 (进栈)
// -------------------------------------------------------------------------
void FileManagerPage::onTableDoubleClicked(int row, int column)
{
    // 提取隐藏在第一列的属性
    QTableWidgetItem* item = file_table_->item(row, 0);
    int64_t nodeId = item->data(Qt::UserRole).toLongLong();
    bool isDir = item->data(Qt::UserRole + 1).toBool();

    if (isDir)
    {
        // 提取纯净的文件夹名字 (去掉前面的 📁 图标和空格)
        QString folderName = item->text().remove(u8"📁 ").trimmed();

        // 1. ID 状态入栈
        history_stack_.push(current_parent_id_);
        current_parent_id_ = nodeId;

        // 2. 👑 路径名称入栈，并用 " / " 优雅拼接刷新 UI
        path_names_.append(folderName);
        path_label_->setText(u8"当前位置: " + path_names_.join(u8" / "));

        RequestListDirectory(current_parent_id_);
    }
    else
    {
        qDebug() << "[FileManager] 双击了文件，准备处理下载...";
    }
}

// -------------------------------------------------------------------------
// 返回上一级 (出栈)
// -------------------------------------------------------------------------
void FileManagerPage::onBtnBackClicked()
{
    if (history_stack_.isEmpty()) {
        CinemaMessageBox::ShowInfo(this, u8"提示", u8"已经是根目录了！");
        return;
    }

    // 1. ID 状态出栈
    current_parent_id_ = history_stack_.pop();

    // 2. 👑 路径名称出栈 (删掉当前文件夹的名字)，刷新 UI
    path_names_.removeLast();
    path_label_->setText(u8"当前位置: " + path_names_.join(u8" / "));

    RequestListDirectory(current_parent_id_);
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
        req.set_parent_id(current_parent_id_);
        req.set_folder_name(folderName.toStdString());
        ThreadPool::Instance()->GetTCPMgr()->SendProtoMsg(omnibox::MsgId::ID_CREATE_FOLDER_REQ, req);
    }
}

void FileManagerPage::onCreateFolderRsp(const omnibox::CreateFolderResponse& rsp)
{
    RequestListDirectory(current_parent_id_); // 刷新当前列表
}

void FileManagerPage::onTableContextMenu(const QPoint& pos)
{
    QTableWidgetItem* item = file_table_->itemAt(pos);
    if (item) {
        file_table_->selectRow(item->row()); // 确保点击右键时该行被选中
        context_menu_->exec(QCursor::pos()); // 在鼠标位置弹出菜单
    }
}

void FileManagerPage::onActionRename()
{
    int row = file_table_->currentRow();
    if (row < 0) return;

    QTableWidgetItem* item = file_table_->item(row, 0);
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
    RequestListDirectory(current_parent_id_);
}

void FileManagerPage::onActionDelete()
{
    int row = file_table_->currentRow();
    if (row < 0) return;

    if (!CinemaMessageBox::ShowQuestion(this, u8"删除确认", u8"确定要删除该项吗？(移入回收站)")) return;

    int64_t nodeId = file_table_->item(row, 0)->data(Qt::UserRole).toLongLong();
    omnibox::DeleteNodeRequest req;
    req.set_user_id(UserMgr::Instance()->GetId());
    req.set_node_id(nodeId);
    ThreadPool::Instance()->GetTCPMgr()->SendProtoMsg(omnibox::MsgId::ID_DELETE_NODE_REQ, req);
}

void FileManagerPage::onDeleteNodeRsp(const omnibox::DeleteNodeResponse& rsp)
{
    RequestListDirectory(current_parent_id_);
}

void FileManagerPage::onActionCut()
{
    int row = file_table_->currentRow();
    if (row < 0) return;

    // 1. 记录要被移动的节点信息到“虚拟剪切板”
    QTableWidgetItem* item = file_table_->item(row, 0);
    cut_node_id_ = item->data(Qt::UserRole).toLongLong();
    cut_node_name_ = item->text().remove(u8"📁 ").remove(u8"📄 ");

    // 2. 激活 UI 粘贴模式
    btn_paste_->setText(QString(u8"📋 粘贴 [%1] 到此").arg(cut_node_name_));
    btn_paste_->setVisible(true);

    // 给用户一个轻量提示
    qDebug() << u8"[FileManager] 剪切成功，请导航到目标文件夹后点击右上角粘贴";
}

void FileManagerPage::onBtnPasteClicked()
{
    // 1. 基本安全校验
    if (cut_node_id_ == -1) return;

    // 前端防痴呆拦截：不能把文件夹移动到自己当前所在的目录 (原地踏步)
    // 注意：更严谨的拦截（比如不能把父目录移动到子目录里）需要后端 MetaService 去校验，前端这里只做基础拦截
    if (cut_node_id_ == current_parent_id_) {
        CinemaMessageBox::ShowWarning(this, u8"警告", u8"不能将文件夹移动到其自身内部！");
        return;
    }

    // 2. 向服务器发送移动请求
    omnibox::MoveNodeRequest req;
    req.set_user_id(UserMgr::Instance()->GetId());
    req.set_node_id(cut_node_id_);
    req.set_target_parent_id(current_parent_id_); // 👑 目标地就是我当前所在的目录！

    ThreadPool::Instance()->GetTCPMgr()->SendProtoMsg(omnibox::MsgId::ID_MOVE_NODE_REQ, req);

    // 3. 恢复 UI 状态 (无论成功失败，发完请求就清空剪切板)
    cut_node_id_ = -1;
    cut_node_name_.clear();
    btn_paste_->setVisible(false);
}

void FileManagerPage::onMoveNodeRsp(const omnibox::MoveNodeResponse& rsp)
{
    // 收到后端的移动成功响应后，刷新当前目录，就能看到刚刚粘贴过来的文件了！
    RequestListDirectory(current_parent_id_);
}

void FileManagerPage::onBtnRefreshClicked()
{
    RequestListDirectory(current_parent_id_);
}

// -------------------------------------------------------------------------
// 🎨 UI 辅助渲染
// -------------------------------------------------------------------------
void FileManagerPage::InsertRowToUI(int64_t nodeId, const QString& name, bool isDir, int64_t size, const QString& updateTime)
{
    int row = file_table_->rowCount();
    file_table_->insertRow(row);

    // 第一列：文件名 (带上 Emoji 图标作为区分)
    QString displayName = (isDir ? u8"📁 " : u8"📄 ") + name;
    QTableWidgetItem* nameItem = new QTableWidgetItem(displayName);

    // 👑 绝杀技巧：将 ID 和 是否为目录 的属性，藏入 UserRole
    nameItem->setData(Qt::UserRole, QVariant::fromValue(nodeId));
    nameItem->setData(Qt::UserRole + 1, QVariant::fromValue(isDir));

    file_table_->setItem(row, 0, nameItem);

    // 第二列：类型
    QTableWidgetItem* typeItem = new QTableWidgetItem(isDir ? u8"文件夹" : u8"文件");
    typeItem->setTextAlignment(Qt::AlignCenter);
    if (isDir) typeItem->setForeground(QColor("#00C3FF")); // 文件夹字体稍微高亮
    file_table_->setItem(row, 1, typeItem);

    // 第三列：大小
    QString sizeStr = isDir ? "-" : FormatSize(size); // 文件夹不显示大小
    QTableWidgetItem* sizeItem = new QTableWidgetItem(sizeStr);
    sizeItem->setTextAlignment(Qt::AlignCenter);
    file_table_->setItem(row, 2, sizeItem);

    // 第四列：更新时间
    QTableWidgetItem* timeItem = new QTableWidgetItem(updateTime);
    timeItem->setTextAlignment(Qt::AlignCenter);
    file_table_->setItem(row, 3, timeItem);
}

QString FileManagerPage::FormatSize(int64_t bytes)
{
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024.0, 'f', 2) + " KB";
    if (bytes < 1024 * 1024 * 1024) return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
    return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}