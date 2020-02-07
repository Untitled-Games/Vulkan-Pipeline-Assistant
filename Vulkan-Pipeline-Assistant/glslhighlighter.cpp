#include "glslhighlighter.h"

#include <QtXml/QtXml>

namespace vpa {
    GLSLHighlighter::GLSLHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
        LoadFromFile(EDIDIR"style.xml");
    }

    void GLSLHighlighter::LoadFromFile(const char* path) {
        QDomDocument styleDOM;
        QFile styleFile(path);
        if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            VPA_WARN("Failed to open style file");
            return;
        } else if (!styleDOM.setContent(&styleFile)) {
            styleFile.close();
            VPA_WARN("Failed to read style file");
            return;
        }

        QDomElement root = styleDOM.firstChildElement();
        QDomNodeList styles = root.elementsByTagName("keywords");

        for (int styleIt = 0; styleIt < styles.size(); ++styleIt) {
            QDomNode thisStyle = styles.at(styleIt);
            if (!thisStyle.isElement()) {
                continue;
            }

            QDomNamedNodeMap styleData = thisStyle.attributes();

            QTextCharFormat style;
            if (styleData.namedItem("bold").nodeValue() == "true") style.setFontWeight(QFont::Bold);
            if (!styleData.namedItem("colour").isNull()) {
                QColor colourVal;
                colourVal.setNamedColor(styleData.namedItem("colour").nodeValue());
                style.setForeground(colourVal);
            } else if (!styleData.namedItem("color").isNull()) {
                QColor colourVal;
                colourVal.setNamedColor(styleData.namedItem("color").nodeValue());
                style.setForeground(colourVal);
            }

            QDomElement styleElement = thisStyle.toElement();
            QDomNodeList patterns = styleElement.elementsByTagName("pattern");
            HighlightingRule rule;
            rule.format = style;
            for (int patternIt = 0; patternIt < patterns.size(); ++patternIt) {    
                rule.pattern = QRegularExpression(patterns.at(patternIt).toElement().text());
                m_highlightingRules.append(rule);
            }
        }
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
}
