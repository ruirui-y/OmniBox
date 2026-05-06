#ifndef FILEMANAGERPAGE_H
#define FILEMANAGERPAGE_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QStack>
#include <QMenu>
#include "common.pb.h"
#include "server_msg.pb.h"

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

    void onTableDoubleClicked(int row, int column);             // 双击进入文件夹
    void onTableContextMenu(const QPoint& pos);                 // 右键菜单 (重命名/删除)

    // ================= 右键菜单动作 =================
    void onActionRename();
    void onActionDelete();

    // ================= 网络数据回执 =================
    void onListDirectoryRsp(const omnibox::ListDirectoryResponse& rsp);
    void onCreateFolderRsp(const omnibox::CreateFolderResponse& rsp);
    void onDeleteNodeRsp(const omnibox::DeleteNodeResponse& rsp);
    void onRenameNodeRsp(const omnibox::RenameNodeResponse& rsp);

private:
    void BuildUI();

    // 核心网络请求
    void RequestListDirectory(int64_t parentId);

    // 辅助 UI 渲染
    void InsertRowToUI(int64_t nodeId, const QString& name, bool isDir, int64_t size, const QString& updateTime);
    QString FormatSize(int64_t bytes);                          // 格式化文件大小 (KB, MB, GB)

private:
    QTableWidget* m_fileTable;                                  // 文件列表
    QLabel* m_pathLabel;                                        // 当前路径提示 (面包屑)

    // 核心状态控制：目录漫游栈
    int64_t m_currentParentId;                                  // 当前所处的目录ID (0表示根目录)
    QStack<int64_t> m_historyStack;                             // 历史路径栈 (用于“返回上一级”)

    // 右键菜单
    QMenu* m_contextMenu;
};

#endif // FILEMANAGERPAGE_H