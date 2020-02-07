#ifndef GLSLHIGHLIGHTER_H
#define GLSLHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

#include "common.h"

namespace vpa {
    class GLSLHighlighter : public QSyntaxHighlighter {
        struct HighlightingRule {
            QRegularExpression pattern;
            QTextCharFormat format;
        };
    public:
        GLSLHighlighter(QTextDocument* parent);

    protected:
        void highlightBlock(const QString& text) override;

    private:
        void LoadFromFile(const char* path);

        QVector<HighlightingRule> m_highlightingRules;
    };
}

#endif // GLSLHIGHLIGHTER_H
