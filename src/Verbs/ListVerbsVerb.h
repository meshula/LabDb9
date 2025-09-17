#pragma once

#include "LabDb/Verbs.h"
#include <vector>
#include <string>
#include <mutex>

namespace LabDb {

class Db9Dispatcher;

class ListVerbsVerb : public IDb9Verb {
public:
    ListVerbsVerb();
    virtual ~ListVerbsVerb() = default;

    std::string getVerbName() const override;
    std::string getDescription() const override;
    
    Db9Response execute(const ::lab::Text::Sexpr& sexpr) override;

    // Static registration function
    static void registerVerb(Db9Dispatcher& dispatcher);

private:
    std::string formatVerbList(const std::vector<std::string>& verbs);
    
    static bool registered;
    static std::once_flag register_flag;
};

} // namespace LabDb
