#ifndef CHORUSKIT_TREECONFIGWIDGET_H
#define CHORUSKIT_TREECONFIGWIDGET_H

#include "QPushButton"
#include "QTreeWidget"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

namespace TemplatePlg {
    namespace Internal {
        class TreeConfigWidget : public QWidget {
            Q_OBJECT
        public:
            explicit TreeConfigWidget(QString configPath, bool configGen = false, QWidget *parent = nullptr);
            ~TreeConfigWidget();

        protected:
            QString configPath;
            bool configGen;
            QTreeWidget *m_treeWidget;
            QVBoxLayout *mainLayout;
            QLineEdit *m_name;
            QLineEdit *m_enname;
            QLineEdit *m_value;
            QComboBox *m_childType;
            QComboBox *m_type;
            QPushButton *m_addButton;
            QPushButton *m_removeButton;
            QPushButton *m_saveButton;

        private:
            QString getLocalLanguage();
            void addTableRow();
            void removeTableRow();
            QJsonObject readJsonFile();
            QJsonObject createJsonFromTree(QTreeWidget *treeWidget, QTreeWidgetItem *item = nullptr);
            void loadConfig(const QJsonObject &config, QTreeWidgetItem *parent = nullptr);
            bool saveConfig();
        };

    }
}
#endif // CHORUSKIT_TREECONFIGWIDGET_H
