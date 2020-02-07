#include "glslhighlighter.h"

namespace vpa {
    GLSLHighlighter::GLSLHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
        CreateKeywordRules();
        CreateAltKeywordRules();
    }

    void GLSLHighlighter::highlightBlock(const QString& text) {
        for (const HighlightingRule &rule : m_highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

    void GLSLHighlighter::CreateKeywordRules() {
        m_keywordFmt.setForeground(Qt::darkBlue);
        m_keywordFmt.setFontWeight(QFont::Bold);
        const QString keywordPatterns[] = {
            QStringLiteral(" float "), QStringLiteral(" int "), QStringLiteral(" uint "),
            QStringLiteral("layout"), QStringLiteral(" in "), QStringLiteral(" out "),
            QStringLiteral(" uniform "), QStringLiteral(" buffer "), QStringLiteral(" void "),
            QStringLiteral(" vec2 "), QStringLiteral(" vec3 "), QStringLiteral(" vec4 "),
            QStringLiteral(" mat3 "), QStringLiteral(" mat4 ")
        };
        HighlightingRule rule;
        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = m_keywordFmt;
            m_highlightingRules.append(rule);
        }
    }

    void GLSLHighlighter::CreateAltKeywordRules() {
        m_altKeywordFmt.setForeground(QColor(180, 120, 0));
        m_altKeywordFmt.setFontWeight(QFont::Bold);
        const QString keywordPatterns[] = {
            QStringLiteral("location ")
        };
        HighlightingRule rule;
        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = m_altKeywordFmt;
            m_highlightingRules.append(rule);
        }
    }
}
