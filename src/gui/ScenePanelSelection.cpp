#include "gui/ScenePanel.h"

#include <cstddef>

#include <QListWidget>
#include <QListWidgetItem>
#include <QString>

void ScenePanel::setSelectedObjectId(int objectId)
{
    if (objectList_ == nullptr) {
        return;
    }

    if (objectId < 0) {
        objectList_->clearSelection();
        loadSelectedEditor();
        return;
    }

    for (int row = 0; row < objectList_->count(); ++row) {
        QListWidgetItem* item = objectList_->item(row);
        if (item == nullptr || item->data(Qt::UserRole).toString() != QStringLiteral("object")) {
            continue;
        }

        const int objectIndex = item->data(Qt::UserRole + 1).toInt();
        if (objectIndex < 0 || objectIndex >= static_cast<int>(scene_.objects.size())) {
            continue;
        }

        const auto& object = scene_.objects[static_cast<std::size_t>(objectIndex)];
        if (object && object->id() == objectId) {
            objectList_->setCurrentRow(row);
            loadSelectedEditor();
            return;
        }
    }

    objectList_->clearSelection();
    loadSelectedEditor();
}
