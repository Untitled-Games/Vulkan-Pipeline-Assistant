#include "configvalidator.h"

#include <QRegularExpression>

#include "pipelineconfig.h"

namespace vpa {

    const QVector<QString> ConfigValidator::ConfigIdentifiers = { "topology" };

    ConfigValidator::ConfigValidator(const PipelineConfig& config) {
        LoadVulkanConstantsFile(RESDIR"vkconstants.txt");
        LoadValidationFile(config, RESDIR"validation.txt");
        qDebug("Loaded validation");
    }

    ConfigValidator::~ConfigValidator() { }

    VPAError ConfigValidator::LoadValidationFile(const PipelineConfig& config, QString fileName) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
             return VPA_CRITICAL("Could not open validation file.");
        }
        else {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (QStringRef(&line, 0, 5) == "<RULE") {
                    m_rules.push_back(ParseRule(config, in, line));
                }
            }
            file.close();
        }
        return VPA_OK;
    }

    VPAError ConfigValidator::LoadVulkanConstantsFile(QString fileName) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
             return VPA_CRITICAL("Could not open vulkan constants file.");
        }
        else {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList splitLine = line.split(" ");
                m_vulkanConstants[splitLine[0]] = splitLine[1].toUInt();
            }
            file.close();
        }
        return VPA_OK;
    }

    VPAError ConfigValidator::Validate(const PipelineConfig& config) const {
        VPA_PASS_ERROR(ValidateLimits(config));
        VPA_PASS_ERROR(ValidateRules());
        return VPA_OK;
    }

    VPAError ConfigValidator::ValidateRules() const {
        for (const Rule& rule : m_rules) {
            VPA_PASS_ERROR(VPAAssert(ExecuteRule(rule), "Validation error for " + rule.name + " See " + rule.reference));
        }
        return VPA_OK;
    }

    VPAError ConfigValidator::ValidateLimits(const PipelineConfig& config) const {
        //vertexBindingDescriptionCount must be less than or equal to VkPhysicalDeviceLimits::maxVertexInputBindings
        //vertexAttributeDescriptionCount must be less than or equal to VkPhysicalDeviceLimits::maxVertexInputAttributes
        VPA_PASS_ERROR(VPAAssert(config.writables.topology != VK_PRIMITIVE_TOPOLOGY_PATCH_LIST, "Topology cannot be patch list test"));

        return VPA_OK;
    }

    ConfigValidator::Rule ConfigValidator::ParseRule(const PipelineConfig& config, QTextStream& in, QString& line) {
        // <RULE "name" "reference"
        //      expr
        //      <IF>
        //          expr
        //          logic op, expr
        //          ...
        //      </IF>
        // </RULE>
        Rule rule;
        QStringList beginningLine = line.split(" "); // todo this don't work
        rule.name = beginningLine[1];
        rule.reference = beginningLine[2];

        line = in.readLine().trimmed().replace('>', ' ').replace('<', ' ');
        rule.requirement = ParseExpression(config, line);
        line = in.readLine(); // <IF>

        line = in.readLine().trimmed().replace('>', ' ').replace('<', ' ');
        rule.conditions.push_back(ParseExpression(config, line));
        while (!in.atEnd()) {
            line = in.readLine().trimmed();
            if (line == "</IF>") break;
            line = line.replace('>', ' ').replace('<', ' ');
            QStringList splitLine = line.split(" ");
            rule.conditionOps.push_back(ParseLogicOp(splitLine[1]));
            QString exprStr = splitLine[3] + " " + splitLine[4] + " " + splitLine[5];
            rule.conditions.push_back(ParseExpression(config, exprStr));
        }
        line = in.readLine();
        return rule;
    }

    ConfigValidator::Expression ConfigValidator::ParseExpression(const PipelineConfig& config, const QString& line) const {
        QStringList list = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        Expression expr;
        expr.refA = ParseConfigIdentifier(config, list.at(0));
        expr.compareOp = ParseCompareOp(list.at(1));
        expr.valB = ParseVulkanConstant(list.at(2));
        return expr;
    }

    ConfigValidator::Ref ConfigValidator::ParseConfigIdentifier(const PipelineConfig& config, const QString& symbol) const {
        Ref r;
        r.type = ValueType::Uint;
        if (symbol == "topology") {
            r.ref = reinterpret_cast<const void*>(&config.writables.topology);
        }
        return r;
    }

    ConfigValidator::Val ConfigValidator::ParseVulkanConstant(const QString& symbol) const {
        Val v;
        v.type = ValueType::Uint;
        v.value.u = m_vulkanConstants[symbol];
        return v;
    }

    ConfigValidator::ComparisonOperator ConfigValidator::ParseCompareOp(const QString& symbol) const {
        if (symbol == "EQUAL") return ComparisonOperator::Equal;
        else if (symbol == "NOT_EQUAL") return ComparisonOperator::NotEqual;
        else if (symbol == "LESSER") return ComparisonOperator::Lesser;
        else if (symbol == "LESSER_EQUAL") return ComparisonOperator::LesserEqual;
        else if (symbol == "GREATER") return ComparisonOperator::Greater;
        else return ComparisonOperator::GreaterEqual;
    }

    ConfigValidator::LogicOperator ConfigValidator::ParseLogicOp(const QString& symbol) const {
        if (symbol == "AND") return LogicOperator::And;
        else return LogicOperator::Or;
    }

    bool ConfigValidator::ExecuteRule(const Rule& rule) const {
        return true;
    }

    VPAError ConfigValidator::VPAAssert(const bool expr, const QString msg) const {
        if (expr) return VPA_WARN(msg);
        else return VPA_OK;
    }
}
