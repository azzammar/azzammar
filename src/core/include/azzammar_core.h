#ifndef AZZAMMAR_CORE_H
#define AZZAMMAR_CORE_H

#include <QString>

class AzzammarCore {
public:
    AzzammarCore();
    QString getSystemVersion() const;
    bool initializeEcosystem();
};

#endif // AZZAMMAR_CORE_H

