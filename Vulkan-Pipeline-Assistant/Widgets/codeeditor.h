#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

#include "Vulkan/compileerror.h"
#include "common.h"

class QLineEdit;
class QLabel;

namespace vpa {
    struct CompileError;
    class CodeEditor : public QPlainTextEdit {
        Q_OBJECT
    public:
        CodeEditor(QWidget* parent = nullptr);

        void LineNumberAreaPaintEvent(QPaintEvent* event);
        int LineNumberAreaWidth();

        void Init(QLabel* modifiedLabel, QLineEdit* fileNameWidget, QString fileExt, QString* configStr) {
            m_modifiedLabel = modifiedLabel;
            m_fileNameWidget = fileNameWidget;
            m_fileExt = fileExt;
            m_configStr = configStr;
        }

        void SetLastCompileErrors(QVector<CompileError>& errors);
        QLineEdit* FileNameWidget() const { return m_fileNameWidget; }

    public slots:
        void Save();
        void Load(bool openDialog);
        void DisplayModificationState(bool changed);

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private slots:
        void UpdateLineNumberAreaWidth(int newBlockCount);
        void HighlightCurrentLine();
        void UpdateLineNumberArea(const QRect& rect, int dy);

    private:
        QWidget* m_lineNumberArea;
        QVector<CompileError> m_compileErrors;
        QLineEdit* m_fileNameWidget;
        QString m_fileExt;
        QString* m_configStr;
        QLabel* m_modifiedLabel;

        static constexpr int NumSpacesInTab = 4;
    };

    class LineNumberArea : public QWidget {
    public:
        LineNumberArea(CodeEditor* editor) : QWidget(editor), m_codeEditor(editor) {}

        QSize sizeHint() const override { return QSize(m_codeEditor->LineNumberAreaWidth(), 0); }

    protected:
        void paintEvent(QPaintEvent *event) override { m_codeEditor->LineNumberAreaPaintEvent(event); }

    private:
        CodeEditor* m_codeEditor;
    };
}
#endif // CODEEDITOR_H
