#ifndef CONFIGVALIDATOR_H
#define CONFIGVALIDATOR_H

#include <QVector>
#include <QMap>
#include <vulkan/vulkan.h>

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
                uint32_t u;
                int i;
                float f;
            } value;
            Val() : type(ValueType::Uint) {
                value.u = 0;
            }
            Val(float f) : type(ValueType::Float) {
                value.f = f;
            }
            Val(uint32_t u) : type(ValueType::Uint) {
                value.u = u;
            }
            Val(int i) : type(ValueType::Int) {
                value.i = i;
            }
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

        friend bool operator==(const Ref& ref, const Val& val);
        friend bool operator!=(const Ref& ref, const Val& val){ return !operator==(ref, val); }
        friend bool operator<(const Ref& ref, const Val& val);
        friend bool operator>(const Ref& ref, const Val& val);
        friend bool operator<=(const Ref& ref, const Val& val){ return !operator>(ref, val); }
        friend bool operator>=(const Ref& ref, const Val& val){ return !operator<(ref, val); }

    public:
        ConfigValidator(const PipelineConfig& config, const VkPhysicalDeviceLimits& limits);
        ~ConfigValidator();

        VPAError Validate(const PipelineConfig& config) const;

    private:
        VPAError LoadValidationFile(const PipelineConfig& config, QString fileName);
        VPAError LoadVulkanConstantsFile(QString fileName);

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
        bool Compare(const Expression& expr) const;
        QString ExtractStrInStr(QString& str, int& pos) const;
        bool CheckSignedIntOverflow(uint32_t a, int32_t b) const;

        QVector<Rule> m_rules;
        QMap<QString, uint32_t> m_vulkanConstants;

        VkPhysicalDeviceLimits m_limits;

        static constexpr uint32_t True = 1;
    };
}

#endif // CONFIGVALIDATOR_H
