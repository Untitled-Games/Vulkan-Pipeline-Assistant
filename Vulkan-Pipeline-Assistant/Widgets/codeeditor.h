#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

#include "Vulkan/compileerror.h"
#include "common.h"

namespace vpa {
    struct CompileError;
    class CodeEditor : public QPlainTextEdit {
        Q_OBJECT
    public:
        CodeEditor(QWidget* parent = nullptr);

        void LineNumberAreaPaintEvent(QPaintEvent* event);
        int LineNumberAreaWidth();

        void SetLastCompileErrors(QVector<CompileError>& errors);

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private slots:
        void UpdateLineNumberAreaWidth(int newBlockCount);
        void HighlightCurrentLine();
        void UpdateLineNumberArea(const QRect& rect, int dy);

    private:
        QWidget* m_lineNumberArea;
        QVector<CompileError> m_compileErrors;
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
