//
//  CCLocalization.hpp
//  cocos2d_libs
//
//  Created by shenxinxing on 3/16/16.
//
//

#ifndef CCLocalization_hpp
#define CCLocalization_hpp

#include <stdio.h>
#include <unordered_map>
#include "platform/CCPlatformMacros.h"

NS_CC_BEGIN

#define CC_LOCALIZATION(key) cocos2d::Localization::getInstance()->getLocalization(key, "")
#define CC_LOCALIZATION_FORMAT(key, format, ...) cocos2d::Localization::getInstance()->getLocalization(key, format, ##__VA_ARGS__)

class CC_DLL Localization
{
public:
    Localization();
    ~Localization();
    
    static Localization* getInstance();
    std::string getLocalization(const std::string& key, const char* format, ...);
    void addLocalization(const std::string& key, const std::string& value);
    void clear();
private:
    std::unordered_map<std::string, std::string> m_localization;
};
NS_CC_END
#endif /* CCLocalization_hpp */
