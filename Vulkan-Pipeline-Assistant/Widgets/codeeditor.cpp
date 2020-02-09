#include "codeeditor.h"

#include <QPainter>
#include <QTextBlock>
#include <QLineEdit>
#include <QFileDialog>
#include <QLabel>

namespace vpa {
    CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent), m_fileExt("") {
        document()->setModified(false);
        m_lineNumberArea = new LineNumberArea(this);
        connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::UpdateLineNumberAreaWidth);
        connect(this, &CodeEditor::updateRequest, this, &CodeEditor::UpdateLineNumberArea);
        connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::HighlightCurrentLine);
        UpdateLineNumberAreaWidth(0);
        HighlightCurrentLine();
        QFontMetrics metrics(font());
        setTabStopDistance(NumSpacesInTab * metrics.averageCharWidth());

        connect(this, SIGNAL(modificationChanged(bool)), this, SLOT(DisplayModificationState(bool)));
    }

    void CodeEditor::LineNumberAreaPaintEvent(QPaintEvent* event) {
        QPainter painter(m_lineNumberArea);
        painter.fillRect(event->rect(), Qt::lightGray);
        painter.setBackground(Qt::red);

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + qRound(blockBoundingRect(block).height());

        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);
                if (Find(m_compileErrors, blockNumber + 1) != -1) {
                    painter.setBackgroundMode(Qt::OpaqueMode);
                    painter.setPen(Qt::white);
                }
                else {
                    painter.setBackgroundMode(Qt::TransparentMode);
                    painter.setPen(Qt::black);
                }
                painter.drawText(0, top, m_lineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
            }
            block = block.next();
            top = bottom;
            bottom = top + qRound(blockBoundingRect(block).height());
            ++blockNumber;
        }
    }

    int CodeEditor::LineNumberAreaWidth() {
        int digits = 1;
        int max = qMax(1, blockCount());
        while (max >= 10) {
            max /= 10;
            ++digits;
        }

        return 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    }

    void CodeEditor::SetLastCompileErrors(QVector<CompileError>& errors) {
        m_compileErrors = errors;
        m_lineNumberArea->repaint();
    }

    void CodeEditor::Save() {
        if (!m_fileNameWidget) return;
        if (!document()->isModified()) return;

        if (m_fileNameWidget->text() == "") {
            QString name = QFileDialog::getSaveFileName(this, tr("Save File"), SHADERSRCDIR, tr(qPrintable("Shader Files (*." + m_fileExt + ")")));
            m_fileNameWidget->setText(name);
            *m_configStr = name;
        }

        QFile file(m_fileNameWidget->text());
        if (file.open(QIODevice::Text | QIODevice::WriteOnly)) {
            file.write(toPlainText().toUtf8());
            file.close();
            document()->setModified(false);
        }
    }

    void CodeEditor::Load(bool openDialog) {
        if (!m_fileNameWidget) return;

        if (openDialog) {
            QString str = QFileDialog::getOpenFileName(this, tr("Open File"), SHADERSRCDIR, tr(qPrintable("Shader Files (*." + m_fileExt + ")")));
            m_fileNameWidget->setText(str);
            *m_configStr = str;
        }

        QFile file(m_fileNameWidget->text());
        if (file.open(QIODevice::Text | QIODevice::ReadOnly)) {
            setPlainText(file.readAll());
            file.close();
            document()->setModified(false);
        }
    }

    void CodeEditor::DisplayModificationState(bool changed) {
        if (!m_modifiedLabel) return;
        if (changed) m_modifiedLabel->setText("*");
        else m_modifiedLabel->setText("");
    }

    void CodeEditor::resizeEvent(QResizeEvent* event) {
        QPlainTextEdit::resizeEvent(event);
        QRect cr = contentsRect();
        m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), LineNumberAreaWidth(), cr.height()));
    }

    void CodeEditor::UpdateLineNumberAreaWidth(int newBlockCount) {
        Q_UNUSED(newBlockCount)
        setViewportMargins(LineNumberAreaWidth(), 0, 0, 0);
    }

    void CodeEditor::HighlightCurrentLine() {
        QList<QTextEdit::ExtraSelection> extraSelections;
        if (!isReadOnly()) {
             QTextEdit::ExtraSelection selection;

             QColor lineColor = QColor(Qt::lightGray);

             selection.format.setBackground(lineColor);
             selection.format.setProperty(QTextFormat::FullWidthSelection, true);
             selection.cursor = textCursor();
             selection.cursor.clearSelection();
             extraSelections.append(selection);
        }
        setExtraSelections(extraSelections);
    }

    void CodeEditor::UpdateLineNumberArea(const QRect& rect, int dy) {
        if (dy) m_lineNumberArea->scroll(0, dy);
        else m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

        if (rect.contains(viewport()->rect())) UpdateLineNumberAreaWidth(0);
    }
}
