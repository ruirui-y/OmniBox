#ifndef FILEMANAGERPAGE_H
#define FILEMANAGERPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QStack>
#include <QMenu>
#include "common.pb.h"
#include "server_msg.pb.h"

class QPushButton;

class FileManagerPage : public QWidget
{
    Q_OBJECT

public:
    explicit FileManagerPage(QWidget* parent = nullptr);

private slots:
    // ================= UI 交互 =================
    void onBtnBackClicked();                                    // 返回上一级
    void onBtnNewFolderClicked();                               // 新建文件夹
    void onBtnRefreshClicked();                                 // 刷新当前目录
    void onBtnPasteClicked();                                   // 顶部“粘贴到此”按钮

    void onTableDoubleClicked(int row, int column);             // 双击进入文件夹
    void onTableContextMenu(const QPoint& pos);                 // 右键菜单 (重命名/删除)

    // ================= 右键菜单动作 =================
    void onActionRename();
    void onActionDelete();
    void onActionCut();                                         // 右键“剪切”

    // ================= 网络数据回执 =================
    void onListDirectoryRsp(const omnibox::ListDirectoryResponse& rsp);
    void onCreateFolderRsp(const omnibox::CreateFolderResponse& rsp);
    void onDeleteNodeRsp(const omnibox::DeleteNodeResponse& rsp);
    void onRenameNodeRsp(const omnibox::RenameNodeResponse& rsp);
    void onMoveNodeRsp(const omnibox::MoveNodeResponse& rsp);   // 移动节点响应

private:
    void BuildUI();

    // 核心网络请求
    void RequestListDirectory(int64_t parentId);

    // 辅助 UI 渲染
    void InsertRowToUI(int64_t nodeId, const QString& name, bool isDir, int64_t size, const QString& updateTime);
    QString FormatSize(int64_t bytes);                          // 格式化文件大小 (KB, MB, GB)

private:
    QTableWidget* file_table_;                                  // 文件列表
    QLabel* path_label_;                                        // 当前路径提示 (面包屑)
    QPushButton* btn_paste_;                                    // 粘贴按钮 (需要全局控制显示隐藏)

    // 核心状态控制：目录漫游栈
    int64_t current_parent_id_;                                 // 当前所处的目录ID (0表示根目录)
    QStack<int64_t> history_stack_;                             // 历史路径栈 (用于“返回上一级”)
    QStringList path_names_;                                    // 用于存储面包屑路径的名称列表

    // 右键菜单
    QMenu* context_menu_;

    // ================= 状态机：虚拟剪切板 =================
    int64_t cut_node_id_;                                       // 当前被剪切的节点ID (-1 表示没剪切任何东西)
    QString cut_node_name_;                                     // 当前被剪切的节点名字 (用于 UI 提示)
};

#endif // FILEMANAGERPAGE_H