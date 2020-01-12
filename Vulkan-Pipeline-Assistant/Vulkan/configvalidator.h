#ifndef CONFIGVALIDATOR_H
#define CONFIGVALIDATOR_H

#include <QVector>
#include <QMap>

#include "../common.h"

class QTextStream;

namespace vpa {
    struct PipelineConfig;

    class ConfigValidator {
        enum class ComparisonOperator {
            Lesser, LesserEqual, Greater,
            GreaterEqual, Equal, NotEqual
        };

        enum class LogicOperator {
            And, Or
        };

        enum class ValueType {
            Uint, Int, Float
        };

        struct Val {
            ValueType type;
            union {
                size_t u;
                int i;
                float f;
            } value;
        };

        struct Ref {
            ValueType type;
            const void* ref;
        };

        struct Expression {
            Ref refA;
            Val valB;
            ComparisonOperator compareOp;
        };

        struct Rule {
            QString name;
            QString reference;
            Expression requirement;
            QVector<Expression> conditions;
            QVector<LogicOperator> conditionOps; // conditionOps.size = conditions.size - 1
        };

        static const QVector<QString> ConfigIdentifiers;

    public:
        ConfigValidator(const PipelineConfig& config);
        ~ConfigValidator();

        VPAError LoadValidationFile(const PipelineConfig& config, QString fileName);
        VPAError LoadVulkanConstantsFile(QString fileName);

    private:
        VPAError Validate(const PipelineConfig& config) const;
        // Validation must consider all the rules provided in the validation file
        VPAError ValidateRules() const;
        // The most basic check is against the physical device limits, if these fail then Vulkan cannot draw in the app
        VPAError ValidateLimits(const PipelineConfig& config) const;

        Rule ParseRule(const PipelineConfig& config, QTextStream& in, QString& line);
        Expression ParseExpression(const PipelineConfig& config, const QString &line) const;
        Ref ParseConfigIdentifier(const PipelineConfig& config, const QString& symbol) const;
        Val ParseVulkanConstant(const QString& symbol) const;
        ComparisonOperator ParseCompareOp(const QString& symbol) const;
        LogicOperator ParseLogicOp(const QString& symbol) const;

        bool ExecuteRule(const Rule& rule) const;
        VPAError VPAAssert(const bool expr, const QString msg) const;

        QVector<Rule> m_rules;
        QMap<QString, size_t> m_vulkanConstants;
    };
}

#endif // CONFIGVALIDATOR_H
