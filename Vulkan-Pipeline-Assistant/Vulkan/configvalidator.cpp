#include "configvalidator.h"

#include <QRegularExpression>

#include "pipelineconfig.h"

namespace vpa {

    ConfigValidator::ConfigValidator(const PipelineConfig& config, const VkPhysicalDeviceLimits& limits) : m_limits(limits) {
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
        VPA_PASS_ERROR(VPAAssert(config.writables.topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
                                 config.writables.patchControlPoints > 0 &&
                                   config.writables.patchControlPoints < m_limits.maxTessellationPatchSize, "patchControlPoints must be greater than zero and less than or equal to VkPhysicalDeviceLimits::maxTessellationPatchSize"));
        VPA_PASS_ERROR(VPAAssert(config.viewportCount >= 0 && config.viewportCount <= m_limits.maxViewports, "viewportCount must be between 1 and VkPhysicalDeviceLimits::maxViewports, inclusive. Value is " + QString::number(config.viewportCount)));
        VPA_PASS_ERROR(VPAAssert(config.scissorCount >= 0 && config.scissorCount <= m_limits.maxViewports, "scissorCount must be between 1 and VkPhysicalDeviceLimits::maxViewports, inclusive. Value is " + QString::number(config.scissorCount)));
        for (uint32_t i = 0; i < config.scissorCount; ++i) {
            VPA_PASS_ERROR(VPAAssert(config.scissorRects[i].offset.x >= 0 && config.scissorRects[i].offset.y >= 0, "The x and y members of offset member of any element of pScissors must be greater than or equal to 0"));
            VPA_PASS_ERROR(VPAAssert(CheckSignedIntOverflow(config.scissorRects[i].extent.width, config.scissorRects[i].offset.x), "Evaluation of (offset.x + extent.width) must not cause a signed integer addition overflow for any element of pScissors"));
            VPA_PASS_ERROR(VPAAssert(CheckSignedIntOverflow(config.scissorRects[i].extent.height, config.scissorRects[i].offset.y), "Evaluation of (offset.y + extent.height) must not cause a signed integer addition overflow for any element of pScissors"));
        }
        VPA_PASS_ERROR(VPAAssert(config.writables.minSampleShading >= 0.0f && config.writables.minSampleShading <= 1.0f, "minSampleShading must be in the range [0, 1]"));

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
        int pos = 0;
        rule.name = ExtractStrInStr(line, pos);
        rule.reference = ExtractStrInStr(line, pos);

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
        if (list.size() == 1) {
            expr.refA.type = ValueType::Uint;
            expr.refA.ref = &True;
            expr.compareOp = list.at(0) == "TRUE" ? ComparisonOperator::Equal : ComparisonOperator::NotEqual;
            expr.valB.type = ValueType::Uint;
            expr.valB.value.u = 1;
        }
        else {
            expr.refA = ParseConfigIdentifier(config, list.at(0));
            expr.compareOp = ParseCompareOp(list.at(1));
            bool okFloat;
            float f = list.at(2).toFloat(&okFloat);
            bool okInt;
            float i = list.at(2).toFloat(&okInt);
            expr.valB = okFloat ? Val(f) : okInt ? Val(i) : ParseVulkanConstant(list.at(2));
        }
        return expr;
    }

    ConfigValidator::Ref ConfigValidator::ParseConfigIdentifier(const PipelineConfig& config, const QString& symbol) const {
        Ref r;
        r.type = ValueType::Uint;
        if (symbol == "topology") {
            r.ref = reinterpret_cast<const void*>(&config.writables.topology);
        }
        else if (symbol == "primitiveRestartEnable") {
            r.ref = reinterpret_cast<const void*>(&config.writables.primitiveRestartEnable);
        }
        else if (symbol == "minSampleShading") {
            r.ref = reinterpret_cast<const void*>(&config.writables.minSampleShading);
        }
        else if (symbol == "viewportCount") {
            r.ref = reinterpret_cast<const void*>(&config.viewportCount);
        }
        else if (symbol == "scissorCount") {
            r.ref = reinterpret_cast<const void*>(&config.scissorCount);
        }
        else if (symbol == "primitiveRestartEnable") {
            r.ref = reinterpret_cast<const void*>(&config.writables.primitiveRestartEnable);
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
        bool conditionsMet = Compare(rule.conditions[0]);
        for (int i = 1; i < rule.conditions.size(); ++i) {
            if (rule.conditionOps[i - 1] == LogicOperator::And) {
                conditionsMet = conditionsMet && Compare(rule.conditions[i]);
            }
            else {
                conditionsMet = conditionsMet || Compare(rule.conditions[i]);
            }
        }
        return !conditionsMet || Compare(rule.requirement);
    }

    bool ConfigValidator::Compare(const Expression& expr) const {
        switch (expr.compareOp) {
        case ComparisonOperator::Equal:
            return expr.refA == expr.valB;
        case ComparisonOperator::NotEqual:
            return expr.refA != expr.valB;
        case ComparisonOperator::Lesser:
            return expr.refA < expr.valB;
        case ComparisonOperator::LesserEqual:
            return expr.refA <= expr.valB;
        case ComparisonOperator::Greater:
            return expr.refA > expr.valB;
        case ComparisonOperator::GreaterEqual:
            return expr.refA >= expr.valB;
        }
        return false;
    }

    QString ConfigValidator::ExtractStrInStr(QString& str, int& pos) const {
        int startPos = str.indexOf("\"", pos);
        pos = str.indexOf("\"", startPos + 1) + 1;
        return str.mid(startPos, pos - startPos);
    }

    bool ConfigValidator::CheckSignedIntOverflow(uint32_t a, int32_t b) const {
        int64_t c;
        c = int64_t(a) + b;
        return c >= INT_MIN || c <= INT_MAX;

    }

    bool operator==(const ConfigValidator::Ref& ref, const ConfigValidator::Val& val) {
        if (ref.type == ConfigValidator::ValueType::Uint) {
            return *reinterpret_cast<const uint32_t*>(ref.ref) == val.value.u;
        }
        else if (ref.type == ConfigValidator::ValueType::Float) {
            return *reinterpret_cast<const float*>(ref.ref) < val.value.f + FLT_EPSILON &&
                    *reinterpret_cast<const float*>(ref.ref) > val.value.f - FLT_EPSILON;
        }
        else {
            return *reinterpret_cast<const int*>(ref.ref) == val.value.i;
        }
    }

    bool operator<(const ConfigValidator::Ref& ref, const ConfigValidator::Val& val) {
        if (ref.type == ConfigValidator::ValueType::Uint) {
            return *reinterpret_cast<const uint32_t*>(ref.ref) < val.value.u;
        }
        else if (ref.type == ConfigValidator::ValueType::Float) {
            return *reinterpret_cast<const float*>(ref.ref) < val.value.f;
        }
        else {
            return *reinterpret_cast<const int*>(ref.ref) < val.value.i;
        }
    }

    bool operator>(const ConfigValidator::Ref& ref, const ConfigValidator::Val& val) {
        if (ref.type == ConfigValidator::ValueType::Uint) {
            return *reinterpret_cast<const uint32_t*>(ref.ref) > val.value.u;
        }
        else if (ref.type == ConfigValidator::ValueType::Float) {
            return *reinterpret_cast<const float*>(ref.ref) > val.value.f;
        }
        else {
            return *reinterpret_cast<const int*>(ref.ref) > val.value.i;
        }
    }
}
