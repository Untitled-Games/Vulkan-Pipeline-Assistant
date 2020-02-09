#ifndef COMPILEERROR_H
#define COMPILEERROR_H

#include "common.h"

namespace vpa {
    struct CompileError {
        static constexpr int LinkerErrorValue = ~0;

        int lineNumber;
        QString message;

        bool operator==(int& i) {
            return i == lineNumber;
        }
        bool operator==(CompileError& other) {
            return other.lineNumber == lineNumber;
        }
    };

    inline int Find(QVector<CompileError>& errs, int line) {
        for (int i = 0; i < errs.size(); ++i) {
            if (errs[i].lineNumber == line) return i;
        }
        return -1;
    }
}

#endif // COMPILEERROR_H
