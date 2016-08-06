//
//  CCLocalization.cpp
//  cocos2d_libs
//
//  Created by shenxinxing on 3/16/16.
//
//

#include "CCLocalization.hpp"
#include "base/ccMacros.h"

NS_CC_BEGIN
static Localization *s_SharedLocalization = nullptr;

Localization* Localization::getInstance()
{
    if (!s_SharedLocalization)
    {
        s_SharedLocalization = new (std::nothrow) Localization();
        CCASSERT(s_SharedLocalization, "FATAL: Not enough memory");
    }
    
    return s_SharedLocalization;
}

Localization::Localization()
{
    
}

Localization::~Localization(){
    
}

std::string Localization::getLocalization(const std::string& key, const char* format, ...){
    
    auto it = m_localization.find(key);
    
    if(it == m_localization.end())
        return key;
    
    if(!strlen(format))
        return it->second;
    
    char buf[512] = {0};
    std::string ret;
    va_list ap;
    va_start(ap, format);
    
    vsnprintf(buf, sizeof(buf), it->second.c_str(), ap);
    ret = buf;
    va_end(ap);
    
    return ret;
}

void Localization::addLocalization(const std::string &key, const std::string &value){
    auto it = m_localization.find(key);
    
    if(it != m_localization.end())
        return;
    
    m_localization.insert(std::pair<std::string, std::string>(key, value));
}

void Localization::clear(){
    m_localization.clear();
}

NS_CC_END