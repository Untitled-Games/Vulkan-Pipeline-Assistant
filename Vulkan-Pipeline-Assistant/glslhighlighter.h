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
        void CreateKeywordRules();
        void CreateAltKeywordRules();

        QVector<HighlightingRule> m_highlightingRules;
        QRegularExpression m_commentStartExpr;
        QRegularExpression m_commentEndExpr;
        QTextCharFormat m_keywordFmt;
        QTextCharFormat m_altKeywordFmt;
        QTextCharFormat m_singleLineCommentFmt;
        QTextCharFormat m_multiLineCommentFmt;
        QTextCharFormat m_typeFmt;
        QTextCharFormat m_functionFmt;
    };
}

#endif // GLSLHIGHLIGHTER_H
